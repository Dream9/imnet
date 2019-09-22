/*************************************************************************
	> File Name: asynclogger.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月08日 星期日 16时10分26秒
 ************************************************************************/
#ifdef _MSC_VER
#include"asynclogger.h"
#include"timestamp.h"
#include"logfile.h"
#else
#include"dependent/logger/asynclogger.h"
#include"dependent/logger/timestamp.h"
#include"dependent/logger/logfile.h"
#endif

namespace imnet {

//brief:初始化异步日志
AsyncLogger::AsyncLogger(const string &basename,int threshold_roll_file_size,int threshold_interval)
	:__is_running(false),
	__flush_interval(threshold_interval),
	__basename(basename),
	__threshold_roll_file_size(threshold_roll_file_size),
	__backend_thread(std::bind(&AsyncLogger::__backend_work_function,this)),
	__latch(1),
	__mutex(),
	__cond(__mutex),
	__frontend_current_buffer(new BufferType),
	__frontend_alternative_buffer(new BufferType),
	__to_write_buffer_container(){

	__frontend_current_buffer->bzero();
	__frontend_alternative_buffer->bzero();
	__to_write_buffer_container.reserve(16);
}

//brief:前端向后端写入数据接口
//becare应该保证len==strlen(content)
void AsyncLogger::append(const char* content, size_t len) {
	imnet::MutexUnique_lock lock_logger(__mutex);
	if (__frontend_current_buffer->getUnusedSize() > static_cast<int>(len)) {
		//未写满时，之前丢入到当前缓冲区中
		__frontend_current_buffer->append(content, len);
	}
	else {
		//写满时，需要先通过尝试2种方法获得新的缓冲区，丢入数据后，然后唤醒后端处理
		__to_write_buffer_container.push_back(__frontend_current_buffer.release());
		if (__frontend_alternative_buffer) {
			__frontend_current_buffer.reset(__frontend_alternative_buffer.release());
		}
		else {
			__frontend_current_buffer.reset(new BufferType);
		}
		__frontend_current_buffer->append(content, len);
		//__cond.wait();//BUG
		__cond.notify();
	}
}

//brief:
//becare:会阻塞在这里，等待后端线程结束
AsyncLogger::~AsyncLogger() {
	if (__is_running) {
		stopBackend();
	}
}

//brief:后端处理例程
//     通过bind将this绑定到可调用对象，以此传入Thread
//function:定期或者被前端唤醒，交换缓冲区指针，flush数据到本地文件
//becare:发生日志堆积时，仅冲刷最早的2块缓冲区内容，其他的缓冲区将会丢弃
void AsyncLogger::__backend_work_function() {
	assert(__is_running);

	//后端Thread已经就绪，前端可以写入数据
	__latch.countDown();

	//后端备用缓冲区
	UniquePtr __backend_first_buffer(new BufferType);
	UniquePtr __backend_second_buffer(new BufferType);
	__backend_first_buffer->bzero();
	__backend_second_buffer->bzero();

	//本地输出文件
	//becare:已在外部进行同步，不需要内部加锁
	Logfile local_logfile(__basename, __threshold_roll_file_size, false);

	//减少临界区的范围
	BufferContainerType container;
	container.reserve(16);

	while (__is_running) {

		///////////////////////////
		//////////临界区//////////
		{
			imnet::MutexUnique_lock lock_logger(__mutex);
			if (__to_write_buffer_container.empty()) {
				__cond.timedwait(__flush_interval);
			}

			__to_write_buffer_container.push_back(__frontend_current_buffer.release());
			//brief:交换待写入缓冲区
			container.swap(__to_write_buffer_container);
			__frontend_current_buffer.swap(__backend_first_buffer);
			if (!__frontend_alternative_buffer) {
				__frontend_alternative_buffer.swap(__backend_second_buffer);
			}
		}
		///////////临界区//////////
		///////////////////////////

		assert(!container.empty());

		//brief:处理数据堆积问题
		if (container.size() > 16) {
			//丢弃信息要同时输出到日志和stderr
			//fprintf(stderr, "Some logs is dropped at %s(about %u buffers)",
			//	CTimestamp::getNow().toString().c_str(), container.size() - 2);
			char buf[256];
			
			//asynclogger.cc:122:66: warning: format ‘%u’ expects argument of type ‘unsigned int’,
			//but argument 5 has type ‘size_t {aka long unsigned int}’ [-Wformat=]
			//CTimestamp::getNow().toString().c_str(), container.size() - 2);
			//改用%zu
			snprintf(buf, sizeof buf, "Some logs is dropped at %s(about %zu buffers)\n",
				CTimestamp::getNow().toString().c_str(), container.size() - 2);
			fputs(buf, stderr);
			local_logfile.append(buf, static_cast<int>(strlen(buf)));
		}

		//brief:将数据写入到本地文件
		//becare:此处是可能阻塞的
		for (const BufferPointerType _buf : container) {
			local_logfile.append(_buf->data(), _buf->size());
		}

		//善后工作：恢复缓冲区指针(包括置空数据)，冲刷数据等
		if (container.size() > 2) {
			container.resize(2);
		}

		if (!__backend_first_buffer) {
			assert(!container.empty());
			__backend_first_buffer.reset(container.back());
			container.pop_back();
			__backend_first_buffer->reset();
		}
		
		if (!__backend_second_buffer) {
			assert(!container.empty());
			__backend_second_buffer.reset(container.back());
			container.pop_back();
			__backend_second_buffer->reset();
		}

		container.clear();
		local_logfile.flush();

	}

	//异步日志已经被stop
	local_logfile.flush();
}

}//!namespace imnet
