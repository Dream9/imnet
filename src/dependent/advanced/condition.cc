/*************************************************************************
	> File Name: condition.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月27日 星期二 09时16分22秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/advanced/condition.h"
#elif defined(_MSC_VER)
#include"condition.h"
#endif

#include<sys/time.h>

using namespace imnet;

//brief:对pthread_cond_timedwait的封装
//parameter:休眠的时间，以秒为单位
//return:将pthread_cond_timedwait调用的结果返回
//becare:需要在外部检查返回值的情况，成功为0，失败返回错误编号，其中ret==ETIMEOUT表示超时
int Condition::timedwait(double wait_seconds) {
	int64_t nanoseconds = static_cast<int64_t>(wait_seconds * 1000 * 1000 * 1000);
	struct timespec absolute_timeout;
	clock_gettime(CLOCK_REALTIME, &absolute_timeout);
	absolute_timeout.tv_sec += static_cast<time_t>((nanoseconds + absolute_timeout.tv_nsec) /( 1000 * 1000 * 1000));
	absolute_timeout.tv_nsec += static_cast<long>((nanoseconds + absolute_timeout.tv_nsec) % (1000 * 1000 * 1000));

	int ret=pthread_cond_timedwait(&__cond, __mutex.getPthread_mutex_tUsedOnlyInternally(), &absolute_timeout);
	__mutex.updateHolderUsedOnlyInternally();
	return ret;
}

