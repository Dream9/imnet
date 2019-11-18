/*************************************************************************
	> File Name: eventloop_thread.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月09日 星期三 09时01分43秒
 ************************************************************************/
#ifdef _MSC_VER
#include"eventloop_thread.h"
#include"eventloop.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/reactor/eventloop.h"
#include"core/reactor/eventloop_thread.h"
#include"dependent/logger/logger.h"
#endif

#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
#pragma message("EventloopThread will use raw pointer to record Eventloop object")
#else
#pragma message("EventloopThread will use shared_ptr/weak_ptr to record Eventloop object")
#endif

namespace imnet {

namespace core {

//brief:初始化线程、condition变量
EventloopThread::EventloopThread(const EventloopThread::CallbackFunctionType& call,const std::string& name)
	:__loop(nullptr),
	__thread_(std::bind(&EventloopThread::__threadFunction,this),name),
	__mutex(),
	__cond(__mutex),
	__call(call){
}

//becare:根据one loop per thread模型，当对象析构时，该线程也应被终结
//      编译时如果定义了宏IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER时，startLoop传出的是野生指针，
//      此时需要用户自行保证线程终结后，不再使用之，否则默认传出weak_ptr,每次使用时需要通过lock保证不会发生段错误
EventloopThread::~EventloopThread() {
	if (__loop) {
		//warning:注意当此对象销毁后，通过startLoop()得到的Eventloop*将处于不可用状态！
		//TODO:可以将__loop通过shared_ptr记录，startLoop()传出一个weak_ptr，外部使用时，尝试性lock之
#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
		LOG_TRACE << "Out of scope, Eventloop="<< __loop <<" will be destroyed soon.";
#else
		LOG_TRACE << "Out of scope, Eventloop="<< __loop.get() <<" will be destroyed soon.";
#endif
		//结束IO循环，等待线程终止
		__loop->quit();
		__thread_.join();
	}
}

//brief:启动Eventloop线程，开始loop
//return:启动的Eventloop的句柄
//becare:本对象的Eventloop生命期与IO循环、线程生命期一致，当线程结束后，本对象将处于
//      不可恢复状态
#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
Eventloop* EventloopThread::startLoop() {
#else
std::weak_ptr<Eventloop> EventloopThread::startLoop(){
#endif
	assert(!__thread_.isStarted());

	__thread_.start();
	
	{
		//需要等待线程初始化完毕
		MutexUnique_lock tmplock(__mutex);
		while (!__loop) {
			__cond.wait();
		}
	}

	return __loop;
}

//brief:线程执行任务，主要负责初始化Eventloop，然后启动循环，等待结束
void EventloopThread::__threadFunction() {
#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
	Eventloop ioloop;

	{
		MutexUnique_lock tmplock(__mutex);
		__loop = &ioloop;
		__cond.notify();
	}

	if (__call) {
		__call(__loop);
	}
#else

	{
		MutexUnique_lock tmplock(__mutex);
		__loop.reset(new Eventloop);
		__cond.notify();
	}
	if (__call) {
		__call(__loop.get());
	}
#endif

	__loop->loop();

	//其实没有必要，因为线程生命已经被外部终结，
	//如需重启该任务，只能重新产生新的IO线程及其配套的Eventloop
	MutexUnique_lock tmplock(__mutex);
	__loop = nullptr;
}

}//!namesapce core
}//!namespace imnet
