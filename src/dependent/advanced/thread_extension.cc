/*************************************************************************
	> File Name: thread_extension.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月28日 星期三 14时29分13秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/advanced/thread_extension.h"
#include"dependent/thread_local_variable.h"
#include"dependent/logger/logger.h"
#include"dependent/exception_extension.h"
#elif defined(_MSC_VER)
#include"thread_extension.h"
#include"../thread_local_variable.h"
#include"../logger/logger.h"
#include"../exception_extension.h"
#endif


#include<sys/prctl.h>


namespace imnet {

//brief:对外隐藏
namespace detail {

//brief:对Thread调用task的内部封装，主要为了修改相应的状态值，然后转调task function
struct AuxiliaryThreadTask {
	using ThreadFunctionType = imnet::Thread::ThreadFunctionType;
	using SelfType = AuxiliaryThreadTask;

	ThreadFunctionType task;
	const string* ptr_name;
	pid_t* ptr_tid;
	CountdownLatch* ptr_latch;

	AuxiliaryThreadTask(const ThreadFunctionType& func,const string* name, pid_t* ptid,CountdownLatch* latch)
		:task(func),ptr_name(name),ptr_tid(ptid),ptr_latch(latch){
	}
	//becare:当传入临时性质的func(或者通过std::move强制转换到右值)调用，对于后者，务必保证不在未重新初始化后就再次使用
	AuxiliaryThreadTask(ThreadFunctionType&& func,const string* name, pid_t* ptid,CountdownLatch* latch)
		:task(std::move(func)),ptr_name(name),ptr_tid(ptid),ptr_latch(latch){
	}


	//brief:修改线程有关的属性，并进行task的调度
	void schedule() {
		//更改tid,唤醒调用线程
		*ptr_tid = imnet::thread_local_variable::gettid();
		ptr_latch->countDown();
		ptr_tid = nullptr;
		ptr_latch = nullptr;

		//更改名称
		imnet::thread_local_variable::tl_thread_name = ptr_name->empty() ? "IMNET Threead" : ptr_name->c_str();
		::prctl(PR_SET_NAME, imnet::thread_local_variable::tl_thread_name);

		//task调度以及异常捕捉
		try {
			task();
			imnet::thread_local_variable::tl_thread_name = "finished";
		}
		catch (const ImnetException& e) {
			imnet::thread_local_variable::tl_thread_name = "crashed";
			fprintf(stderr, "Thread:%s is crashed\n", ptr_name->c_str());
			fprintf(stderr, "reason:%s\n", e.what());
			fprintf(stderr, "stackTrace:%s\n", e.backtrace());
			abort();
		}
		catch (const std::exception& e) {
			imnet::thread_local_variable::tl_thread_name = "crashed";
			fprintf(stderr, "Thread:%s crashed\n", ptr_name->c_str());
			fprintf(stderr, "reason:%s\n", e.what());
			abort();
		}
		catch (...) {
			imnet::thread_local_variable::tl_thread_name = "crashed";
			fprintf(stderr, "Thread:%s is crashed by unknown reason\n", ptr_name->c_str());
			throw;
		}
	}
};

//brief:外部的调用例程,为了适应void*(void*)的ptread_create的接口
void* startSchedule(void* data) {
	reinterpret_cast<AuxiliaryThreadTask*>(data)->schedule();
	return NULL;
}

}//!namespace detail
AtomicInt32 Thread::__thread_count;

//brief:常规版本的初始化
Thread::Thread(const ThreadFunctionType& function,const string &thread_name)
	:__stared(false),__joined(false),__task(function),__ptid(),__tid(0),__name(thread_name),
    __countdown_latch(1){
	__initilization();
}

//brief:右值移动版本的初始化
Thread::Thread(ThreadFunctionType&& function,const string &thread_name)
	:__stared(false),__joined(false),__task(std::move(function)),__ptid(),__tid(0),__name(thread_name),
    __countdown_latch(1){
	__initilization();
}

//brief:完成初始化任务
//     对于空的线程名予以自动名字
void Thread::__initilization() {
	AtomicDataType count_now = ++__thread_count;
	if (__name.empty()) {
		//__name = "Thread" + std::to_string(count_now);
		char buf[32];
		snprintf(buf, sizeof buf, "Thread%d", count_now);
		__name.assign(buf);
	}
}

//brief:解决线程生命期的冲突
Thread::~Thread() {
	if (__stared && !__joined) {
#ifndef _THREADTASK_AUTO_DETACH
		std::terminate();
#else
		pthread_detach(__ptid);
#endif
	}
}

//brief:启动任务，并初始化线程信息
void Thread::start() {
	assert(__stared == false);
	__stared = true;

	detail::AuxiliaryThreadTask att(std::move(__task), &__name, &__tid, &__countdown_latch);
	if (pthread_create(&__ptid, NULL, &detail::startSchedule, &att)) {
		//pthread_create调用失败
		__stared = false;
		LOG_SYSFATAL << "pthread_create() failed";
	}
	else {
		//等待子线程设置tid结束，防止竞态
		__countdown_latch.wait();
		assert(__tid > 0);
	}
}

//brief:关于detach是否作为公共接口
//     c++11thread的做法是提供给用户，那么用户有责任在thread生命周期之前调用detach或者join
//     这里还提供另一种思路，即隐藏detach,那么用户不在需要强制调用detach或者join
#ifndef _THREADTASK_AUTO_DETACH
void Thread::detach() {
	//if (!__stared || __joined) {
	//	LOG_ERROR << "thread is not stared or it has benn joined.";
	//	return;
	//}
	assert(__stared == true);
	assert(__joined == false);

	__joined = true;
	if (pthread_detach(__ptid)) {
		//出错例程
		__joined = false;
		LOG_SYSERR << "pthread_detach() failed";
	}
}
#endif

//brief:是否仍与某个线程关联
bool Thread::joinable() {
	return __stared && !__joined;
}

void Thread::join() {
	assert(__stared);
	assert(__joined == false);
	__joined = true;
	if (pthread_join(__ptid, NULL)) {
		//出错例程
		__joined = false;
		LOG_SYSERR << "pthread_join() failed";
	}
}

//brief:对外隐藏
namespace detaile {

//brief:fork之后子线程的初始化回调函数
void onChildForkCallback() {
	imnet::thread_local_variable::tl_thread_name = "main";
	imnet::thread_local_variable::tl_tid = 0;
	imnet::thread_local_variable::thread_local_variable_initialization();
}

//brief:主线程构造时负责注册回调函数，保证fork之后的子线程有统一的性质
class ThreadAttributeAutomaticSet {
public:
	ThreadAttributeAutomaticSet(){
		imnet::thread_local_variable::tl_thread_name = "main";
		imnet::thread_local_variable::thread_local_variable_initialization();
		pthread_atfork(NULL, NULL, &onChildForkCallback);
	}
};

ThreadAttributeAutomaticSet auto_init_thread_attribute;
}//!namespace detaile

}//!namespace imnet








