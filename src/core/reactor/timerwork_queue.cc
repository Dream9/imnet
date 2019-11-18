/*************************************************************************
	> File Name: timer_work_queue.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 10时39分19秒
 ************************************************************************/
#ifdef _MSC_VER
#include"./timerwork_queue.h"
#include"./timerwork_id.h"
#include"../../dependent/logger/logger.h"
#include"eventloop.h"
#else
#include"core/reactor/timerwork_queue.h"
#include"core/reactor/timerwork_id.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#endif

#include<sys/timerfd.h>
#include<unistd.h>

namespace imnet {

namespace core {

namespace detail {

//brief:关于timerfd的操作
//timerfd_create() creates a new timer object, and returns a file
//descriptor that refers to that timer.The clockid argument specifies
//the clock that is used to mark the progress of the timer, and must be
//one of the following :
//
//CLOCK_REALTIME
//A settable system - wide real - time clock.
//
//CLOCK_MONOTONIC
//A nonsettable monotonically increasing clock that measures
//time from some unspecified point in the past that does not
//change after system startup.
//
//CLOCK_BOOTTIME(Since Linux 3.15)
//Like CLOCK_MONOTONIC, this is a monotonically increasing
//clock.However, whereas the CLOCK_MONOTONIC clock does not
//measure the time while a system is suspended, the
//CLOCK_BOOTTIME clock does include the time during which the
//system is suspended.This is useful for applications that
//need to be suspend - aware.CLOCK_REALTIME is not suitable for
//such applications, since that clock is affected by
//discontinuous changes to the system clock.
//
//CLOCK_REALTIME_ALARM(since Linux 3.11)
//This clock is like CLOCK_REALTIME, but will wake the system if
//it is suspended.The caller must have the CAP_WAKE_ALARM
//capability in order to set a timer against this clock.
//
//CLOCK_BOOTTIME_ALARM(since Linux 3.11)
//This clock is like CLOCK_BOOTTIME, but will wake the system if
//it is suspended.The caller must have the CAP_WAKE_ALARM
//capability in order to set a timer against this clock.
int createTimerfd() {
	int ret = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (ret < 0) {
		LOG_SYSFATAL << "createTimerfd() failed";
	}
	return ret;
}

void readTimerfd(int timerfd) {
	uint64_t data;
	ssize_t n = ::read(timerfd, &data, sizeof data);
	//LOG_TRACE<<"TimerworkQueue::handleRead()"
	if (n != sizeof data) {
		LOG_ERROR << "read() "<<timerfd<<",only get " << n << " bytes";
	}
}

//timerfd_settime() arms(starts) or disarms(stops) the timer referred
//to by the file descriptor fd.
//
//The new_value argument specifies the initial expiration and interval
//for the timer.The itimerspec structure used for this argument
//contains two fields, each of which is in turn a structure of type
//timespec :
//
//struct timespec {
//	time_t tv_sec;                /* Seconds */
//	long   tv_nsec;               /* Nanoseconds */
//};
//
//struct itimerspec {
//	struct timespec it_interval;  /* Interval for periodic timer */
//	struct timespec it_value;     /* Initial expiration *//////需要修改值
//};

//new_value.it_value specifies the initial expiration of the timer, in
//seconds and nanoseconds.Setting either field of new_value.it_value
//to a nonzero value arms the timer.Setting both fields of
//new_value.it_value to zero disarms the timer.
//
//Setting one or both fields of new_value.it_interval to nonzero values
//specifies the period, in seconds and nanoseconds, for repeated timer
//expirations after the initial expiration.If both fields of
//new_value.it_interval are zero, the timer expires just once, at the
//time specified by new_value.it_value.
//
//By default, the initial expiration time specified in new_value is
//interpreted relative to the current time on the timer's clock at the
//time of the call(i.e., new_value.it_value specifies a time relative
//	to the current value of the clock specified by clockid).An absolute
//	timeout can be selected via the flags argument.
//
//	The flags argument is a bit mask that can include the following val‐
//	ues :
//
//TFD_TIMER_ABSTIME
//Interpret new_value.it_value as an absolute value on the
//timer's clock.  The timer will expire when the value of the
//timer's clock reaches the value specified in
//new_value.it_value.
//
//TFD_TIMER_CANCEL_ON_SET
//If this flag is specified along with TFD_TIMER_ABSTIME and the
//clock for this timer is CLOCK_REALTIME or CLOCK_REAL‐
//TIME_ALARM, then mark this timer as cancelable if the real -
//time clock undergoes a discontinuous change(settimeofday(2),
//	clock_settime(2), or similar).When such changes occur, a
//	current or future read(2) from the file descriptor will fail
//	with the error ECANCELED.
//
//	If the old_value argument is not NULL, then the itimerspec structure
//	that it points to is used to return the setting of the timer that was
//	current at the time of the call; see the description of timerfd_get‐
//	time() following.

//brief:初始化时间结构
void initialExpirationOfTheTimer(struct timespec &to_be_set, CTimestamp when) {
	//int64_t microseconds = when.getMicroseconds() - CTimestamp::getNow().getMicroseconds();
	int64_t microseconds = when.data() - CTimestamp::getNow().data();
	microseconds = microseconds < 100 ? 100 : microseconds;

	to_be_set.tv_sec = static_cast<time_t>(microseconds / CTimestamp::kRatioFromMicrosecondToSecond);
	to_be_set.tv_nsec = static_cast<long>(microseconds % CTimestamp::kRatioFromMicrosecondToSecond * 1000);
}

//brief:重新设置超时时间
void settime(int timerfd, CTimestamp when) {
	struct itimerspec new_time;
	memZero(&new_time, sizeof  new_time);
	initialExpirationOfTheTimer(new_time.it_value, when);
	int ret = ::timerfd_settime(timerfd, 0, &new_time, nullptr);
	if (ret) {
		LOG_SYSERR << "timerfd_settime() failed";
	}
}

}//!namespace detail

//brief:负责构建新的timerfd并将关注事件注册给Eventloop
TimerworkQueue::TimerworkQueue(Eventloop *loop)
	:__loop(loop),
	__timerfd(detail::createTimerfd()),
	__channel_of_timerfd(__loop,__timerfd),
	__is_processing_expired_timers(false){
	__channel_of_timerfd.setReadCallback(std::bind(&TimerworkQueue::__handleRead, this));
	__channel_of_timerfd.activateRead();
}

//brief:析构时先将注册事件去除，然后释放句柄
//becare:timerfd的生命期由TimerworkQueue掌控，
TimerworkQueue::~TimerworkQueue() {
	__channel_of_timerfd.deactivateAll();
	__channel_of_timerfd.removeFromEventloop();
	::close(__timerfd);
}

//brief:添加定时任务
//becare:返回TimerworkId，由用户负责保存使用
TimerworkId TimerworkQueue::add(const TimerworkQueue::TimerworkCallbackType& work, CTimestamp expired_time, double interval) {
	Timerwork* timer_work = new Timerwork(work, expired_time, interval);

	__loop->pushBackIntoLoop(std::bind(&TimerworkQueue::__addTimerworkInLoop, this, timer_work));

	return TimerworkId(timer_work, timer_work->getSequence());
}
TimerworkId TimerworkQueue::add(TimerworkQueue::TimerworkCallbackType&& work, CTimestamp expired_time, double interval) {
	Timerwork* timer_work = new Timerwork(work, expired_time, interval);

	__loop->pushBackIntoLoop(std::bind(&TimerworkQueue::__addTimerworkInLoop, this, timer_work));

	return TimerworkId(timer_work, timer_work->getSequence());
}

//brief:删除定时任务
//parameter:add时得到的TimerworkId
void TimerworkQueue::cancel(const TimerworkId& timerword_id) {
	__loop->pushBackIntoLoop(std::bind(&TimerworkQueue::__cancelTimerworkInLoop, this, timerword_id));
}
void TimerworkQueue::cancel(TimerworkId&& timerword_id) {
	__loop->pushBackIntoLoop(std::bind(&TimerworkQueue::__cancelTimerworkInLoop, this, timerword_id));
}

////////////以下为内部实现
//becarr：保证在IO线程内完成操作，不存在竞争关系
void TimerworkQueue::__addTimerworkInLoop(Timerwork* timer_work) {
	__loop->assertInLoopThread();

	bool need_reset_timerfd = __insert(timer_work);
	if (need_reset_timerfd) {
		detail::settime(__timerfd, timer_work->getExpiration());
	}
}

//brief：在IO线程中删除任务
//becare:字注销发生在：超时后任务调度过程中，取消自己。
void TimerworkQueue::__cancelTimerworkInLoop(const TimerworkId& timerwork_id) {
	__loop->assertInLoopThread();
	assert(__container_for_locaiton.size() == __container_for_timer.size());

	auto iter = __container_for_locaiton.find({ timerwork_id.__ptr,timerwork_id.__sequence });
	if (iter != __container_for_locaiton.end()) {
		//becare:首先要保证存在
		LOG_TRACE << "A timerwork(" << timerwork_id.__sequence << ") is canceled.";
		__container_for_timer.erase(WorkType(iter->first->getExpiration(),iter->first));
		size_t n = __container_for_timer.erase(WorkType(iter->first->getExpiration(), iter->first));
		assert(n == 1); (void)n;
		delete iter->first;
		__container_for_locaiton.erase(iter);
	}
	else if (__is_processing_expired_timers) {
		//becare:针对子注销的情况，标记之，保证不在__reset中将其放回
		LOG_TRACE << "A timerwork(" << timerwork_id.__sequence << ") is self canceled.";
		__container_for_self_cancel.insert({ timerwork_id.__ptr,timerwork_id.__sequence });
	}
	else {
		LOG_WARN << "attempt to cancel a timerwork that not exitsts:" << timerwork_id.__sequence;
	}
}

//brief:超时处理例程
//      从timerfd读取数据，寻找并取出超时任务，依次执行超时任务，将未取消并且要求重复的任务重新放回
void TimerworkQueue::__handleRead() {
	__loop->assertInLoopThread();

	detail::readTimerfd(__timerfd);

	CTimestamp now = CTimestamp::getNow();
	auto expired_work = __getExpired(now);

	//以下执行超时任务，并标记自注销
	__is_processing_expired_timers = true;
	for (const auto& item : expired_work) {
		item.second->start();//可能发生自注销的地方
	}
	__is_processing_expired_timers = false;
	//以上执行超时任务，并标记自注销

	__reset(expired_work, now);
}

//brief:取出超时任务
//      借助于lower_bound找到第一个大于等于的值，即第一个未超时的任务
std::vector<TimerworkQueue::WorkType> TimerworkQueue::__getExpired(CTimestamp now) {
	assert(__container_for_locaiton.size() == __container_for_timer.size());

	WorkType current_sentry(now, reinterpret_cast<Timerwork*>(UINTPTR_MAX));
	auto end_iter = __container_for_timer.lower_bound(current_sentry);

	std::vector<WorkType> out;
	std::copy(__container_for_timer.begin(), end_iter, std::back_inserter(out));
	__container_for_timer.erase(__container_for_timer.begin(), end_iter);

	for (const auto &item : out) {
		size_t n = __container_for_locaiton.erase({ item.second,item.second->getSequence() });
		assert(1 == n); (void)n;
	}

	return out;
}

//brief；重复的定时任务重新放回
//     可能会修改下一次的超时唤醒时间
void TimerworkQueue::__reset(const std::vector<TimerworkQueue::WorkType>& expired_work, CTimestamp now) {
	assert(__container_for_locaiton.size() == __container_for_timer.size());
	
	for (const auto& item : expired_work) {
		if (item.second->needRepeat()
			&& __container_for_self_cancel.find({ item.second,item.second->getSequence() }) == __container_for_self_cancel.end()) {
			//确保任务要求重复并且没有自我注销
			item.second->reset(now);
			__insert(item.second);
		}
		else {
			delete item.second;
		}
	}

	CTimestamp next_expired_time;
	if (!__container_for_timer.empty()) {
		next_expired_time = __container_for_timer.begin()->first;
	}

	if (next_expired_time.isValid()) {
		//如果时间有效，则重新设置下次超时时间
		detail::settime(__timerfd, next_expired_time);
	}
}

//brief:添加任务
//becare:保证在Eventloop所在的IO线程中运行本任务，不存在线程竞争，因此是安全的
//return:true for need settime();false for not
bool TimerworkQueue::__insert(Timerwork* timer_work) {
	__loop->assertInLoopThread();

	//是否重新设置超时
	CTimestamp new_time = timer_work->getExpiration();
	//bool need_settime = new_time < __container_for_timer.begin()->first ? true : false;
	bool need_settime = __container_for_timer.empty() || new_time < __container_for_timer.begin()->first
		                ? true : false;

	//添加任务
	auto ans1 = __container_for_timer.insert({ new_time,timer_work });
	auto ans2 = __container_for_locaiton.insert({ timer_work,timer_work->getSequence() });
	
	if (!ans1.second || !ans2.second) {
		//针对重复添加的同一任务
		LOG_WARN << "attempt to insert a timerwork:" << timer_work->getSequence() << " that has been already added";
	}

	assert(__container_for_locaiton.size() == __container_for_timer.size());
	return need_settime;
}



}//!namespace core


}//!namespace imnet

