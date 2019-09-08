/*************************************************************************
	> File Name: logfile.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月05日 星期四 09时07分02秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_LOGFILE_H_
#define _IMNET_DEPENDENT_LOGFILE_H_

#ifdef _MSC_VER
#include"../noncopyable.h"
#include"../types_extension.h"
#include"../advanced/mutexlock.h"
#else
#include"dependent/noncopyable.h"
#include"dependent/types_extension.h"
#include"dependent/advanced/mutexlock.h"
#endif

#include<memory>

namespace imnet {

namespace file_extension {
class Appendfile;//前向声明
}

//brief:本地日志存储实现
//becare:构造时可以指定是否需要mutex保护
//      刷新机制是基于事件的，当写日志触发时，会根据参数进行自动刷新以及日志滚动
class Logfile :noncopyable {
public:
	typedef Logfile* PointerType;

	explicit Logfile(const string& process_name,
		off_t threshhold_filesize,
		bool need_mutex = true,
		int threshhold_flush_interval = 4,
		int threshhold_check_write_number = 1024);

	~Logfile();

	void append(const char* content, int len_content);
	void flush();
	bool rollFile();

private:
	//被更新的策略
	void __append(const char* content, int len_content);
	void __append_without_lock(const char* content, int len_content);
	void __flush();
	void __flush_without_lock();

	string getNextLogfileName(time_t& now);

	static
	//brief:用于绑定不同策略，主要是针对有锁和无锁两个版本的分离
	void(Logfile::*__flushFunction)();
	void(Logfile::*__appendFunction)(const char*, int);
	
	const string __process_basename;
	const off_t __threshold_file_size;
	const int __threshold_flush_interval;
	const int __threshold_check_write_number;

	const static int kDayToSecondsRatio = 24 * 60 * 60;//用于定位天数
	time_t __last_flush;//上次刷新时间，用于定期刷新
	time_t __day_number;//Unix时间中的第几天
	int __count_write_number;//写入日志的个数
		std::unique_ptr<file_extension::Appendfile> __file;//实际handler

	//becare:这是使用中的可选项,依赖于构造时的参数
	std::unique_ptr<Mutex> __mutex;
};


}


#endif
