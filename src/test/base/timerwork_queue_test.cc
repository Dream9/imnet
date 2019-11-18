/*************************************************************************
	> File Name: timerwork_queue_test.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月11日 星期一 12时11分43秒
 ************************************************************************/

#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"

#include<cstdio>

using namespace imnet;
using namespace imnet::core;

Eventloop* g_loop;
void testFunc();
void testFunc2();

int main(){
	CLogger::setLogLevel(CLogger::kTrace);

	Eventloop loop;
	g_loop = &loop;

	loop.runAfter(testFunc,9);
	loop.runEvery(testFunc2,2.5);

	loop.loop();
}

void testFunc(){
	printf("I'm %u\n", thread_local_variable::gettid());
}

void testFunc2(){
	printf("This function should be called every 2.5s\n");
}


