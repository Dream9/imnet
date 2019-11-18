/*************************************************************************
	> File Name: timerwork.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月27日 星期五 15时18分27秒
 ************************************************************************/

#ifdef _MSC_VER
#include"./timerwork.h"
#else
#include"core/reactor/timerwork.h"
#endif

imnet::AtomicInt64 imnet::core::Timerwork::__global_sequence;

//brief:重启任务时，重设超时时间
//becare:不重复的任务，时间将会无效
void imnet::core::Timerwork::reset(CTimestamp now) {
	__expiration_time = __repeat ? timestampAdd(now, __interval) : CTimestamp::getInvalid();
}
