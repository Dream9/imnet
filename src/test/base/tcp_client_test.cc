/*************************************************************************
	> File Name: tcp_client_test.cc
	> Author: Dream9
	> Brief:测试TcpServer
	> Created Time: 2019年11月09日 星期六 14时12分36秒
 ************************************************************************/

#include"core/tcp/tcp_client.h"
#include"core/tcp/tcp_connection.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#include"dependent/process_information.h"
#include"dependent/logger/timestamp.h"


#include<cstdio>
#include<string.h>
#include<assert.h>

using namespace imnet;
using namespace imnet::core;

void onConnection(const TcpConnectionPtr&);
void onMessage(const TcpConnectionPtr&,Buffer*,CTimestamp);
static int i = 0;

Eventloop* g_loop;//or use tl_current_thread_eventloop?
TcpClient* g_client;

int main(int argc, char* argv[]){
	CLogger::setLogLevel(CLogger::kTrace);
	printf("tcp_client_test,pid=%u\n", process_information::getPid());
	int port = 6666;
	if(argc>1){
		port = atoi(argv[1]);
	}

	Eventloop loop;
	g_loop = &loop;
	Sockaddr telnet_addr("127.0.0.1", port);
	printf("ready connect to %s", telnet_addr.inet_ntop_withport().c_str());

	//初始化客户端
	TcpClient client(&loop, telnet_addr, "No.996");
	g_client = &client;
	client.setConnectionCallback(onConnection);
	client.setMessageCallback(onMessage);
	client.activateRetry();//非用户主动关闭时，需要重连
	client.connect();

	loop.loop();
}

void onConnection(const TcpConnectionPtr& con){
	if(con->isConnected()){
		printf("new connection:%s from %s.\n",con->getName().c_str(),
				con->getPeerAddress().inet_ntop_withport().c_str());
		
		con->send("Hello World,I'm No.996\n");
	}
	else{
		printf("kokodayo,if there are at least 2 messages,this progress should end\n");

		//用户主动推出事件循环
		g_loop->quit();
	}
}

void onMessage(const TcpConnectionPtr& con, Buffer* buf, CTimestamp stamp){
	//FIXME:在两个序列点之间对同一族数据发生了修改以及访问的冲突
	//printf("received %lu bytes data from %s at %s.\n"
	//		"data is:\n"
	//		"%s",buf->getDataSize(),con->getName().c_str(),
	//		stamp.toString().c_str(),buf->retrieveAllToString().c_str());
	
	auto len=buf->getDataSize();
	printf("received %lu bytes data from %s at %s.\n"
			"data is:\n"
			"%s",len,con->getName().c_str(),
			stamp.toString().c_str(),buf->retrieveAllToString().c_str());

	++i;
	if(i>=2){
		printf("function retry is closed\n");
		g_client->deactivateRetry();
	}
}
