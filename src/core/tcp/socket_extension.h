/*************************************************************************
	> File Name: socket_extension.h
	> Author: Dream9
	> Brief: 针对本项目的有关<sys/socket.h>的适配
	> Created Time: 2019年10月10日 星期四 10时52分33秒
 ************************************************************************/
#ifndef _CORE_SOCKET_EXTENSION_H_
#define _CORE_SOCKET_EXTENSION_H_

#ifdef _WIN32
#include<WS2tcpip.h>
#else
#include<arpa/inet.h>
#include<netinet/in.h>//some system require this file instead of <arpa/inet.h>
#include<endian.h>
#endif

namespace imnet {

namespace core {

class Sockaddr;

}

//brief:本项目中只使用TCP协议,允许半关闭读端，自动处理自连接问题，
//     以及众多函数的适配
namespace socket_ops {

////////////////////////////////////////////////////////////////////////
//关于套接字的操作
int createNonblockingTcpSocket(sa_family_t family);
void bind(int sockfd, const struct sockaddr* addr);
void listen(int sockfd);
int accept(int sockfd, struct sockaddr_in6* addr);
int connect(int sockfd, const struct sockaddr* addr);//TODO

int read(int sockfd, void* buf, size_t len);//TODO
int readv(int sockfd, const struct iovec* iov, int ionumber);//TODO
int write(int sockfd, const void* buf, size_t len);

void close(int sockfd);
void shutdownWrite(int sockfd);

//TODO:
int getErrno(int sockfd);//用于connector判断连接是否出错
bool isSelfConnect(int sockfd);//用于connector判断自连接
struct sockaddr_in6 getLocalSockaddrFromFd(int sockfd);//用于从已经建立连接的socket中获取本地地址，主要用于TcpServer/TcpClient
struct sockaddr_in6 getPeerSockaddrFromFd(int sockfd);//用于从已经建立连接的socket中获取对方地址,主要用于TcpClient
////////////////////////////////////////////////////////////////////////
//关于网络地址格式的操作

//字节序调整
inline uint16_t htons(uint16_t data){
	return ::htons(data);
}
inline uint32_t htonl(uint32_t data) {
	return ::htonl(data);
}
inline uint16_t ntohs(uint16_t data) {
	return ::ntohs(data);
}
inline uint32_t ntohl(uint32_t data) {
	return ::ntohl(data);
}
//一下的两个实现采用的是endian下的函数
//beace:这亮着的定义都是宏
//define be64toh(x) __bswap__64 (x) 
//因此不要带着::使用
inline uint64_t hton64(uint64_t data) {
	//return ::htobe64(data);
	return htobe64(data);
}
inline uint64_t ntoh64(uint64_t data) {
	//return ::be64toh(data);
	return be64toh(data);
}

//关于这里的glibc库函数之所以很繁琐，就是因为c语言不支持重载，
//导致必须引入额外的参数来进行辨别同名函数的具体功能
//这里的实现是：令c++重载在此文件内发生，注意分发则交由用户处理
void inet_pton(const char* str, uint16_t port, struct sockaddr_in* addr);
void inet_pton(const char* str, uint16_t port, struct sockaddr_in6* addr);

void inet_ntop(const struct sockaddr_in* addr, char* buf, size_t len);
void inet_ntop(const struct sockaddr_in6* addr, char* buf, size_t len);
void inet_ntop_withport(const struct sockaddr_in* addr, char* buf, size_t len);
void inet_ntop_withport(const struct sockaddr_in6* addr, char* buf, size_t len);
}//!namespace socket_ops

}//!namespace imnet


#endif
