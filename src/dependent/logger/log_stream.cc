/*************************************************************************
	> File Name: log_stream.cpp
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月16日 星期五 09时38分22秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/logger/log_stream.h"
#elif defined(_MSC_VER)
#include"log_stream.h"
#endif

#include<algorithm>
#include<limits>
#include<assert.h>
#include<inttypes.h>

using namespace imnet;
using namespace imnet::detail;


//-Wtype - limits:
//Warn if a comparison is always true or always false 
//due to the limited range of the data type, but do not
//warn for constant expressions.For example, warn if an
//unsigned variable is compared against zero with ‘ < ’
//or ‘ >= ’.This warning is also enabled by - Wextra.
//
//忽略这项warning
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wtautological-compare"
#else
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace imnet {
namespace detail {

//brief:itoa的通用实现

template<typename T>
size_t convert(char buf[], T value, int radix) {
	assert(radix >= 2 && radix <= 36);
	int sign = (radix == 0 && value < 0) ? 1 : 0;//只有十进制才有正负之分

	char *ptr = buf;
	uint64_t i = sign ? -static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
}

//brief:方便进制转换
//     也可以通过 i+'A'-10或者i+'0'的形式来节省空间
const char digits_for_convert_10radix[] = "9876543210123456789";
const char * const ptr_digits_zero = digits_for_convert_10radix + 9;
static_assert(sizeof(digits_for_convert_10radix) == 20,
	"bad digits for convert 10 radix");
const char digits_for_convert_16randix[] = "0123456789ABCDEF";
static_assert(sizeof(digits_for_convert_16randix) == 17,
	"bad digigts for convert 16 radix");

//brief:十进制特化,转化为字符串
//becare:buf空间由外部保证足够大
template<typename T>
size_t convert(char buf[], T value) {
	assert(*ptr_digits_zero == '0');
	T i = value;
	char *ptr = buf;

	do{
		int pos = static_cast<int>(i % 10);
		i /= 10;
		*(ptr++) = ptr_digits_zero[pos];
	} while (i);

	if (value < 0)
		*(ptr++) = '-';
	*ptr = '\0';
	std::reverse(buf, ptr);
	
	return ptr - buf;
}

//brief:十六进制特化,转化为字符串
//     主要是为了输出指针地址时使用
//     因此类型采用了uintptr_t
size_t convertHex(char buf[], uintptr_t value) {
	uintptr_t i = value;
	char *ptr = buf;

	do {
		int pos = static_cast<int>(i % 16);
		i /= 16;
		*ptr++ = digits_for_convert_16randix[pos];
	} while (i);

	*ptr = '\0';
	std::reverse(buf, ptr);
	return ptr - buf;
}

//显示实例化代码
template class CFixedBuffer<kLargeBuffer>;
template class CFixedBuffer<kSmallBuffer>;
}//namespace detail

//todo
//brief:
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
string formatSI(int64_t value) {
	;
	return "";
}
string formatIEC(int64_t value) {
	;
	return "";
}
#undef __STDC_FORMAT_MACROS

}//namespace imnet
/////////////////////////////////////////////
/////////////////////////////////////////////
//具体的定义实现
/////////////////////////////////////////////
/////////////////////////////////////////////
//brief:仅用于方便debug时查看buffer的内容
template<int SIZE>
const char* CFixedBuffer<SIZE>::debugString(){
	*__ptrCur = '\0';
	return __data;
}

//brief:默认的两个回调函数
template<int SIZE>
void CFixedBuffer<SIZE>::cookieStart() {
	;
}
template<int SIZE>
void CFixedBuffer<SIZE>::cookieEnd() {
	;
}


//brief:检查预设阈值是否匹配当前平台数字转化为十进制时准确信息的长度
void CLogStream::staticCheck() {
	static_assert(std::numeric_limits<long double>::digits10 <= kMaxNumericSize - 10,
		"CLogStream::kMaxNumericSize should be larger");
	static_assert(std::numeric_limits<long long>::digits10 <= kMaxNumericSize - 10, 
		"CLogStream::kMaxNumericSize should be larger");
}

//brief:重载流运算符的实现
    //SelfType& operator<<(short);
	//SelfType& operator<<(unsigned short);
	//SelfType& operator<<(int);
	//SelfType& operator<<(unsigned int);
	//SelfType& operator<<(long);
	//SelfType& operator<<(unsigned long);
	//SelfType& operator<<(long long);
	//SelfType& operator<<(unsigned long long);
	//SelfType& operator<<(const void*);
	//SelfType& operator<<(double);


CLogStream& CLogStream::operator<<(short value) {
	return operator<<(static_cast<int>(value));
}
CLogStream& CLogStream::operator<<(unsigned short value) {
	return operator<<(static_cast<unsigned int>(value));
}
CLogStream& CLogStream::operator<<(int value) {
	appendFormatInteger(value);
	return *this;
}
CLogStream& CLogStream::operator<<(unsigned int value) {
	appendFormatInteger(value);
	return *this;
}
CLogStream& CLogStream::operator<<(long value) {
	appendFormatInteger(value);
	return *this;
}
CLogStream& CLogStream::operator<<(unsigned long value) {
	appendFormatInteger(value);
	return *this;
}
CLogStream& CLogStream::operator<<(long long value) {
	appendFormatInteger(value);
	return *this;
}
CLogStream& CLogStream::operator<<(unsigned long long value) {
	appendFormatInteger(value);
	return *this;
}
//brief:特例，输出指针地址(转换为Hex)
CLogStream& CLogStream::operator<<(const void  *ptr_value) {
	uintptr_t value = reinterpret_cast<uintptr_t>(ptr_value);
	if (__buf.getUnusedSize() >= kMaxNumericSize) {
		char *buf = __buf.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = convertHex(buf + 2, value);
		__buf.add(len + 2);
	}
	return *this;
}
//brief:浮点型的转化，通过snprintf实现
CLogStream& CLogStream::operator<<(double value) {
	if (__buf.getUnusedSize() >= kMaxNumericSize) {
		int len=snprintf(__buf.current(),kMaxNumericSize,"%.12g",value);
		__buf.add(len);
	}
	return *this;
}

//brief:fmt格式的扩展
//      通过forward转发实现
template<typename...Args>
Fmt::Fmt(const char *fmt, Args&&... args) {
	__len=snprintf(__buf,sizeof __buf,fmt,std::forward<Args>(args)...);
	assert(__len >= 0);//snprintf decode failed
}

//template<typename T>
//Fmt::Fmt(const char *fmt, T v) {
//	__len=snprintf(__buf,sizeof __buf,fmt,v);
//	assert(__len >= 0);//snprintf decode failed
//}

//brief:尽管支持可变参数包，但为了防止代码膨胀，只进行一元参数的显示实例化
//Fmt d("ds", 15);
//Fmt::Fmt(const char *fmt, short);
//Fmt::Fmt(const char *fmt, unsigned short);
//Fmt::Fmt(const char *fmt, int);
//Fmt::Fmt(const char *fmt, unsigned int);
//Fmt::Fmt(const char *fmt, long);
//Fmt::Fmt(const char *fmt, unsigned long);
//Fmt::Fmt(const char *fmt, long long);
//Fmt::Fmt(const char *fmt, unsigned long long);
//Fmt::Fmt(const char *fmt, float);
//Fmt::Fmt(const char *fmt, double);
//Fmt::Fmt(const char *fmt, char);

template<typename T>
void CLogStream::appendFormatInteger(T value) {
	if (__buf.getUnusedSize() >= kMaxNumericSize) {
		size_t len = convert(__buf.current(), value);
		__buf.add(len);
	}
}
