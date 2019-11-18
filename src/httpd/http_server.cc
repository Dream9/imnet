/*************************************************************************
	> File Name: http_server.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月15日 星期五 10时19分42秒
 ************************************************************************/

#ifdef _MSC_VER
#include"http_server.h"
#include"http_context.h"
#include"http_response.h"
#include"../core/reactor/eventloop.h"
#include"../dependent/logger/logger.h"
#else
#include"httpd/http_server.h"
#include"httpd/http_context.h"
#include"httpd/http_response.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#endif

#include<boost/any.hpp>

using namespace imnet;
using namespace imnet::http;
using namespace imnet::core;

namespace imnet{

namespace http{
//brief:默认的http服务
//      直接返回404
void defaultHttpService(const HttpRequest& request, HttpResponse* response){
	response->setStatus(ResponseStatusCode::kNotFound);
	response->setContentType("text/html; charset=utf-8");//becare:汉字编码 :set fileencoding查看 
	response->addHeader("Server", "imnet-httpd");
	response->setKeepAlive(false);
	const char res[] =
		"<html>"
		"<head><title>界面被查水表了</title></head?"
		"<body><p>估计是回不来了-_-</p></body>\r\n"
		"</html>";
	response->setBody(res, res + sizeof res - 1);
}

}//!namespace http
}//!namespace imnet

//brief:初始化各个参数
//     复用port通过环境变量设置
HttpServer::HttpServer(Eventloop* loop, const string& name, const Sockaddr& address, HttpServer::HttpServiceCallbackType call)
	:__server(loop, name, address),
	__service_call(call){

	__server.setConnectionCallback(std::bind(&HttpServer::__onConnection, this, std::placeholders::_1));
	__server.setMessageCallback(std::bind(&HttpServer::__onMessage, this, std::placeholders::_1,
			std::placeholders::_2,std::placeholders::_3));
}

//brief:
void HttpServer::start() {
	LOG_INFO << "HttpServer:" << __server.getName() << " online.";
	__server.start();
}

//brief:响应连接事件，主要是在初次建立连接时为其维护一个唯一的上下文结构
void HttpServer::__onConnection(const TcpConnectionPtr& connection) {
	if (connection->isConnected()) {
		LOG_TRACE << "New:" << connection->getName();
		connection->setContext(HttpContext());
	}
	else {
		LOG_TRACE<<connection->getName()<<" :Done";
	}
}

//brief:响应信息抵达事件，利用context逐步处理每一个请求
void HttpServer::__onMessage(const TcpConnectionPtr& connection, Buffer* buf, CTimestamp timestamp) {

	HttpContext* context = boost::any_cast<HttpContext>(connection->getContextPtr());

	int ret = context->parseRequest(buf, timestamp);
	if (ret < 0) {
		__onBadRequest(connection);
		return;
	}
	if (context->isDone()) {
		__onRequest(connection, context->getHttpRequest());
		context->reset();
	}
}

//brief;响应400格式错误
void HttpServer::__onBadRequest(const TcpConnectionPtr& connection) {
	char header400[] = "HTTP/1.1 400 BadRequest\r\n"
		"Server:imnet-httpd 1.1/r/n"
		"Content-Type:text/html\r\n";

	char body400[]="<html>"
		"<head><title>小伙子不要乱请求</title></head>\r\n"
		"<body><p>年轻人总想搞出点大事情,400了吧</p></body>\r\n"
		"</html>";

	char length[32] = "Content-Length:%u\r\n\r\n";
	snprintf(length, sizeof length, length, sizeof body400);

	connection->send(header400, sizeof header400);
	connection->send(length);
	connection->send(body400);

	connection->shutdown();
}

//brief:处理请求
void HttpServer::__onRequest(const TcpConnectionPtr& connection, const HttpRequest& request) {
	string tmp = request.getHeader("Connection");

	//HTTP1.1默认Keep-Alive
	HttpResponse response(tmp != "Close" || (tmp == "" && request.getVersion() == Version::kHttp11));
	__service_call(request, &response);

	Buffer buf;
	response.appendToBuffer(&buf);
	connection->send(&buf);

	if (!response.keepAlive()) {
		connection->shutdown();
	}
}
