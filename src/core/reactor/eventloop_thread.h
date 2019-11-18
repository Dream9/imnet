/*************************************************************************
	> File Name: eventloop_thread.h
	> Author: Dream9
	> Brief: one-loop per-thread的封装
	> Created Time: 2019年10月09日 星期三 09时01分37秒
 ************************************************************************/
#ifndef _CORE_EVENTLOOP_THREAD_H_
#define _CORE_EVENTLOOP_THREAD_H_

#ifdef _MSC_VER
#include"../../dependent/advanced/condition.h"
#include"../../dependent/advanced/thread_extension.h"
#else
#include"dependent/advanced/condition.h"
#include"dependent/advanced/thread_extension.h"
#endif

#include<functional>

#ifndef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
#include<memory>
#endif
namespace imnet {

namespace core {

class Eventloop;

//brief:产生一个进行loop循环的IO线程
//     本对象具有和线程同等的生命期，同生同死(线程结束后，本对象不在可用)
class EventloopThread :noncopyable{
public:
	typedef std::function<void(Eventloop*)> CallbackFunctionType;

	EventloopThread(const CallbackFunctionType& callback=CallbackFunctionType(),
		const std::string& threadname="" );
	~EventloopThread();

#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
	Eventloop* startLoop();
#else
	std::weak_ptr<Eventloop> startLoop();
#endif

private:
	void __threadFunction();

#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
	Eventloop* __loop;
#else
	std::shared_ptr<Eventloop> __loop;
#endif
	
	//Thread __thread;/为了区别于GCC内部的__thread
	Thread __thread_;
	Mutex __mutex;
	Condition __cond;

	CallbackFunctionType __call;//可选项
};
}//!namespace core
}//!namespace imnet 


#endif
