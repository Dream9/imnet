/*************************************************************************
	> File Name: buffer.cc
	> Author: Dream9
	> Brief:
	> Created Time: 2019年10月15日 星期二 14时22分47秒
 ************************************************************************/
#ifdef _MSC_VER
#include"buffer.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/buffer.h"
#include"dependent/logger/logger.h"
#endif

#include<errno.h>

//散布读
#include<sys/uio.h>

namespace imnet {

//namespace detail{
namespace core{

const int Buffer::kDefaultVectorSize = 1024;//默认缓冲区__buf全部数据大小
const int Buffer::kPrependLength = 8;//起始前缀空间大小

#define PREPEND_FUNCTION(bits) \
inline void Buffer::prependInt##bits(uint##bits##_t data){\
	data = socket_ops::hton64(data);\
	prepend(&data, sizeof data);\
}

PREPEND_FUNCTION(64);
PREPEND_FUNCTION(32);
PREPEND_FUNCTION(16);

#undef PREPEND_FUNCTION


#define APPEND_FUNCTION(bits) \
inline void Buffer::appendInt##bits(uint##bits##_t data) {\
	uint64_t net_data = socket_ops::hton64(data);\
	append(&net_data, sizeof net_data);\
}

APPEND_FUNCTION(64);
APPEND_FUNCTION(32);
APPEND_FUNCTION(16);

#undef APPEND_FUNCTION

#define READ_FUNCTION(bits) \
inline uint##bits##_t Buffer::readInt##bits() {\
	uint##bits##_t out = peakInt##bits();\
	retrieve(sizeof out);\
	return out;\
}

READ_FUNCTION(64);
READ_FUNCTION(32);
READ_FUNCTION(16);
READ_FUNCTION(8);

#undef READ_FUNCTION

#define PEAK_FUNCTION(bits) \
inline uint##bits##_t Buffer::peakInt##bits()const {\
	assert(getDataSize() >= sizeof(uint##bits##_t));\
	uint##bits##_t out;\
	::memcpy(&out, peak(), sizeof out);\
	return socket_ops::ntohl(out);\
}

//PEAK_FUNCTION(64);//已在头文件中实现，主要是带有一些说明，故没有统一使用该形式实现
PEAK_FUNCTION(32);
PEAK_FUNCTION(16);
PEAK_FUNCTION(8);

#undef PEAK_FUNCTION


//关于散布读readv
//struct iovec {
//	void  *iov_base;    /* Starting address */
//	size_t iov_len;     /* Number of bytes to transfer */
//};

//On success, readv(), preadv() and preadv2() return the number of
//bytes read; writev(), pwritev() and pwritev2() return the number of
//bytes written.
//
//Note that it is not an error for a successful call to transfer fewer
//bytes than requested(see read(2) and write(2)).
//
//On error, -1 is returned, and errno is set appropriately.

//brief:本函数采用散布读的方式，同时使用栈上空间作为一个可伸缩的备用缓冲区。
//     主要是应对__buf空间不足的情况，这是一种在效率和空间方面的妥协：一方面
//     __buf初始大小将会受限于最大并发数，不宜过大，另一方面希望尽量减少read的
//     系统调用次数。
//becare:本系统采用水平触发，因此无论如何只read一次
//      本函数不做错误处理（包括不处理EOF），只将错误信息传出
ssize_t Buffer::readFrom(int fd, int* saved_errno) {
	char stackbuf[65536];
	struct iovec arr[2];
	size_t idle = getIdleSize();

	arr[0].iov_base = beginWrite();
	arr[0].iov_len = idle;

	arr[1].iov_base = stackbuf;
	arr[1].iov_len = sizeof stackbuf;

	ssize_t n = readv(fd, arr, 2);

	if (n < 0) {
		//sys error
		//LOG_SYSERR<<
		//将错误信息传出即可
		if (saved_errno)
			*saved_errno = errno;
	}
	//else if(n==0)不处理EOF
	else if (static_cast<size_t>(n) <= idle) {
		//只使用了自身缓冲区
		__write_index += static_cast<size_t>(n);
	}
	else {
		//使用了栈上缓冲区
		__write_index += static_cast<size_t>(n);
		append(stackbuf, static_cast<size_t>(n) - idle);
	}

	return n;
}


}//!namesapce core 
}//!namesapce imnet
