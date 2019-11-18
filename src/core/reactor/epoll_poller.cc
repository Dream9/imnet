/*************************************************************************
	> File Name: epoll_poller.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月07日 星期一 11时27分54秒
 ************************************************************************/
#ifdef _MSC_VER
#include"epoll_poller.h"
#include"../../dependent/logger/logger.h"
#include"../../dependent/types_extension.h"
#include"channel.h"

#else
#include"core/reactor/epoll_poller.h"
#include"core/reactor/channel.h"
#include"dependent/logger/logger.h"
#include"dependent/types_extension.h"
#endif

#include<assert.h>
#include<errno.h>

#include<sys/epoll.h>
#include<poll.h>

namespace imnet {

namespace core{

extern const int kNew;//定义在Channel.cc

}
	
namespace detail {

static_assert(EPOLLIN == POLLIN, "epoll should use same flag value as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll should use same flag value as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll should use same flag value as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll should use same flag value as poll");
static_assert(EPOLLERR == POLLERR, "epoll should use same flag value as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll should use same flag value as poll");

//搞错命名空间了
//extern const int kNew;//定义在Channel.cc

namespace {
const int kRemoved = 1;
const int kAdded = 2;

//brief:epoll_ctl操作信息输出
const char* operationToString(int op) {
	switch (op) {
	case EPOLL_CTL_DEL:
		return "DEL";
	case EPOLL_CTL_ADD:
		return "ADD";
	case EPOLL_CTL_MOD:
		return "MOD";
	default:
		return "Unknown";
	}
}
}

//epoll_create1()
//If flags is 0, then, other than the fact that the obsolete size
//argument is dropped, epoll_create1() is the same as epoll_create().
//The following value can be included in flags to obtain different
//behavior :
//
//EPOLL_CLOEXEC
//Set the close - on - exec(FD_CLOEXEC) flag on the new file
//descriptor.See the description of the O_CLOEXEC flag in
//open(2) for reasons why this may be useful.

//brief:初始化
EPollPoller::EPollPoller(core::Eventloop* loop)
	:Poller(loop),
	__epollfd(::epoll_create1(EPOLL_CLOEXEC)),
	__epoll_events(16){
	if (__epollfd < 0)
		LOG_SYSFATAL << "EPollPoller:epoll_create1 failed";
}

EPollPoller::~EPollPoller() = default;

//brief:更新或者添加Channel
//becare:和PollPoller不同，对于不在关心事件发生的fd必须从底层rb-tree中删除，而不能通过设为负数忽略之
void EPollPoller::updateChannel(core::Channel* channel) {
	Poller::assertInLoopThread();
	assert(channel);
	assert(channel->getEventloop() == Poller::__loop);

	LOG_TRACE << "fd:" << channel->getHandle() << "events:" << channel->getEvents();
	//if (channel->getIndex < 0) {
	//新产生的或者已经移除的都需要通过EPOLL_CTL_ADD操作
	int index = channel->getIndex();
	int tmpfd = channel->getHandle();
	if (index == core::kNew || index == kRemoved ) {
		//添加新的或者已经删除的(即重新激活之)
		if (index == kRemoved) {
			assert(__channels.find(tmpfd) != __channels.end());
			assert(__channels[tmpfd] == channel);
		}
		else {
			//assert(__channels.find(tmpfd) != __channels.end());
			assert(__channels.find(tmpfd) == __channels.end());
			__channels.insert({ tmpfd,channel });
		}
		channel->setIndex(kAdded);
		__epollCtl(channel, EPOLL_CTL_ADD);
	}
	else {
		//更新已经添加的channel
		assert(__channels.find(tmpfd) != __channels.end());
	    assert(__channels[tmpfd] == channel);
		assert(index == kAdded);

		if (channel->isNoneActivated()) {
			//区别于PollPoller,这里需要真的删除
			__epollCtl(channel, EPOLL_CTL_DEL);
			channel->setIndex(kRemoved);
		}
		else {
			__epollCtl(channel, EPOLL_CTL_MOD);
		}
	}
}

//brief:去除channel
//     本函数在TcpConnection关闭后，被注册到工作队列中，作为善后工作的一部分
//becare:被删除的channel,应该已经停止关心事件发生，
void EPollPoller::eraseChannel(core::Channel* channel) {
	Poller::assertInLoopThread();
	assert(channel);
	assert(channel->getEventloop() == Poller::__loop);

	LOG_TRACE << "fd:" << channel->getHandle();
	int tmpfd = channel->getHandle();
	int index = channel->getIndex();
	assert(__channels.find(tmpfd) != __channels.end());
	assert(__channels[tmpfd] == channel);
	assert(channel->isNoneActivated());
	assert(index == kAdded || index == kRemoved);

	size_t n = __channels.erase(tmpfd);
	assert(n == 1); (void)n;

	if (index == kAdded) {
		__epollCtl(channel, EPOLL_CTL_DEL);
	}

	//这项设置意义不大，该函数往往作为Channel析构前的最后依次回调
	channel->setIndex(core::kNew);
}

//brief:IO多路复用
//becare:必须处理信号唤醒的情况，或者采用epoll_pwait()
//parameter:active_channels可能会被修改，Eventloop调用之前务必先clear
CTimestamp EPollPoller::poll(int timeout_microseconds, ChannelList* active_channles) {
	int number = ::epoll_wait(__epollfd, &*__epoll_events.begin(), __epoll_events.size(), timeout_microseconds);
	int current_errno = errno;
	CTimestamp now(CTimestamp::getNow());

	if (number < 0) {
		//处理信号唤醒
		if(current_errno!=EINTR){
			errno = current_errno;
			LOG_SYSERR << "epoll_wait failed()";
		}
	}
	else if (number == 0) {
		//处理超时
		LOG_TRACE << "timeout expires";
	}
	else {
		//处理就绪的事件
		LOG_TRACE << number << " fds are ready";
		__fillActiveChannels(active_channles, number);

		if (static_cast<size_t>(number) == __epoll_events.size()) {
			__epoll_events.resize(number * 2);
		}
	}
	return now;
}

//brief:将就绪的事件传递给用户
void EPollPoller::__fillActiveChannels(EPollPoller::ChannelList* active_channels, int number_of_events) {
	for (int i = 0; i < number_of_events; ++i) {
		core::Channel* cur = reinterpret_cast<core::Channel*>(__epoll_events[i].data.ptr);
#ifndef NDEBUG
		auto iter = __channels.find(cur->getHandle());
		assert(iter != __channels.end());
		assert(iter->second == cur);
#endif
		cur->setRevents(__epoll_events[i].events);
		active_channels->emplace_back(cur);
	}
}

//brief:对epoll_ctl的封装
void EPollPoller::__epollCtl(core::Channel* channel, int op) {

	//typedef union epoll_data {
	//	void        *ptr;
	//	int          fd;
	//	uint32_t     u32;
	//	uint64_t     u64;
	//} epoll_data_t;

	//struct epoll_event {
	//	uint32_t     events;      /* Epoll events */
	//	epoll_data_t data;        /* User data variable */
	//};

	struct epoll_event epevent;
	epevent.events = channel->getEvents();
	epevent.data.ptr = channel;
	int tmpfd = channel->getHandle();
	LOG_TRACE << "epoll_ctl op=" << operationToString(op) << " fd=" << tmpfd
		<< ",event=" << channel->eventsToString();

	if (::epoll_ctl(__epollfd, op, tmpfd, &epevent) < 0) {
		if (op == EPOLL_CTL_DEL) {
			LOG_SYSERR << "epoll_ctl DEL failed.fd=" << tmpfd;
		}
		else {
			LOG_SYSFATAL << "epoll_ctl " << operationToString(op) << " failed.fd=" << tmpfd;
		}
	}
}

}
}
