/*************************************************************************
	> File Name: eventloop_test.cc
	> Author: Dream9
	> Brief: 测试Eventloop可用
	> Created Time: 2019年11月06日 星期三 18时27分30秒
 ************************************************************************/

#include"core/reactor/eventloop.h"
#include"dependent/advanced/thread_extension.h"
#include"dependent/thread_local_variable.h"
#include"dependent/logger/logger.h"

#include<cstdio>


void threadFunc();

int main(){
	//修改输出级别
	imnet::CLogger::setLogLevel(imnet::CLogger::kTrace);

	printf("pid=%u/n",imnet::thread_local_variable::gettid());

	imnet::core::Eventloop loop;
	
//	imnet::Thread(threadFunc);

//	thread.start();

	loop.loop();

}

void threadFunc(){
	printf("Thread:");
}
