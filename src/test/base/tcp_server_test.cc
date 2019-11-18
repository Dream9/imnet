/*************************************************************************
	> File Name: tcp_server_test.cc
	> Author: Dream9
	> Brief:测试TcpServer
	> Created Time: 2019年11月07日 星期四 09时45分57秒
 ************************************************************************/

#include"core/tcp/tcp_server.h"
#include"core/tcp/tcp_connection.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#include"dependent/process_information.h"
#include"dependent/logger/timestamp.h"
#include"dependent/advanced/atomic_type.h"

#include<boost/any.hpp>

#include<cstdio>
#include<limits>
#include<string.h>
#include<assert.h>

using namespace imnet;
using namespace imnet::core;

void onConnection(const TcpConnectionPtr&);
void onMessage(const TcpConnectionPtr&,Buffer*,CTimestamp);
static AtomicInt32 global_id(0);
static const int ENOUGH = 128;

static_assert(ENOUGH >= sizeof("Hello World") + std::numeric_limits<int32_t>::digits10);

int main(){
	LOG_INFO<<"tcp_server_test,pid=" << process_information::getPid() << "Listen at 5996";

	Eventloop loop;
	Sockaddr listen_addr(5996);

	//初始化服务端
	TcpServer serv(&loop,"ServerTest",listen_addr);
	serv.setConnectionCallback(onConnection);
	serv.setMessageCallback(onMessage);
	serv.setThreadNumber(2);
	serv.start();

	loop.loop();
}

void onConnection(const TcpConnectionPtr& con){
	if(con->isConnected()){
		printf("new connection:%s from %s.\n",con->getName().c_str(),
				con->getPeerAddress().inet_ntop_withport().c_str());
		int32_t newid = ++global_id;
		con->setContext(newid);
		
		char buf[ENOUGH];
		snprintf(buf, sizeof buf, "Hello World,%d\n",newid);
		con->send(buf, strlen(buf));
	}
	else{
		printf("connection:%s is down\n",con->getName().c_str());
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

	auto id = con -> getContext();

	if(id.empty()){
		int32_t newid = ++global_id;
		con->send("Hello world, keep your id:%d\nGot it\n", newid);
		con->setContext(newid);
	}
	else{
		//LOG_INFO<<boost::any_cast<int32_t>(id);
		char buf[ENOUGH];
		snprintf(buf, sizeof buf, "Got it,%d\n",boost::any_cast<int32_t>(id));
		con->send(buf, strlen(buf));
	}

}
