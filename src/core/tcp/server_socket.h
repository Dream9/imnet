/*************************************************************************
	> File Name: server_socket.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 11时17分28秒
 ************************************************************************/
#ifndef _CORE_SERVE_SOCKET_H_
#define _CORE_SERVE_SOCKET_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
#else
#include"dependent/noncopyable.h"
#endif

struct tcp_info;

namespace imnet {

namespace core {

class Sockaddr;

//brief:基于tcp协议的serve端套接字的封装
//     主要提供了对该套接字的socket,bind,listen,accept,读端关闭，地址重用，保活探测，
//     关闭Negale算法等功能
//becare:sockfd一旦传给本对象，则表示所有权的转移
class ServeSocket:noncopyable {
public:
	explicit ServeSocket(int fd) :__sockfd(fd) {};//直接转移某个已经产生的套接字
	explicit ServeSocket(const Sockaddr& addr);
	//explicit ServeSocket(sa_family_t family);//无法区分，这是错误的，舍弃

	~ServeSocket();

	void bind(const Sockaddr& addr);
	void listen();
	int accept(Sockaddr* peeraddr);
	void shutdownWrite();

	//状态设置
	void setTcpNoDelay(bool on);
	void setReuseAddr(bool on);
	void setKeepAlive(bool on);
	//The new socket option allows multiple sockets on the same host to bind to the 
	//same port, and is intended to improve the performance of multithreaded
	//network server applications running on top of multicore systems.
	//brief:通过设置本属性，可以使得多个套接字监听同一个端口，从而实现listen的均衡化
	void setReusePort(bool on);

	//状态信息获取
	int getHandle() const {
		return __sockfd;
	}
	bool getTcpInfo(struct tcp_info* parser_ans)const;
	bool getTcpInfo(char* str, int len)const;
	//bool getTcpInfo(char* str, size_t len)const;

private:
	//底层数据资源，本对象掌控其生命期
	const int __sockfd;
};
}
}

#endif
