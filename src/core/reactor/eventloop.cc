/*************************************************************************
	> File Name: eventloop.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 09时51分39秒
 ************************************************************************/
#ifdef _MSC_VER
#include"eventloop.h"
#include"poller.h"
#include"../../dependent/logger/logger.h"
#include"channel.h"
#include"./timerwork_queue.h"
#else
#include"core/reactor/eventloop.h"
#include"core/reactor/poller.h"
#include"dependent/logger/logger.h"
#include"core/reactor/channel.h"
#include"core/reactor/timerwork_queue.h"
#endif

#include<signal.h>
#include<sys/eventfd.h>
#include<unistd.h>

#include<algorithm>

#ifdef _WIN32
typedef int ssize_t;
#endif

namespace {
//brief:one loop per thread
thread_local imnet::core::Eventloop* tl_current_thread_eventloop;
const int kPollTimeoutMicrosecond = 10000;

//忽略SIGPIPE信号
class IgnoreSigPipe {
public:
	IgnoreSigPipe() {
		::signal(SIGPIPE, SIG_IGN);
	}
};
IgnoreSigPipe auto_ignore_sigpipe;

//eventfd() creates an "eventfd object" that can be used as an event
//wait / notify mechanism by user - space applications, and by the kernel
//to notify user - space applications of events.The object contains an
//unsigned 64 - bit integer(uint64_t) counter that is maintained by the
//kernel.This counter is initialized with the value specified in the
//argument initval.
//
//As its return value, eventfd() returns a new file descriptor that can
//be used to refer to the eventfd object.
//
//The following values may be bitwise ORed in flags to change the
//behavior of eventfd() :
//
//	EFD_CLOEXEC(since Linux 2.6.27)
//	Set the close - on - exec(FD_CLOEXEC) flag on the new file
//	descriptor.See the description of the O_CLOEXEC flag in
//	open(2) for reasons why this may be useful.
//
//	EFD_NONBLOCK(since Linux 2.6.27)
//	Set the O_NONBLOCK file status flag on the open file
//	description(see open(2)) referred to by the new file
//	descriptor.Using this flag saves extra calls to fcntl(2) to
//	achieve the same result.
//
//	EFD_SEMAPHORE(since Linux 2.6.30)
//	Provide semaphore - like semantics for reads from the new file
//	descriptor.See below.
//
//	In Linux up to version 2.6.26, the flags argument is unused, and must
//	be specified as zero.

//brief:创建eventfd
int createEventfd() {
	int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (fd < 0) {
		LOG_SYSERR << "createEventfd():faild";
		abort();
	}
	return fd;
}

}//!namespace

namespace imnet {

namespace core {

/*
pid_t __thread_id;
	bool __is_looping;
	bool __is_handling_event;
	bool __is_handling_work_queue;
	std::atomic_bool __need_quit_loop;

	int64_t __loop_iteration_count;
	CTimestamp __poll_return_time;

	boost::any __context;//辅助用户，可以存储任意类型数据的上下文环境

	//brief:复合
	std::unique_ptr<Poller> __poller;//负责等待事件
	std::unique_ptr<TimerWorkQueue> __timer_work_queue;//负责超时工作队列

	int __wakeup_loop_fd;
	std::unique_ptr<Channel> __wakeup_loop_channel;//响应第三方唤醒

	ChannelContainerType __activeChannels;//由Poller负责维护的当前事件集合
	Channel* __current_working_on_channel;

	mutable Mutex __mutex;
	s
*/

//brief:初始化各项参数，包括线程缓存Eventloop指针，定时工作队列，以及Poller
Eventloop::Eventloop()
	:__thread_id(thread_local_variable::gettid()),
	__is_looping(false),
    __is_handling_event(false),
    __is_handling_work_queue(false),
    __need_quit_loop(false),
	__loop_iteration_count(0),
    __poller(Poller::getPollerFromEnvironment(this)),
	__timer_work_queue(new TimerworkQueue(this)),
    __wakeup_loop_fd(createEventfd()),
	__wakeup_loop_channel(new Channel(this,__wakeup_loop_fd)),
	__current_working_on_channel(nullptr)
{
	LOG_DEBUG << "Eventloop " << this << "was created in thread" << __thread_id;

	//becare:保证IO线程只有唯一的Eventloop
	if (tl_current_thread_eventloop) {
		LOG_FATAL << "Unexpected Eventloop object " << tl_current_thread_eventloop << "exists";
	}
	else {
		tl_current_thread_eventloop = this;
	}

	//设置eventfd的响应处理，可以唤醒loop
	__wakeup_loop_channel->setReadCallback(std::bind(&Eventloop::handleRead, this));
	__wakeup_loop_channel->activateRead();
}

//brief:善后工作，主要是清除与线程的关联，同时先停止检测唤醒事件，然后关闭eventfd
//becare:本对象应该已经不再监听事件
Eventloop::~Eventloop() {
	assert(!__is_looping);

	LOG_DEBUG << "Eventloop " << this << "destructed";
	__wakeup_loop_channel->deactivateAll();
	__wakeup_loop_channel->removeFromEventloop();
	::close(__wakeup_loop_fd);
	tl_current_thread_eventloop = nullptr;
}


//brief:终止执行
void Eventloop::__abortForOutOfLoopThread() {
	LOG_FATAL << "EventLoop::__abortForOutOfLoopThread:" << this
		<< ",current tid is " << thread_local_variable::gettid()
		<< ",expected tid is " << __thread_id;
}

//brief:核心循环
void Eventloop::loop() {
	assert(!__is_looping);
	assertInLoopThread();

	__is_looping = true;
	__need_quit_loop = false;
	LOG_TRACE << "Eventloop:" << this << " start looping";

	//while (::__builtin_expect(__need_quit_loop, false)) {
	while (!__need_quit_loop) {

		//等待事件发生，并由Poller负责填充活动的Channel
		__activeChannels.clear();
		__poll_return_time = __poller->poll(kPollTimeoutMicrosecond, &__activeChannels);
		++__loop_iteration_count;
		
		//for test
		//LOG_INFO << __loop_iteration_count;

#ifndef NDEBUG
		//输出详细信息
		if (CLogger::getLogLevel() <= CLogger::kTrace) {
			__debugActiveChannelInformation();
		}
#endif

		//处理事件
		__is_handling_event = true;
		for (Channel* channel : __activeChannels) {
			__current_working_on_channel = channel;
			__current_working_on_channel->handleEvent(__poll_return_time);
		}
		__current_working_on_channel = nullptr;
		__is_handling_event = false;

		//处理工作队列
		__processPendingWork();
	}

	LOG_TRACE << "Eventloop:" << this << " finish looping";
	__is_looping = false;
}

//brief:结束当前循环
//becare:发送结束信号，但仍然需要等待最后一个loop结束
//      当处于统一IO线程时，可以立即生效
//      (因为调用者一定要么位于handling event,要么位于processing pending work)
//      相反，如果从外部结束，应当保证其已经被唤醒
void Eventloop::quit() {
	//FIXME:
	//__need_quit_loop = false;
	__need_quit_loop = true;
	if (!isInLoopThread()) {
		wakeupEventloop();
	}
}

//brief:将队列注入到loop中
//becare:如果在IO线程，那么可以立刻执行，否则需要加入到队列之中
void Eventloop::pushBackIntoLoop(Eventloop::CallbackFunctionType work) {
	if (isInLoopThread()) {
		work();
	}
	else {
		//本身是值传递，故移动之后不会影响到原来的值
		pushBackIntoQueue(std::move(work));
	}
}

//brief:将工作注入到队列之中
void Eventloop::pushBackIntoQueue(Eventloop::CallbackFunctionType work) {
	{
		//添加工作到队列中
		//以下为临界区
		imnet::MutexUnique_lock unique_lock(__mutex);
		__pending_work.emplace_back(std::move(work));
		//以上为临界区
	}

	//becare:只有一种情况之下是不需要唤醒操作的，即__is_handling_event==true isInLoopThread()==true
	//       主要是在__processPendingWork()中，会交换获得当前的pending queue,否则就要由下一次loop完成本任务
	//       此时需要保证会被唤醒
	if (!isInLoopThread() || __is_handling_work_queue) {
		wakeupEventloop();
	}
}

//return:待处理工作队列大小
size_t Eventloop::getQueueSize()const {
	return __pending_work.size();
}

//brief:添加定时任务
TimerworkId Eventloop::runAt(Eventloop::CallbackFunctionType work, CTimestamp when) {
	return __timer_work_queue->add(std::move(work),when,-1.0);
}
TimerworkId Eventloop::runAfter(Eventloop::CallbackFunctionType work, double seconds) {
	CTimestamp when(timestampAdd(CTimestamp::getNow(), seconds));
	return __timer_work_queue->add(std::move(work), when, -1.0);
}
TimerworkId Eventloop::runEvery(Eventloop::CallbackFunctionType work, double interval_seconds) {
	CTimestamp when(timestampAdd(CTimestamp::getNow(), interval_seconds));
	return __timer_work_queue->add(std::move(work), when, interval_seconds);
}

//brief:删除定时任务
void Eventloop::cancel(TimerworkId* timerwork_id) {
	return __timer_work_queue->cancel(*timerwork_id);
}
void Eventloop::cancel(TimerworkId timerwork_id) {
	return __timer_work_queue->cancel(std::move(timerwork_id));
}

//brief:同步Channel信息到Poller
void Eventloop::updateChannel(Channel* channel) {
	assertInLoopThread();
	__poller->updateChannel(channel);
}
void Eventloop::removeChannel(Channel* channel) {
	assertInLoopThread();
	__poller->eraseChannel(channel);
}

bool Eventloop::hasChannel(Channel* channel) {
	assertInLoopThread();
	return __poller->hasChannel(channel);
}

//brief:通过向eventfd中写入数据唤醒Eventloop
void Eventloop::wakeupEventloop() {
	//write(2)
	//	A write(2) call adds the 8 - byte integer value supplied in its
	//	buffer to the counter.The maximum value that may be stored
	//	in the counter is the largest unsigned 64 - bit value minus 1
	//	(i.e., 0xfffffffffffffffe).If the addition would cause the
	//	counter's value to exceed the maximum, then the write(2)
	//	either blocks until a read(2) is performed on the file
	//	descriptor, or fails with the error EAGAIN if the file
	//	descriptor has been made nonblocking.

	//	A write(2) fails with the error EINVAL if the size of the
	//	supplied buffer is less than 8 bytes, or if an attempt is made
	//	to write the value 0xffffffffffffffff.

	//int data = 1;//要求写入8bits
	uint64_t data = 1;
	ssize_t n = ::write(__wakeup_loop_fd, &data, sizeof data);
	if (n != sizeof data) {
		LOG_ERROR << "Eventloop::wakeupEventloop():only write" << n << "bytes";
	}
}

//brief:Eventloop需要自己响应eventfd的事件处理
void Eventloop::handleRead() {
	//read(2)
	//	Each successful read(2) returns an 8 - byte integer.A read(2)
	//	fails with the error EINVAL if the size of the supplied buffer
	//	is less than 8 bytes.

	//	The value returned by read(2) is in host byte order—that is,
	//	the native byte order for integers on the host machine.

	//	The semantics of read(2) depend on whether the eventfd counter
	//	currently has a nonzero value and whether the EFD_SEMAPHORE
	//	flag was specified when creating the eventfd file descriptor :

	//*If EFD_SEMAPHORE was not specified and the eventfd counter
	//	has a nonzero value, then a read(2) returns 8 bytes
	//	containing that value, and the counter's value is reset to
	//	zero.

	//	*  If EFD_SEMAPHORE was specified and the eventfd counter has
	//	a nonzero value, then a read(2) returns 8 bytes containing
	//	the value 1, and the counter's value is decremented by 1.

	//	*  If the eventfd counter is zero at the time of the call to
	//	read(2), then the call either blocks until the counter
	//	becomes nonzero(at which time, the read(2) proceeds as
	//		described above) or fails with the error EAGAIN if the file
	//	descriptor has been made nonblocking.

	uint64_t data;
	ssize_t n = ::read(__wakeup_loop_fd, &data, sizeof data);
	if (n != sizeof data) {
		LOG_ERROR << "Eventloop::handleRead():only read" << n << "bytes";
	}
}

//brief:处理工作队列
//      为了减少临界区，通过swap将数据交换出来
//becare:当处于__is_handling_work_queue状态时，新加入的工作任务都必须延迟到下次loop
void Eventloop::__processPendingWork() {
	__is_handling_work_queue = true;
	std::vector<CallbackFunctionType> temp;

	{
		//临界区内交换数据
		imnet::MutexUnique_lock unique_lock(__mutex);
		temp.swap(__pending_work);
	}

	std::for_each(temp.begin(), temp.end(),
		[](const CallbackFunctionType& work) {work(); });

	__is_handling_work_queue = false;
}
//brief:one loop per thread,从IO线程中获得当前Eventloop
Eventloop* Eventloop::getCurrentThreadEventloopHandler() {
	return tl_current_thread_eventloop;
}

//brief:Debug模式下记录事件信息
//becare:需要全局输出等级大于等于kTrace
void Eventloop::__debugActiveChannelInformation() {
	std::for_each(__activeChannels.begin(), __activeChannels.end(),
		[](Channel* ptr) {LOG_TRACE << "{" << ptr->reventsToString() << "}"; });
}
}//!namespace core

}//!namespace imnet


