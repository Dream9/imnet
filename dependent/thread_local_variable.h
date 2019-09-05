/*************************************************************************
	> File Name: thread_local_variable.h
	> Author: Dream9
	> Brief: 定义几个线程声明周期的变量，用于缓存部分数据
	> Created Time: 2019年08月20日 星期二 09时11分06秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_THREAD_LOCAL_VARIABLE_H_
#define _IMNET_DEPENDENT_THREAD_LOCAL_VARIABLE_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"types_extension.h"
#endif

namespace imnet {
namespace thread_local_variable {

//brief:分别记录线程tid,以及外部指定的线程名称
extern thread_local pid_t tl_tid;
//extern thread_local string tl_tid_string;
extern thread_local string tl_thread_name;

void thread_local_variable_initialization();
bool isMainThread();
string backTrace(bool needDemangle);

//以下定义获得线程生命周期变量的接口
//brief:获得线程id
//inline int gettid() {
inline pid_t gettid() {
	if (__builtin_expect(tl_tid == 0, 0)) {//采用分支预测优化，只会第一次调用时出错
		thread_local_variable_initialization();
	}
	return tl_tid;
}

//brief:获得线程名称
inline string getname() {
	return tl_thread_name;
}

//brief:对nanosleep的封装
//becare:仅用于部分状态调试使用，正常业务逻辑需要禁止调用
void sleep_for(int64_t usec);
//
}//namepace thread_local_variable
}//namespace imnet


#endif

