/*************************************************************************
	> File Name: eventloop_thread_pool.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月09日 星期三 09时03分18秒
 ************************************************************************/
#ifdef _MSC_VER
#include"eventloop.h"
#include"eventloop_thread.h"
#include"eventloop_thread_pool.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/reactor/eventloop.h"
#include"core/reactor/eventloop_thread.h"
#include"core/reactor/eventloop_thread_pool.h"
#include"dependent/logger/logger.h"
#endif

#include<string.h>

namespace imnet {

namespace core {

//brief:
EventloopThreadPool::EventloopThreadPool(Eventloop* loop,const string& name,
	int number_of_thread,EventloopThreadPool::CallbackType call)
	:__baseloop(loop),
	__number_of_thread(number_of_thread),
	__start_state(false),
	__index(0),
	__name(name),
	__call(call){

}

EventloopThreadPool::~EventloopThreadPool() = default;

//brief:启动IO线程池
//becare:只在__baseloop线程中进行初始化，避免竞争
void EventloopThreadPool::start() {
	assert(!__start_state);
	__baseloop->assertInLoopThread();

	__eventloopthread_list.reserve(__number_of_thread);
	__eventloop_list.reserve(__number_of_thread);

	LOG_TRACE << __number_of_thread << "io threads will be created";
	//在控制线程中初始化各个工作线程，并保持他们的句柄(通过unique_ptr)
	for (int i = 0; i < __number_of_thread; ++i) {
		char buf[32];
		snprintf(buf, sizeof buf, "%s%d", __name.c_str(), i);
		//EventloopThread* cur = new EventloopThread(__call, __name + std::to_string(i));
		EventloopThread* cur = new EventloopThread(__call, buf);
		__eventloopthread_list.emplace_back(std::unique_ptr<EventloopThread>(cur));
		__eventloop_list.emplace_back(cur->startLoop());
	}

	//针对没有工作线程只有控制线程序的情况
	if (__number_of_thread == 0 && __call) {
		__call(__baseloop);
	}

	__start_state = true;
}

//brief:从IO线程池获得一个服务线程
//becare:只能在控制线程(__baseloop)中选择工作线程,避免竞争
Eventloop* EventloopThreadPool::getNextEventloop() {
	__baseloop->assertInLoopThread();
	assert(__start_state);

	//控制线程即为工作线程
	if (__number_of_thread == 0)
		return __baseloop;

	if (static_cast<size_t>(__index) >= __eventloop_list.size()) {
		__index = 0;
	}

#ifdef IMNET_EVENTLOOPTHREAD_USE_RAW_POINTER
	return __eventloop_list[__index++];
#else
	//brief:应对比较极端的情况，当某个io线程发生了意外下线的情况
	//      此时会清除相关句柄，减少__number_of_thread,然后递归到下次调用
	//      最极端情况下，吧__baseloop返回
	std::shared_ptr<Eventloop> io_loop(__eventloop_list[__index].lock());
	if (io_loop) {
		//becare:尝试性锁住没有问题，但不代表运行时绝对安全
		++__index;
		return io_loop.get();
	}
	else {
		//交换到尾部释放，递归下去
		swap(__eventloop_list[__index], __eventloop_list.back());
		swap(__eventloopthread_list[__index], __eventloopthread_list.back());
		__eventloop_list.pop_back();
		__eventloopthread_list.pop_back();
		--__number_of_thread;
		//++__index;//不须，已经发生了交换
		return getNextEventloop();
	}
#endif
}

}//!namespace core
}//!namespace imnet
