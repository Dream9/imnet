/*************************************************************************
	> File Name: thread_extension.h
	> Author: Dream9
	> Brief:对POSIX 线程函数的封装
	> Created Time: 2019年08月28日 星期三 14时29分03秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_THREAD_EXTENSION_H_
#define _IMNET_DEPENDENT_THREAD_EXTENSION_H_
#ifdef __GNUC__
#include"dependent/advanced/atomic_type.h"
#include"dependent/advanced/countdown_latch.h"
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"atomic_type.h"
#include"countdown_latch.h"
#include"../types_extension.h"
#endif

#include<functional>

#include<pthread.h>

namespace imnet {

//brief:对POSIX pthread的封装
//     每个任务(函数)通过bind传递给Thread对象，作为回调函数
//     start()之后将进行ptread_create
//     join()之后进行ptread_join
//     Thread对象生命结束之后会根据情况调用pthread_detach
//     此外负责维护线程有关的状态
//becare:对象语义，不可复制
//     关于Thread对象与pthread生命期的问题：如果定义了_THREADTASK_AUTO_DETACH
//     那么Thread生命结束时自动detach，否则调用terminate
class Thread :noncopyable {
public:
	//brief:作为线程调用函数的抽象
	//becare:其他任意接口可以通过bind调整之
	typedef std::function<void()> ThreadFunctionType;
	explicit Thread(const ThreadFunctionType& func, const string &name = "");
	explicit Thread(ThreadFunctionType&& func, const string &name = "");
	~Thread();

	void start();
	void join();
#ifndef _THREADTASK_AUTO_DETACH
#pragma message("_THREADTASK_AUTO_DETACH is not activited.")
	void detach();
#endif
	bool joinable();

private:
	using AtomicDataType = AtomicInt32::ValueType;
	void __initilization();

	//由于pthread_create需要额外的start()操作，而不是在构造时就完成，导致需要额外变量记录信息
	bool __stared;
	bool __joined;
	ThreadFunctionType __task;
	//becare:pthread_t的特殊实现以及其复用方式，导致其不适合调试使用
	pthread_t __ptid;
	//becare:通过gettid获得的tid，可以通过ps -T等查看状态
	pid_t __tid;
	string __name;
	CountdownLatch __countdown_latch;
	static AtomicInt32 __thread_count;//用于记录已经start(pthread_create)过的线程数目

};
}

#endif

