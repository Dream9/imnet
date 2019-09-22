/*************************************************************************
	> File Name: log_stream.h
	> Author: Dream9
	> Brief: 封装的流式输出的类
	> Created Time: 2019年08月14日 星期三 09时40分14秒
 ************************************************************************/

#ifndef _IMNET_DEPENDENT_LOG_STREAM_H_
#define _IMNET_DEPENDENT_LOG_STREAM_H_

#ifdef __GNUC__
#include"dependent/noncopyable.h"
#include"dependent/string_piece.h"
#include"dependent/types_extension.h"
#elif defined(_MSC_VER)
#include"../noncopyable.h"
#include"../string_piece.h"
#include"../types_extension.h"
#endif
namespace imnet {

namespace detail {
const int kSmallBuffer = 4000;
const int kLargeBuffer = kSmallBuffer * 1000;
const int kFmtBuffer = 32;

//brief:CLogStream的底层buffer，固定大小
template<int SIZE>
class CFixedBuffer {
public:
	//brief:初始化指针
	CFixedBuffer() :__ptrCur(__data) {
		setCookie(cookieStart);
	}

	//brief:析构并注册回调函数
	~CFixedBuffer() {
		setCookie(cookieEnd);
	}

	//brief:缓冲区剩余大小
	int getUnusedSize()const {
		return static_cast<int>(end() - __ptrCur);
	}

	//brief:得到缓冲区起始数据
	const char* data()const {
		return __data;
	}

	//brief:得到已使用缓冲区大小
	int size()const {
		return static_cast<int>(__ptrCur - __data);
	}

	//brief:直接通过__ptrCur追加数据
	//becare:需要配合getUnusedSize()判断是否可写，以及add()跳转指针位置
	char* current() {
		return __ptrCur;
	}
	void add(size_t len) {
		__ptrCur += len;
	}

	//brief:自动追加数据的接口
	//becare:当缓冲区不够时，数据将被丢弃
	void append(const char *str, size_t len) {
		if (implicit_cast<size_t>(getUnusedSize())>len) {
			memcpy(__ptrCur, str, len);
			__ptrCur += len;
		}
#ifndef NDEBUG
		else {
			fprintf(stderr,"CFixedBuffer::%s buffer size(%d) is less message size(%d)\n",
					__FUNCTION__,getUnusedSize(),static_cast<int>(len));
		}
#endif
	}

	//brief:清空数据，用于clear 流的状态位
	void reset() {
		__ptrCur = __data;
	}

	//brief:数据置零
	void bzero() {
		memZero(__data, sizeof __data);
	}

	//brief:设置回调函数
	void setCookie(void(*cookie)()) {
		__cookie = cookie;
	}
	
	//brief:测试使用的接口
	string toString() const {
		return string(__data, size());
	}
	CStringPiece toStringPiece()const {
		return CStringPiece(__data, size());
	}

	//brief://////////////
	const char* debugString();

private:
	//brief:返回buffer的end
	//becare:由于本类是对外隐藏的，故不应暴露
	const char* end() const {
		return __data + sizeof __data;
	}

	//brief:提供默认的回调处理
	//becare:当前仅预留出回调的接口，但并没有实现
	static void cookieStart();
	static void cookieEnd();

	//brief:底层数据
	char __data[SIZE];
	char *__ptrCur;
	void(*__cookie)();
};
}//namespace detail
//

//废弃
//brief:辅助类
//function:在不经过拷贝到CStringPiece
//class T_auxiliary {
//public:
//	T_auxiliary(const void *data, int length)
//		:str(reinterpret_cast<const char*>(data)),len(length) {
//		;
//	}
//
//	const char *str;
//	int len;
//};

//brief:流式输出的封装
//      提供一个类型安全的、可扩展的、固定buffer的专用于日志的流式输出类
//      可以为其进行支持格式化的功能扩展
//becare:所谓类型安全，是通过重载，以及全部转换为字符串的形式实现的
class CLogStream :noncopyable {
public:
	typedef detail::CFixedBuffer<detail::kSmallBuffer> BufferType;
	typedef CLogStream SelfType;

	SelfType& operator<<(bool value) {
		__buf.append(value ? "1" : "0", 1);
		return *this;
	}

	//重载流式输出函数
	SelfType& operator<<(short);
	SelfType& operator<<(unsigned short);
	SelfType& operator<<(int);
	SelfType& operator<<(unsigned int);
	SelfType& operator<<(long);
	SelfType& operator<<(unsigned long);
	SelfType& operator<<(long long);
	SelfType& operator<<(unsigned long long);
	SelfType& operator<<(const void*);//打印指针地址
	SelfType& operator<<(double);

	SelfType& operator<<(char v) {
		__buf.append(&v, 1);
		return *this;
	}

	SelfType& operator<<(float value) {
		return operator<<(static_cast<double>(value));
	}

	SelfType& operator<<(const char *str) {
		if (str) {
			__buf.append(str, strlen(str));
		}
#ifdef IMNET_LOG_OUTPUTNULL
		else {
			//becare:空指针仍然输出(null)
			__buf.append("(null)", sizeof "(null)");
		}
#endif
		return *this;
	}

	SelfType& operator<<(const unsigned char *str) {
		return operator<<(reinterpret_cast<const char*>(str));
	}

	SelfType& operator<<(const string &str) {
		__buf.append(str.c_str(), str.size());
		return *this;
	}

	SelfType& operator<<(const CStringPiece &str) {
		__buf.append(str.data(), str.size());
		return *this;
	}

	SelfType& operator<<(const BufferType &value) {
		return operator<<(value.toStringPiece());
	}

	//brief:直接采用append输出数据到缓冲区
	void append(const char *str, int len) {
		__buf.append(str, len);
	}

	//brief:获得缓冲区
	const BufferType& buffer() {
		return __buf;
	}

	//brief:重置buffer,类似stringstream等的clear(),用于重新接收数据
	void clear() {
		__buf.reset();
	}
private:

	void staticCheck();

	template<typename T>
	void appendFormatInteger(T);

	static const int kMaxNumericSize = 32;//number转换为字符串时最大的生成长度
	BufferType __buf;
};

//brief:流式输出的格式化扩展类
//     实现方式为将参数完美转发给snprintf
class Fmt {
public:
	template<typename ...Args>
	Fmt(const char *fmt, Args&&... args);

	//template<typename T>
	//Fmt(const char *fmt, T value);

	const char* data()const {
		return __buf;
	}
	int size()const {
		return __len;
	}

private:

	char __buf[detail::kFmtBuffer];
	int __len;
};

//brief:将Fmt与CLogStream关联起来
inline CLogStream& operator<<(CLogStream &os, const Fmt &fmt) {
	os.append(fmt.data(), fmt.size());
	return os;
}

//brief:提供数字单位转换的功能函数
//     分别通过SI单位或者IEC单位表示
string formatSI(int64_t n);
string formatIEC(int64_t n);

}//namespace imnet



#endif
