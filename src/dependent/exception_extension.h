/*************************************************************************
	> File Name: exception_extension.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月28日 星期三 14时13分42秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_EXCEPTION_EXTENSION_H_
#define _IMNET_DEPENDENT_EXCEPTION_EXTENSION_H_

#ifdef __GNUC__
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"types_extension.h"
#endif

#include<exception>

namespace imnet {

class ImnetException :public std::exception {
public:
	ImnetException(string what,bool need_demangle=false);
	~ImnetException() = default;

	const char* what()const noexcept{
		return __error_info.c_str();
	}
	const char* backtrace()const {
		return __backtrace_info.c_str();
	}

private:
	string __error_info;
	string __backtrace_info;
};

}//!namespace imnet

#endif
