/*************************************************************************
	> File Name: acceptor_test.cc
	> Author: Dream9
	> Brief:测试Acceptor可用 
	> Created Time: 2019年11月06日 星期三 15时14分10秒
 ************************************************************************/

#include"core/reactor/eventloop.h"
#include"core/tcp/acceptor.h"
#include"core/tcp/sockaddr.h"
#include"core/tcp/socket_extension.h"
#include"dependent/process_information.h"
#include"dependent/logger/logger.h"

#include<stdio.h>

using namespace imnet;

void onConnect(int sockfd, const core::Sockaddr& peer);

int main(){

	CLogger::setLogLevel(CLogger::kTrace);
	LOG_INFO<<"acceptor_test:pid=" << process_information::getPid();

	//监听地址
	core::Sockaddr listen_addr(6666);
	LOG_INFO<<"LISTEN on:"<<listen_addr.inet_ntop_withport();

	core::Eventloop loop;

	//设置监听处理
	core::Acceptor acce(&loop,listen_addr);
	acce.setConnectionCallback(onConnect);
	acce.listen();

	loop.loop();
}

void onConnect(int sockfd, const core::Sockaddr& peer){
	printf("Accetp a new connection,peer addr:%s\n",peer.inet_ntop_withport().c_str());

	char data[]="Hello World!\n";
	::write(sockfd,data,sizeof data);
	
	socket_ops::close(sockfd);
}
