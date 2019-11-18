/*************************************************************************
	> File Name: poll_poller.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月07日 星期一 11时27分06秒
 ************************************************************************/
#ifdef _MSC_VER
#include"poll_poller.h"
#include"../../dependent/logger/logger.h"
#include"../../dependent/types_extension.h"
#include"channel.h"
#else 
#include"core/reactor/channel.h"
#include"dependent/logger/logger.h"
#include"dependent/types_extension.h"
#include"core/reactor/poll_poller.h"
#endif

#include<assert.h>
#include<errno.h>

#include<poll.h>

namespace imnet {

namespace detail {

PollPoller::PollPoller(core::Eventloop* loop) :Poller(loop) {

}
PollPoller::~PollPoller() = default;

//brief:更新或者追加新的Channel,并负责更新或者追加相对应的pollfd结构消息
//becare：仅在Eventloop线程中调用本函数，则不存在竞争
void PollPoller::updateChannel(core::Channel* channel) {
	Poller::assertInLoopThread();
	assert(channel);
	assert(channel->getEventloop() == Poller::__loop);

	LOG_TRACE << "fd:" << channel->getHandle() << "events:" << channel->getEvents();
	int index = channel->getIndex();
	//主要是更新
	if (index > 0) {
		//index大于0为合法索引，此时仅需要跟新pollfd结构即可
		int tmpfd = channel->getHandle();
		assert(__channels.find(tmpfd) != __channels.end());
		assert(__channels[tmpfd] == channel);
		assert(static_cast<size_t>(index) < __pollfds.size());
		assert(__pollfds[index].fd == tmpfd || __pollfds[index].fd == -tmpfd - 1);

		//struct pollfd {
		//	int   fd;         /* file descriptor */
		//	short events;     /* requested events */
		//	short revents;    /* returned events */
		//};
		//设置pollfd的信息
		//becare:对于不在关心事件发生的描述符，将其fd设置为取反-1(既可以被忽略也可以防止0)
		struct pollfd& pfd = __pollfds[index];
		pfd.fd = channel->isNoneActivated() ? -tmpfd - 1 : tmpfd;
		pfd.events = static_cast<short>(channel->getEvents());
		pfd.revents = 0;
	}
	else {
		//需要创建新的pollfd结构，并且更新map记录结构
		assert(false == Poller::hasChannel(channel));

		//追加新的pollfd
		int tmpfd = channel->getHandle();
		struct pollfd pfd;
		pfd.fd = channel->isNoneActivated() ? -tmpfd - 1 : tmpfd;
		pfd.events = static_cast<short>(channel->getEvents());
		pfd.revents = 0;

		__pollfds.emplace_back(pfd);
		int index = static_cast<int>(__pollfds.size());
		channel->setIndex(index);
		__channels.insert({ index,channel });
	}
}

//brief:去除channel
//     本函数在TcpConnection关闭后，被注册到工作队列中，作为善后工作的一部分
//becare:被删除的channel,应该已经停止关心事件发生，
void PollPoller::eraseChannel(core::Channel* channel) {
	Poller::assertInLoopThread();
	assert(channel);
	assert(channel->getEventloop() == Poller::__loop);

	LOG_TRACE << "fd:" << channel->getHandle();
	int tmpfd = channel->getHandle();
	int index = channel->getIndex();
	assert(__channels.find(tmpfd) != __channels.end());
	assert(__channels[tmpfd] == channel);
	assert(channel->isNoneActivated());
	assert(static_cast<size_t>(index) < __pollfds.size());
	assert(__pollfds[index].fd = -tmpfd - 1);

	//在map<int,Channel*>中删除
	size_t n = __channels.erase(tmpfd);
	assert(n == 1); (void)n;

	//在vector<struct pollfd>中删除
	//采用了swap+pop_back()的方式
	if (static_cast<size_t>(index) != __pollfds.size() - 1) {
		__pollfds[index] = __pollfds.back();
		int fd = __pollfds[index].fd;
		fd = fd < 0 ? -fd - 1 : fd;
		auto iter=__channels.find(fd);
		assert(iter != __channels.end());

		iter->second->setIndex(index);
	}
	__pollfds.pop_back();

	//这项设置意义不大，该函数往往作为Channel析构前的最后依次回调
	channel->setIndex(-1);
}

//brief:IO多路复用
//becare:必须处理信号唤醒的情况，或者采用ppoll
//parameter:active_channels可能会被修改，Eventloop调用之前务必先clear
CTimestamp PollPoller::poll(int timeout_microseconds, PollPoller::ChannelList* active_channels) {
	Poller::assertInLoopThread();
	//The call will block until either :
	//*a file descriptor becomes ready;
	//*the call is interrupted by a signal handler; or
	//*the timeout expires.
	int happend_events = ::poll(&*__pollfds.begin(), __pollfds.size(), timeout_microseconds);
	int current_errno = errno;
	CTimestamp now(CTimestamp::getNow());

	if (happend_events < 0) {
		//处理信号唤醒
		if (current_errno != EINTR) {
			errno = current_errno;
			LOG_SYSERR << "PollPoller::poll() failed.";
		}
	}
	else if (happend_events == 0) {
		//处理超时
		LOG_TRACE << "timeout expires";
	}
	else {
		//处理就绪的事件
		LOG_TRACE << happend_events << "events are ready";
		__fillActiveChannels(active_channels,happend_events);
	}

	return now;
}

//brief:当poll返回之后，遍历所有的__pollfds,寻找发生的事件，并装填到指定容器
void PollPoller::__fillActiveChannels(PollPoller::ChannelList* active_channels, int number_of_events) {
	for (auto iter = __pollfds.begin(); number_of_events > 0 && iter != __pollfds.end(); ++iter) {
		if (iter->revents > 0) {
			--number_of_events;
			auto item = __channels.find(iter->fd);
#ifndef NDEBUG
			//检测合法性
			if (item == __channels.end()) {
				LOG_FATAL << "PollPoller::__fillActiveChannels,there is not a channel that can match a happended fd";
			}
			else if (item->second->getHandle() != iter->fd) {
				LOG_FATAL << "PollPoller::__fillActiveChannels,struct pollfd item's fd"
					<< iter->fd << " doesn't match channel's fd" << item->second->getHandle();
			}
#endif
			core::Channel* temp = item->second;
			temp->setRevents(iter->revents);
			active_channels->emplace_back(temp);
		}
	}
}

}
}
