/*************************************************************************
	> File Name: asynclogger_test.cc
	> Author: Dream9
	> Brief: Asynclogger使用示范
	> Created Time: 2019年09月22日 星期日 14时42分27秒
 ************************************************************************/

#include"dependent/logger/asynclogger.h"
#include"dependent/logger/logger.h"
#include"dependent/logger/timestamp.h"

#include<time.h>

using namespace std;

imnet::AsyncLogger *g_log;
const int kTestTimes=300000;
string junkdata(3000,'D');
void outputFunc(const char*,int);

int main(int argc,char*argv[]){
	//启动AsyncLogger
	imnet::AsyncLogger alog(::basename(argv[0]),64*1024);
	g_log=&alog;
	g_log->startBackend();

	{
		//配合CLogger使用
		imnet::CLogger::setOutputFunc(outputFunc);//配接

		imnet::CTimestamp start=imnet::CTimestamp::getNow();
		for(int i=0;i<kTestTimes;++i){
			LOG_INFO<<junkdata;
			LOG_WARN<<"asdfghjkl";
			//becare:如果持续不断的写入，那么会导致前后端IO速度不匹配，引发部分日志被丢弃
			//sleep(1);

		}
		imnet::CTimestamp end=imnet::CTimestamp::getNow();
		printf("log items: %d,cost time is %f\n",kTestTimes,imnet::timestampDifference(end,start));
	}

	{
		//单独使用,此时需要用户自己负责输出格式化
		for(int i=0;i<kTestTimes;++i){
			g_log->append("as",2);
		}
	}

	//becare:没有必要，在析构的时候会唤醒后端，然后阻塞等待刷新完成
	//g_log->stopBackend();
}


//brief:用于AsyncLogger和CLogger配接
void outputFunc(const char*msg, int len){
	g_log->append(msg,len);
}
