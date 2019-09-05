/*************************************************************************
	> File Name: exception_extension.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月28日 星期三 14时13分53秒
 ************************************************************************/

#ifdef __GNUC__
#include"dependent/exception_extension.h"
#include"dependent/thread_local_variable.h"
#elif defined(_MSC_VER)
#include"exception_extension.h";
#include"thread_local_variable.h"
#endif

using namespace imnet;

//brief:初始化错误信息和堆栈信息
//parameter:need_demangle:会对名称进行逆向解析
ImnetException::ImnetException(string what, bool need_demangle)
	:__error_info(std::move(what)),
	__backtrace_info(thread_local_variable::backTrace(need_demangle)) {
}
