/*************************************************************************
	> File Name: logfile.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月05日 星期四 09时07分09秒
 ************************************************************************/
#ifdef _MSC_VER
#include"logfile.h"
#include"../file_extension.h"
#include"../process_information.h"
#else
#include"dependent/logger/logfile.h"
#include"dependent/file_extension.h"
#include"dependent/process_information.h"
#endif


#include<ctime>
#include<assert.h>

namespace imnet {

Logfile::Logfile(const string& process_name,
		off_t threshhold_filesize,
		bool need_mutex,
		int threshhold_flush_interval,
		int threshhold_check_write_number)
	:__process_basename(process_name),
	__threshold_file_size(threshhold_filesize),
	__threshold_flush_interval(threshhold_flush_interval),
	__threshold_check_write_number(threshhold_check_write_number){

	assert(__process_basename.find('/') == string::npos);

	//更新策略
	if (need_mutex) {
		__flushFunction = &Logfile::__flush;
		__appendFunction = &Logfile::__append;
	}
	else {
		__flushFunction = &Logfile::__flush_without_lock;
		__appendFunction = &Logfile::__append_without_lock;
	}
	//切换日志文件
	rollFile();
}

//brief:主动outline
Logfile::~Logfile() = default;

void Logfile::append(const char *content,int len_content) {
	(this->*__appendFunction)(content,len_content);
}

void Logfile::flush() {
	(this->*__flushFunction)();
}

//brief:内置的两种写日志策略
//becare:append事件累计到阈值时将会触发roll以及flush检查
void Logfile::__append_without_lock(const char *content,int len_content) {
	__file->writeToEndUnlocked(content, len_content);
	if (__file->writtenBytes() > __threshold_file_size) {
		rollFile();
	}
	else {
		++__count_write_number;
		if (__count_write_number > __threshold_check_write_number) {
			//触发检测
			__count_write_number = 0;//更新检测周期
			time_t now_seconds = ::time(NULL);
			time_t now_day_number = now_seconds / kDayToSecondsRatio;
			if (now_day_number != __day_number) {
				//进入新的一天，必须更换文件
				rollFile();
			}
			else if(now_seconds-__last_flush>__threshold_flush_interval){
				//检测刷新间隔是否满足条件
				__last_flush = now_seconds;
				__file->flush();
			}
		}
	}
}
void Logfile::__append(const char *content, int len_content) {
	assert(__mutex);
	MutexUnique_lock u_lock(*__mutex);
	__append_without_lock(content, len_content);
}

//brief:内置的两种flush策略
void Logfile::__flush_without_lock() {
	__file->flush();
}
void Logfile::__flush() {
	assert(__mutex);
	MutexUnique_lock u_lock(*__mutex);
	__flush();
}

//brief:滚动日志文件，包括基于事件的时间触发以及基于事件的大小触发
//becare:尽管这里并没有加锁，但是理论上如果存在潜在的竞争，用户应当选择
//      激活mutex的构造函数，从而保证内部成员对__file handler的访问(主要是flush和append)
//      之前，都选择了带锁的策略，因此此函数并没有加锁。相反，如果没有激活mutex,或者在
//      没有保护的情况下外部调用rollFile的行为是线程不可重入的
bool Logfile::rollFile() {
	time_t now_seconds;
	string next_filename = getNextLogfileName(now_seconds);

	//更新数据
	__last_flush = now_seconds;
	__day_number = now_seconds/ kDayToSecondsRatio;
	__file.reset(new file_extension::Appendfile(next_filename));

	return true;
}

//brief:获得下一个log文件的滚动名称,并修改now时间
//     格式为 主程序名.GMT时间.机器名.进程pid.后缀名(.log)
//     实例:loggertest.20010118130218.abc.1245.log
//becare:采用GMT时间可以提供一致的标准
string Logfile::getNextLogfileName(time_t &now) {
	string result;
	result.reserve(__process_basename.size() + 64);

	result.append(__process_basename);
	
	struct tm tm_now;
	now = ::time(NULL);

	::gmtime_r(&now, &tm_now);
	char buf[32];
	strftime(buf, sizeof buf, ".%Y%m%d%H%M%S.", &tm_now);
	result += buf;

	result += process_information::getHostname();

	snprintf(buf, sizeof buf, ".%d", process_information::getPid());
	result += buf;

	result += ".log";

	return result;
}

}//!namespace imnet



