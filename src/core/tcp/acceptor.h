/*************************************************************************
	> File Name: acceptor.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时32分07秒
 ************************************************************************/
#ifndef _CORE_ACCEPTOR_H_
#define _CORE_ACCEPTOR_H_

#ifdef _MSC_VER
#include"server_socket.h"
#include"../reactor/channel.h"
#else
#include"core/tcp/server_socket.h"
#include"core/reactor/channel.h"
#endif

#include<functional>

namespace imnet {

namespace core {

class Eventloop;
class Sockaddr;

class Acceptor : noncopyable {
public:
	typedef std::function<void(int, const Sockaddr&)> OnConnectionCallbackType;
	Acceptor(Eventloop* loop, const Sockaddr& listen_address, OnConnectionCallbackType func, bool need_reuse_port=false);
	Acceptor(Eventloop* loop, const Sockaddr& listen_address);
	~Acceptor();

	void listen();

	void setConnectionCallback(OnConnectionCallbackType call) {
		__new_connection_callback = call;
	}
	void setReusePort(bool on);
	void setKeepAlive(bool on);
	void setTcpNoDelay(bool on);

	bool isListening()const {
		return __is_listening;
	}

private:
	void handleRead();

	Eventloop * __loop;
	ServeSocket __listen_fd;
	OnConnectionCallbackType __new_connection_callback;
	Channel __channel;
	bool __is_listening;
	int __idle_fd_for_exhaust;

};
}
}


#endif
