/*************************************************************************
	> File Name: httpd_server_test.cc
	> Author: Dream9
	> Brief:测试httpd 
	> Created Time: 2019年11月15日 星期五 17时16分57秒
 ************************************************************************/

#include"httpd/http_server.h"
#include"core/reactor/eventloop.h"
#include"core/tcp/sockaddr.h"
#include"dependent/logger/logger.h"
#include"dependent/process_information.h"

#include<stdlib.h>

using namespace imnet;
using namespace imnet::core;
using namespace imnet::http;

int main(int argc, char**argv){
	LOG_INFO << argv[0] << ",pid = " << process_information::getPid() <<", Listen at 5996.";

	//使用HTTP1.0协议
	//::setenv("IMNET_HTTP10","1");

	Eventloop loop;
	Sockaddr address(5996);

	//测试默认的处理
	HttpServer server(&loop, "DN", address);
	server.setThreadNumber(2);
	server.start();

	loop.loop();
}

