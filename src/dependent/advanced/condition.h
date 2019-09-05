/*************************************************************************
	> File Name: condition.h
	> Author: Dream9
	> Brief:针对pthread_cond_t的封装 
	> Created Time: 2019年08月27日 星期二 09时16分14秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_CONDITION_H_
#define _IMNET_DEPENDENT_CONDITION_H_

#ifdef __GNUC__
#include"dependent/advanced/mutexlock.h"
#elif defined(_MSC_VER)
#include"mutexlock.h"
#endif

#include<pthread.h>

namespace imnet {

//brief:对pthread_cond_t的封装
//function:提供了pthread_cond_t初始化、销毁以及wait和notify/notify_all
//becare:条件变量的标准使用方式：
//       a:加锁，进入循环判断条件，wait//注意前两个步骤都是由用户负责完成
//       b:加锁，业务逻辑，解锁，notify
class Condition :noncopyable {
public:
	explicit Condition(Mutex &m) :__mutex(m) {
		CHECK_PTHREAD_RETURN(pthread_cond_init(&__cond, NULL));
	}
	~Condition() {
		CHECK_PTHREAD_RETURN(pthread_cond_destroy(&__cond));
	}
	//brief:pthread_cond_wait返回时mutex会重新持有，因此更改持有者信息是安全的
	//becare:此时getPthread_mutex_t()返回的是一个已经上锁的mutex指针，这一点必须由用户层保证
	void wait() {
		CHECK_PTHREAD_RETURN(pthread_cond_wait(&__cond, __mutex.getPthread_mutex_tUsedOnlyInternally()));
		__mutex.updateHolderUsedOnlyInternally();//作用是保证持有者信息不变
	}
	void notify() {
		CHECK_PTHREAD_RETURN(pthread_cond_signal(&__cond));
	}
	void notifyall() {
		CHECK_PTHREAD_RETURN(pthread_cond_broadcast(&__cond));
	}

	int timedwait(double wait_seconds);
private:
	Mutex &__mutex;
	pthread_cond_t __cond;
};



}//!namespace imnet



#endif
//

