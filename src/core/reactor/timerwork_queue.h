/*************************************************************************
	> File Name: timer_work_queue.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 10时39分08秒
 ************************************************************************/
#ifndef _CORE_TIMERWORK_QUEUE_H_
#define _CORE_TIMERWORK_QUEUE_H_ 

#ifdef _MSC_VER
#include"../../dependent/logger/timestamp.h"
#include"channel.h"
#include"timerwork.h"
#else
#include"dependent/logger/timestamp.h"
#include"core/reactor/channel.h"
#include"core/reactor/timerwork.h"
#endif

#include<set>
#include<memory.h>
#include<vector>
#include<functional>

namespace imnet{

namespace core {

class Eventloop;
class TimerworkId;

//brief:定时任务管理、
//      持有timerfd,并将事件注册给Eventloop
//      为了快速定位超时的工作，采用set<CTimestamp,Pointer>的形式
//		为了快速定位工作，采用set<Timerwork*,SequenceType>的形式，注意两者表述的数据是一致的
//      添加及删除任务交到Eventloop中去完成，因此只在一个线程中，上述两个数据不存在竞争关系
class TimerworkQueue : noncopyable{
public:
	typedef Timerwork::TimerworkCallbackType TimerworkCallbackType;
	TimerworkQueue(Eventloop *loop);
	~TimerworkQueue();
	
	//外部接口
	TimerworkId add(const TimerworkCallbackType& work,CTimestamp expiration_time,double interval);
	TimerworkId add(TimerworkCallbackType&& work,CTimestamp expiration_time,double interval);
	void cancel(const TimerworkId& timerwork_id);
	void cancel(TimerworkId&& timerwork_id);

private:
	//时间升序排列
	//typedef std::unique_ptr<Timerwork> Pointer;
	typedef Timerwork* Pointer;
	typedef std::pair<CTimestamp, Pointer> WorkType;
	typedef std::set<WorkType> TimeSortListType;

	//指针升序排列
	typedef Timerwork::SequenceType SequenceType;
	typedef std::pair<Timerwork*, SequenceType> LocationType;
	typedef std::set<LocationType> TimerworkSortListType;

	//timerfd的ReadCallback
	void __handleRead();

	//将此工作绑定到IO线程中
	void __cancelTimerworkInLoop(const TimerworkId& timerwork_id);
	void __addTimerworkInLoop(Timerwork* timer_work);

	bool __insert(Timerwork* timerwork);
	std::vector<WorkType> __getExpired(CTimestamp now);
	void __reset(const std::vector<WorkType>& expired_work, CTimestamp now);

	Eventloop* __loop;
	const int __timerfd;
	Channel __channel_of_timerfd;

	TimeSortListType __container_for_timer;
	TimerworkSortListType __container_for_locaiton;

	bool __is_processing_expired_timers;
	TimerworkSortListType __container_for_self_cancel;
};

}//!namespace core
}//!namspace imnet


#endif
