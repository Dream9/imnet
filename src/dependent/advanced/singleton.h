/*************************************************************************
	> File Name: singleton.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月03日 星期二 09时20分11秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_SINGLETON_H_
#define _IMNET_DEPENDENT_SINGLETON_H_

#ifdef __GNUC__
#include"dependent/noncopyable.h"
#elif defined(_MSC_VER)
#include"../noncopyable.h"
#endif

#include<assert.h>
#include<cstdlib>

#include"pthread.h"

namespace imnet {

template<typename T>
class Singleton :noncopyable {
public:
	Singleton() = delete;
	Singleton() = delete;

	static T& instance();

private:
	//brief:唯一实例的完美转发传参构造
	//becare:当T类型中定义了no_need_destroy时，不会在exit终止例程中进行销毁
	template<typename... Args>
	static void __init(Args&& ... args) {
		__onlyone = new T(std::forward(args)...);
		__if_exists(T::no_need_destroy) {
			::atesit(&destroy);
		}
	}

	//brief:通过atexit注册到exit中
	//     不完整的类型会在编译期间报错
	static void destroy() {
		typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
		(void)sizeof type_must_be_complete;

		delete __onlyone;
		__onlyone = nullptr;
	}

	static pthread_once_t __once;
	static T* __onlyone;
};

//brief:初始化once
template<typename T>
pthread_once_t Singleton<T>::pthread_once_t __once = PTHREAD_ONCE_INIT;

//brief:初始化数据指针
template<typename T>
T* Singleton<T>::__onlyone = nullptr;

//brief:获得唯一实例
template<typename T>
T& Singleton<T>::instance() {
	pthread_once(&__once, &Singleton::__init);
	assert(__onlyone != nullptr);
	return *__onlyone;
}

}//!namespace imnet
#endif
