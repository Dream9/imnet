/*************************************************************************
	> File Name: timer_work.h
	> Author: Dream9
	> Brief: 本类提供定时任务的封装
	> Created Time: 2019年09月27日 星期五 15时20分11秒
 ************************************************************************/
#ifndef _CORE_TIMERWORK_H_
#define _CORE_TIMERWORK_H_

#ifdef _MSC_VER
#include"../../dependent/logger/timestamp.h"
#include"../../dependent/advanced/atomic_type.h"
#else
#include"dependent/logger/timestamp.h"
#include"dependent/advanced/atomic_type.h"
#endif

#include<functional>

namespace imnet {

namespace core {

//breif:用于表示定时任务
class Timerwork:noncopyable {
public:
	typedef std::function<void()> TimerworkCallbackType;
	typedef imnet::AtomicInt64::ValueType SequenceType;

	Timerwork(const TimerworkCallbackType&& work,CTimestamp expiration_time,double interval)
		:__work(std::move(work)),
	    __expiration_time(expiration_time),
		__interval(interval),
		__repeat(__interval>0.0),
		__sequence(++Timerwork::__global_sequence){

	}
	
	Timerwork(const TimerworkCallbackType& work,CTimestamp expiration_time,double interval)
		:__work(work),
	    __expiration_time(expiration_time),
		__interval(interval),
		__repeat(__interval>0.0),
		__sequence(++Timerwork::__global_sequence){

	}

	//brief:处理任务
	void start() {
		__work();
	}
	//brief:设置再次启动任务的超时时间
	void reset(CTimestamp now);

	//brief:获得内部的状态
	CTimestamp getExpiration() const { 
		return __expiration_time; 
	}
	bool needRepeat()const {
		return __repeat;
	}
	SequenceType getSequence()const {
		return __sequence;
	}

	static SequenceType getGlobalSequence() {
		return Timerwork::__global_sequence.load();
	}

private:
	static AtomicInt64 __global_sequence;

	const TimerworkCallbackType __work;
	//const CTimestamp __expiration_time;//becare:如果允许重启，将会修改之
	CTimestamp __expiration_time;
	const double __interval;//becare:单位为秒
	const bool __repeat;
	const SequenceType __sequence;
};

}//!namespace core
}//!namespace imnet



#endif
