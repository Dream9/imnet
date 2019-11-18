/*************************************************************************
	> File Name: buffer.cc
	> Author: Dream9
	> Brief:
	> Created Time: 2019��10��15�� ���ڶ� 14ʱ22��47��
 ************************************************************************/
#ifdef _MSC_VER
#include"buffer.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/buffer.h"
#include"dependent/logger/logger.h"
#endif

#include<errno.h>

//ɢ����
#include<sys/uio.h>

namespace imnet {

//namespace detail{
namespace core{

const int Buffer::kDefaultVectorSize = 1024;//Ĭ�ϻ�����__bufȫ�����ݴ�С
const int Buffer::kPrependLength = 8;//��ʼǰ׺�ռ��С

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

//PEAK_FUNCTION(64);//����ͷ�ļ���ʵ�֣���Ҫ�Ǵ���һЩ˵������û��ͳһʹ�ø���ʽʵ��
PEAK_FUNCTION(32);
PEAK_FUNCTION(16);
PEAK_FUNCTION(8);

#undef PEAK_FUNCTION


//����ɢ����readv
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

//brief:����������ɢ�����ķ�ʽ��ͬʱʹ��ջ�Ͽռ���Ϊһ���������ı��û�������
//     ��Ҫ��Ӧ��__buf�ռ䲻������������һ����Ч�ʺͿռ䷽�����Э��һ����
//     __buf��ʼ��С������������󲢷��������˹�����һ����ϣ����������read��
//     ϵͳ���ô�����
//becare:��ϵͳ����ˮƽ����������������ֻreadһ��
//      ��������������������������EOF����ֻ��������Ϣ����
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
		//��������Ϣ��������
		if (saved_errno)
			*saved_errno = errno;
	}
	//else if(n==0)������EOF
	else if (static_cast<size_t>(n) <= idle) {
		//ֻʹ������������
		__write_index += static_cast<size_t>(n);
	}
	else {
		//ʹ����ջ�ϻ�����
		__write_index += static_cast<size_t>(n);
		append(stackbuf, static_cast<size_t>(n) - idle);
	}

	return n;
}


}//!namesapce core 
}//!namesapce imnet
