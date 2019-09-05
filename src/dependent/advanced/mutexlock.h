/*************************************************************************
	> File Name: mutexlock.h
	> Author: Dream9
	> Brief: 对pthread_mutex_t的封装
	> Created Time: 2019年08月27日 星期二 09时14分59秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_MUTEX_LOCK_H_
#define _IMNET_DEPENDENT_MUTEX_LOCK_H_

#ifdef __GNUC__
#include"dependent/noncopyable.h"
#include"dependent/thread_local_variable.h"
#elif defined(_MSC_VER)
#include"../noncopyable.h"
#include"../thread_local_variable.h"
#endif

#include<assert.h>

#include<pthread.h>

//brief:检查POSIX 返回值
//becare:默认情况下开启检测
#ifndef NOT_CHECK_PTHREAD_RETURN_VALUE
#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int _errno, const char *file, unsigned int line,
	const char *function_name) noexcept __attribute((__noreturn__));
__END_DECLS
#define CHECK_PTHREAD_RETURN(ret) \
        ({decltype(ret) _errno=ret;\
		 if(__builtin_except(_errno,0))\
			___assert_perror_fail(_errno,__FILE__,__LINE__,__FUNCTION__);\
		})
#else
#define CHECK_PTHREAD_RETURN(ret)\
		({decltype(ret) _errno=ret;\
		  assert(_errno==0);\
		  (void)_errno;\
		})
#endif
#else
#define CHECK_PTHREAD_RETURN(ret)

#endif

namespace imnet {

//brief:RAII封装的pthread_mutex_t
//function:pthread_mutex_t的初始化与销毁，pv操作，记录锁的持有者信息
//becare:持有者信息总是在获得锁的状态时进行更改，包括在释放锁之前修改为0
class Mutex:noncopyable {
public:
	//brief:standart routine
	Mutex():__mutex_holder(0){
		CHECK_PTHREAD_RETURN(pthread_mutex_init(&__mutex, NULL));
	}
	~Mutex() {
		assert(__mutex_holder == 0);
		CHECK_PTHREAD_RETURN(pthread_mutex_destroy(&__mutex));
	}
	bool isLockedByThisThread()const {
		return __mutex_holder == thread_local_variable::gettid();
	}

	//becare:以下操作不期望用户直接调用，仅供内部使用
	void lock() {
		CHECK_PTHREAD_RETURN(pthread_mutex_lock(&__mutex));
		__updateHolder();
	}
	void unlock() {
		__updateHolder(0);
		CHECK_PTHREAD_RETURN(pthread_mutex_unlock(&__mutex));
	}
	pthread_mutex_t* getPthread_mutex_tUsedOnlyInternally() {
		return &__mutex;
	}
	void updateHolderUsedOnlyInternally() {
		__updateHolder();
	}
	//becare:以上操作不期望用户直接调用，仅供内部使用

private:
	//brief:更新持有者信息
	void __updateHolder() {
		__mutex_holder = thread_local_variable::gettid();
	}
	void __updateHolder(int i) {
		__mutex_holder = i;
	}
	pthread_mutex_t  __mutex;
	int __mutex_holder;
};


//brief:RAII管理的Mutex
//      实现类似于unique_lock()
//becare:lock_guard()提供更加灵活的锁管理，但需要额外的成本付出
class MutexUnique_lock:noncopyable{
public:
	using MutexType = Mutex;
	explicit MutexUnique_lock(MutexType &m) :__mutex(m) {
		__mutex.lock();
	}
	~MutexUnique_lock() {
		__mutex.unlock();
	}
private:
	MutexType& __mutex;
};

}//namespace imnet

//brief:不允许通过临时匿名变量管理Mutex资源
#define MutexUnique_lock(x) static_assert(false,"Temporary object will die soon(mutex may be not locked!)")

#endif


