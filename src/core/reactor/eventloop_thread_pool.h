/*************************************************************************
	> File Name: eventloop_thread_pool.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月09日 星期三 09时03分14秒
 ************************************************************************/

#ifndef _CORE_EVENTLOOP_THREAD_POOL_H_
#define _CORE_EVENTLOOP_THREAD_POOL_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
#else
#include"dependent/noncopyable.h"
#endif

#include<vector>
#include<functional>
#include<string>
#include<memory>

namespace imnet {

namespace core {

class Eventloop;
class EventloopThread;

//brief:提供线程池的封装，有一个起控制作用的__baseloop及提供服务能力的其他多个loop数组组成
//     本类的一种使用是作为TcpServe的一员
class EventloopThreadPool :noncopyable {
public:
	typedef std::function<void(Eventloop*)> CallbackType;

	EventloopThreadPool(Eventloop* loop, const string& name, 
		int number_of_thread = 0, CallbackType call = CallbackType());
	~EventloopThreadPool();

	void start();

	Eventloop* getNextEventloop();
	void setThreadNumber(size_t number) {
		__number_of_thread = number;
	}
	void setThreadInitCallback(const CallbackType& call) {
		__call = call;
	}

private:
	Eventloop* __baseloop;
	int __number_of_thread;
	bool __start_state;
	int __index;
	std::string __name;
	EventloopThreadPool::CallbackType __call;

#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
	std::vector<Eventloop*> __eventloop_list;
#else
	std::vector<std::weak_ptr<Eventloop>>__eventloop_list;
#endif

	std::vector<std::unique_ptr<EventloopThread>> __eventloopthread_list;
};
}
}


#endif

