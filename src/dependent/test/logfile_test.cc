/*************************************************************************
	> File Name: logfile_test.cc
	> Author: Dream9
	> Brief: 展示了Logfile实例
	> Created Time: 2019年09月20日 星期五 10时13分28秒
 ************************************************************************/

#include"dependent/logger/logfile.h"
#include"dependent/logger/logger.h"
#include"dependent/advanced/thread_extension.h"

#include<memory.h>
#include<string>
#include<string.h>
#include<functional>

#include<libgen.h>

void outputFunc(const char*,int);
void flushFunc();
void threadFunc(void*);

using namespace std;

unique_ptr<imnet::Logfile>g_log;
const int kTestTimes=100000;

int main(int argc, char*argv[]){
	char junkdata[]="123456789 asdfghjkl zxcvbnm qwertyuiop";
	char junkdata2[]="QWERTYUIOPASDFGHJKLZXCVBNM\n";	
	char junkdata3[]="123456789\n";	

	{
		//单线程下单独使用
		imnet::Logfile log(::basename(argv[0]),64*1024,false);
		for(int i=0;i<kTestTimes;++i)
			log.append(junkdata,sizeof junkdata -1);
	}

	{
		//单线程下配合logger使用
		string basename(::basename(argv[0]));
		basename+="_connect_to_logger";

		//通过set系列函数与CLogger配合
		imnet::CLogger::setOutputFunc(outputFunc);
		imnet::CLogger::setFlushFunc(flushFunc);

		g_log.reset(new imnet::Logfile(std::move(basename),64*1024,false));//basename之后空
		for(int i=0;i<kTestTimes;++i)
			LOG_INFO<<junkdata;//数据将会追加到CLogfile
	}

	{
		//多线程环境下使用
		string basename(::basename(argv[0]));
		basename+="_pthread";

		g_log.reset(new imnet::Logfile(std::move(basename),64*1024));//默认开启内部锁保护
		imnet::Thread t1(::bind(threadFunc,junkdata2)); 
		imnet::Thread t2(::bind(threadFunc,junkdata3)); 
		t1.start();
		t2.start();
		t1.join();
		t2.join();

		g_log->flush();
	}

}


//配合CLogger
void outputFunc(const char *msg,int len){
	g_log->append(msg,len);
}
void flushFunc(){
	g_log->flush();
}

//多线程环境测试
void threadFunc(void*msg){
	char *str=reinterpret_cast<char*>(msg);
	int len=static_cast<int>(strlen(str));

	for(int i=0;i<kTestTimes;++i){
		g_log->append(str,len);
	}
}
