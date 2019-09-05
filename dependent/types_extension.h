/*************************************************************************
	> File Name: types.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019年08月13日 星期二 10时30分32秒
 ************************************************************************/

#ifndef _IMNET_DEPENDENT_TYPES_EXTENSION_H_
#define _IMNET_DEPENDENT_TYPES_EXTENSION_H_

#include<cstring>
#include<string>
#include<stdint.h>

#ifndef NDEBUG
#include<assert.h>
#endif

namespace imnet {
using std::string;

//brief:与static_cast相比，进行up_cast的时候可以做安全检查
template<typename To, typename From>
inline To implicit_cast(const From &value) {
	return value;
}

//brief:debug时采用RTTI进行类型检查，release时使用static_cast转换
//becare:主要用于down_cast
template<typename To, typename From>
inline To down_cast(const From &value) {
#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
	assert(value == nullptr || dynamic_cast<From>(value) != nullptr);
#endif
	return static_cast<To>(value);
}

//brief:内存置零
//becare:n代表字节数
inline void memZero(void *p, size_t n) {
	memset(p, 0, n);
}


}


#endif
