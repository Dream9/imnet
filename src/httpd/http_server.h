/*************************************************************************
	> File Name: http_server.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月15日 星期五 10时19分37秒
 ************************************************************************/

#ifndef _HTTPD_HTTP_SERVER_H_
#define _HTTPD_HTTP_SERVER_H_

#ifdef _MSC_VER
#include"../core/tcp/tcp_server.h"
#else
#include"core/tcp/tcp_server.h"
#endif

namespace imnet {

namespace core{

class Buffer;
class Eventloop;

}

namespace http{

class HttpRequest;
class HttpResponse;

void defaultHttpService(const HttpRequest&, HttpResponse*);

class HttpServer :noncopyable {
public:
	typedef std::function <void(const HttpRequest&, HttpResponse*)> HttpServiceCallbackType;
	typedef std::function < void(core::Eventloop*)> ThreadInitCallbackType;

	HttpServer(core::Eventloop* loop, const string& name, const core::Sockaddr& address, HttpServiceCallbackType = defaultHttpService);

	core::Eventloop* getEventloop()const {
		return __server.getEventloop();
	}

	void setServiceCallback(const HttpServiceCallbackType& call) {
		__service_call = call;
	}

	void setThreadInitCallback(const ThreadInitCallbackType& call){
		__server.setThreadInitCallback(call);
	}

	void setThreadNumber(int number) {
		__server.setThreadNumber(number);
	}

	void start();
	//void start() {
	//	__server.start();
	//}

private:
	void __onMessage(const core::TcpConnectionPtr& connection, core::Buffer* buf, CTimestamp timestamp);	
	void __onConnection(const core::TcpConnectionPtr& connection);
	void __onRequest(const core::TcpConnectionPtr& connection, const HttpRequest& request);
	void __onBadRequest(const core::TcpConnectionPtr& connection);
	
	core::TcpServer __server;
	HttpServiceCallbackType __service_call;

};

}//!namespace http
}//!namespace imnet



#endif
