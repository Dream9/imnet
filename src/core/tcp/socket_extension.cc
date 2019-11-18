/*************************************************************************
	> File Name: socket_extension.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时52分42秒
 ************************************************************************/
#ifdef _MSC_VER
#include"socket_extension.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/socket_extension.h"
#include"dependent/logger/logger.h"
#endif

#include<errno.h>
#ifdef _WIN32
#include<winsock.h>
#include<io.h>
#else
#include<sys/socket.h>
#include<unistd.h>
#endif

#include<sys/uio.h>
#include<fcntl.h>

namespace imnet {

namespace socket_ops {

#define SYSCALL_ERR(x) if((x)<0)\
    {\
		LOG_SYSERR<<__FUNCTION__<<" failed.";\
	}

#define SYSCALL_FATAL(x) if((x)<0)\
	{\
		LOG_SYSFATAL<<__FUNCTION__<<" failed.";\
	}


//将文件描述符设为非阻塞并且写时关闭
void setNonblockingAndCloexec(int fd) {
	int flags = ::fcntl(fd, F_GETFL, NULL);
	flags |= O_NONBLOCK | FD_CLOEXEC;
	int ret = ::fcntl(fd, F_SETFD, flags);
	//assert(ret >= 0); (void)ret;
	SYSCALL_ERR(ret);
}

int createNonblockingTcpSocket(sa_family_t family) {
	//创建指定地址族的遵循tcp传输控制协议的字节流类型的套接字描述符
	//int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
	//if (sockfd < 0) {
	//	LOG_SYSFATAL << __FUNCTION__ << ":failed!";
	//}

	////将其设置为非阻塞模式
	//setNonblockingAndCloexec(sockfd);

	int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	SYSCALL_FATAL(sockfd);
	return sockfd;
}

//brief:bind的封装
//becare:这里采用了兼容ipv6地址的实现，具体参见address_extension.h文件
void bind(int sockfd, const struct sockaddr* addr) {
	int ret = ::bind(sockfd, addr, sizeof(struct sockaddr_in6));
	SYSCALL_FATAL(ret);
}

//brief:listen的封装
void listen(int sockfd) {
	//int listen(int sockfd, int backlog);
	//	The backlog argument defines the maximum length to which the queue of
	//	pending connections for sockfd may grow.If a connection request
	//	arrives when the queue is full, the client may receive an error with
	//	an indication of ECONNREFUSED or , if the underlying protocol supports
	//	retransmission, the request may be ignored so that a later reattempt
	//	at connection succeeds.
	
	//	If the backlog argument is greater than the value in
	//	/ proc / sys / net / core / somaxconn, then it is silently truncated to that
	//	value; the default value in this file is 128.  In kernels before
	//	2.4.25, this limit was a hard coded value, SOMAXCONN, with the value
	//	128.
	int ret = ::listen(sockfd, SOMAXCONN);
	SYSCALL_FATAL(ret);
	LOG_TRACE << "current length of the queue of pending connections is" << SOMAXCONN;
}

//brief:
int accept(int sockfd, struct sockaddr_in6* addr) {
	socklen_t addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in6));

	//The accept4() system call is available starting with Linux 2.6.28;
	//support in glibc is available starting with version 2.10.

	//accept4() is a nonstandard Linux extension.

#if __GNUC__>2 || \
	(__GNUC__==2 && __GNUC__MINOR__>=10) 
	//int out = ::accept4(sockfd, static_cast<sockaddr*>(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	int out = ::accept4(sockfd, static_cast<sockaddr*>((void*)addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
	//int out = ::accept(sockfd, static_cast<struct sockaddr*>(addr), &addrlen);
	int out = ::accept(sockfd, static_cast<struct sockaddr*>((void*)addr), &addrlen);
#endif

	if (out < 0) {
		//处理连接失败
		int saved_errno = errno;//防止被其他错误覆盖
		switch (saved_errno) {
		case EINTR:
		//case EWOULDBLOCK://EWOULDBLOCK和EINTR有着相同的值
		case EAGAIN:
		case ECONNABORTED:
		case EPROTO:
		case EPERM:
		case EMFILE:
			errno = saved_errno;
			//以上错误应当被忽略
			//LOG_SYSERR << __FUNCTION__ << " error.";
			break;
		default:
			LOG_SYSFATAL << __FUNCTION__ << " fatal error.";
		}
	}

#if __GNUC__<2 || \
	(__GNUC__==2 && __GNUC__MINOR__<10) 
	setNonblockingAndCloexec(out);
#endif

	return out;
}

//brief:关闭描述符
void close(int sockfd) {
	//FIXME: EINTR  The close() call was interrupted by a signal; see signal(7). 需要重启？
	SYSCALL_ERR(::close(sockfd));
}

//brief:关闭读端
//     此时对方的读事件POLLIN会被激活
void shutdownWrite(int sockfd) {
	SYSCALL_ERR(::shutdown(sockfd, SHUT_WR));
}

//brief:主要由Connector使用建立连接
//becare:由于底层C不支持重载，更没有多态，因此系统预留的接口中必须显示表明该指针可以访问的实际空间大小
//      此处统一使用最大的sockaddr_in6大小
//      本函数的address应该是通过Sockaddr转换而来的，如果不是则小心内存越界风险
int connect(int sockfd, const struct sockaddr* address) {
	//int length = 0;
	//if (address->sa_family == AF_INET)
	//	length = sizeof(sockaddr_in);
	//else
	//	length = sizeof(sockaddr_in6);
	//int ret = ::connect(sockfd, address, length);
	
	return ::connect(sockfd, address, sizeof(struct sockaddr_in6));
}

//brief:读写的封装
int read(int sockfd, void* buf, size_t len) {
	return ::read(sockfd, buf, len);
}
int readv(int sockfd, const struct iovec* iobuf, int len) {
	return ::readv(sockfd, iobuf, len);
}
int write(int sockfd, const void* buf, size_t len) {
	return ::write(sockfd, buf, len);
}

//brief:获得套接字错误状态
int getErrno(int sockfd) {
	int ret = 0;
	socklen_t length = static_cast<socklen_t>(sizeof ret);

	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &ret, &length) < 0) {
		return errno;
	}
	else {
		return ret;
	}
}

//brief:判断自连接
bool isSelfConnect(int sockfd) {
	struct sockaddr_in6 local = getLocalSockaddrFromFd(sockfd);
	struct sockaddr_in6 peer = getPeerSockaddrFromFd(sockfd);

	//return memcmp(&local, &peer, sizeof local) == 0;
	if (local.sin6_family == AF_INET) {
		auto first = reinterpret_cast<struct sockaddr_in*>(&local);
		auto second = reinterpret_cast<struct sockaddr_in*>(&peer);

		return first->sin_port == second->sin_port &&
			memcmp(&first->sin_addr, &second->sin_addr, sizeof(first->sin_addr)) == 0;
	}
	else if(local.sin6_family==AF_INET6) {
		return local.sin6_port == peer.sin6_port &&
			memcmp(&local.sin6_addr, &peer.sin6_addr, sizeof(local.sin6_addr)) == 0;
	}

	//既不是ipv4也不是ipv6
	return false;
}

//brief:获得本地绑定的地址
struct sockaddr_in6 getLocalSockaddrFromFd(int sockfd) {
	struct sockaddr_in6 out;
	memZero(&out, sizeof out);

	socklen_t length = static_cast<socklen_t>(sizeof out);
	
	void* ptr = &out;
	int v = ::getsockname(sockfd, static_cast<sockaddr*>(ptr), &length);
	SYSCALL_ERR(v);

	return out;
}
//brief:获得远端绑定的地址
struct sockaddr_in6 getPeerSockaddrFromFd(int sockfd) {
	struct sockaddr_in6 out;
	memZero(&out, sizeof out);

	socklen_t length = static_cast<socklen_t>(sizeof out);
	
	void* ptr = &out;
	int v = ::getpeername(sockfd, static_cast<sockaddr*>(ptr), &length);
	SYSCALL_ERR(v);

	return out;
}
//////////////////////////////////////////////////////

//brief:
void inet_pton(const char* str, uint16_t port, struct sockaddr_in* addr) {
	assert(str);
	assert(addr);
	//assert(addr->sin_family == AF_INET);

	addr->sin_family = AF_INET;
	//addr->sin_port = port;//z注意采用网络序
	addr->sin_port = htons(port);
	int ret = ::inet_pton(AF_INET, str, &addr->sin_addr);
	SYSCALL_ERR(ret);
}

void inet_pton(const char* str, uint16_t port, struct sockaddr_in6* addr) {
	assert(str);
	assert(addr);
	//assert(addr->sin6_family == AF_INET6);

	addr->sin6_family = AF_INET6;
	//addr->sin6_port = port;//error
	addr->sin6_port = htons(port);
	int ret = ::inet_pton(AF_INET6, str, &addr->sin6_addr);
	SYSCALL_ERR(ret);
}

//将二进制地址转换到点分十进制
void inet_ntop(const struct sockaddr_in* addr, char* buf, size_t len) {
	assert(addr);
	assert(addr->sin_family == AF_INET);
	assert(len >= INET_ADDRSTRLEN);

	::inet_ntop(AF_INET, &addr->sin_addr, buf, static_cast<socklen_t>(len));
}
void inet_ntop(const struct sockaddr_in6* addr, char* buf, size_t len) {
	assert(addr);
	assert(addr->sin6_family == AF_INET6);
	assert(len >= INET6_ADDRSTRLEN);

	::inet_ntop(AF_INET6, &addr->sin6_addr, buf, static_cast<socklen_t>(len));
}
//带有port信息版本
void inet_ntop_withport(const struct sockaddr_in* addr, char* buf, size_t len) {
	assert(len >= INET_ADDRSTRLEN + 6);

	socket_ops::inet_ntop(addr, buf, len);
	size_t size = ::strlen(buf);
	uint16_t port = socket_ops::ntohs(addr->sin_port);

	snprintf(buf + size, len - size, ":%u", port);
}
void inet_ntop_withport(const struct sockaddr_in6* addr, char* buf, size_t len) {
	assert(len >= INET6_ADDRSTRLEN + 6);

	socket_ops::inet_ntop(addr, buf, len);
	size_t size = ::strlen(buf);
	uint16_t port = socket_ops::ntohs(addr->sin6_port);

	snprintf(buf + size, len - size, ":%u", port);
}

#undef SYSCALL_ERR
#undef SYSCALL_FATAL

}
}
