/*************************************************************************
	> File Name: tcp_server.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时32分38秒
 ************************************************************************/
#ifndef _CORE_TCP_SERVER_H_
#define _CORE_TCP_SERVER_H_

#ifdef _MSC_VER
#include"tcp_connection.h"
#include".././../dependent/types_extension.h"
#else
#include"core/tcp/tcp_connection.h"
#include"dependent/types_extension.h"
#endif

#include<memory>
#include<map>
#include<functional>
#include<atomic>

namespace imnet {

namespace core {

class Eventloop;
class Acceptor;
class Sockaddr;
class EventloopThreadPool;

//brief:对服务端的封装
//     用户可以传入一些例程用于在特定事件的处理
class TcpServer : noncopyable {
public:
	typedef std::function<void(Eventloop*)> ThreadInitCallbackType;
	typedef std::map<std::string, TcpConnectionPtr> ConnectionContainerType;
	typedef int CountType;

	TcpServer(Eventloop* loop, const string& basename, const Sockaddr& listen_address);
	~TcpServer();

	//
	string getListenAddress()const {
		return __listen_address;
	}
	string getName()const {
		return __basename;
	}
	Eventloop* getEventloop()const {
		return __loop;
	}


	//用户注入的接口
	void setConnectionCallback(const ConnectionCallbackType& callback) {
		__on_connection_call = callback;
	}
	void setMessageCallback(const MessageCallbackType& callback) {
		__on_message_call = callback;
	}
	void setWriteCompleteCallback(const WriteCompleteCallbackType& callback) {
		__on_write_complete_call = callback;
	}
	void setThreadInitCallback(const ThreadInitCallbackType& callback);
		
	void setThreadNumber(size_t number);

	void start();

private:
	//对用户传入的__on_connect_call的封装
	void __newConnectionCallback(int sockfd, const Sockaddr& peer_address);
	//唯一的关闭事件响应处理例程
	void __removerConnectionCallback(const TcpConnectionPtr& connection);
	void __removeConnectionCallbackInloop(const TcpConnectionPtr& connection);

	Eventloop* __loop;
	string __basename;
	string __listen_address;
	CountType __count;//由于创建连接总是在loop线程，因此不需要锁保护
	//bool __is_running;
	std::atomic_int __is_running;

	std::unique_ptr<Acceptor> __acceptor;
	ConnectionContainerType __container;

	std::unique_ptr<EventloopThreadPool> __thread_pool;

	//用户传入的处理例程
	ConnectionCallbackType __on_connection_call;
	MessageCallbackType __on_message_call;
	WriteCompleteCallbackType __on_write_complete_call;
	//ThreadInitCallbackType __on_thread_init_call;

};


}//!namespace core
}//!namespace imnet

#endif
