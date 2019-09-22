/*************************************************************************
	> File Name: asynclogger.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月08日 星期日 16时10分20秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_ASYNCLOGGER_H_
#define _IMNET_DEPENDENT_ASYNCLOGGER_H_

#ifdef _MSC_VER
#include"../advanced/condition.h"
#include"../advanced/bound_blocking_queue.h"
#include"../advanced/countdown_latch.h"
#include"../advanced/mutexlock.h"
#include"../advanced/thread_extension.h"
#include"log_stream.h"
#else
#include"dependent/advanced/mutexlock.h"
#include"dependent/advanced/condition.h"
#include"dependent/advanced/countdown_latch.h"
#include"dependent/advanced/thread_extension.h"
#include"dependent/logger/log_stream.h"
#endif

#include<boost/ptr_container/ptr_vector.hpp>
#include<vector>
#include<algorithm>
#include<atomic>

namespace imnet {

//brief:实例化声明，减少代码膨胀
//      实例化定义位于log_stream.cc
extern template class imnet::detail::CFixedBuffer<imnet::detail::kLargeBuffer>;

//brief:对外隐藏
namespace detail
{

template<typename T>
class ptr_vector {
public:
	typedef std::vector<T*> ContainerType;
	typedef typename ContainerType::value_type PointerType;
	typedef typename ContainerType::iterator iterator;

	//brief:自动释放所有资源
	~ptr_vector() {
		std::for_each(__data.begin(), __data.end(), [](T *cur) {delete cur; });
	}
	//brief:其他的routine直接转调
	void push_back(T *value) { __data.emplace_back(value); }
	void pop_back() { __data.pop_back(); }
	void clear() { __data.clear(); }
	bool empty()const { return __data.empty(); }
	void swap(ptr_vector &other) { __data.swap(other.__data); }
	void reserve(size_t len) { __data.reserve(len); }
	void resize(size_t len) { __data.resize(len); }
	size_t size()const { return __data.size(); }
	iterator begin() { return __data.begin(); }
	iterator end() { return __data.end(); }
	T*& back() { return __data.back(); }
	T*& front() { return __data.front(); }




private:
	std::vector<T*> __data;
};

}//！namespace detail

//brief:异步日志设施
//      分为前后端工作，使用双缓冲实现，数据交换通过缓冲指针交换完成
//      前端写满时，通知后端写入信息
//      后端会定期唤醒，flush数据
//      消息的堆积会导致后端仅保留最早一部分日志数据
class AsyncLogger :noncopyable {
public:
	AsyncLogger(const string& basename, int threshold_roll_file_size, int flush_interval=3);
	~AsyncLogger();

	void append(const char *content, size_t len);

	void startBackend() {
		__is_running = true;
		__backend_thread.start();
		__latch.wait();//等待后端初始化完成
	}

	//brief:唤醒后端作最后处理，并等待其结束
	void stopBackend() {
		__is_running = false;
		__cond.notify();
		__backend_thread.join();
	}

private:
	void __backend_work_function();

	typedef imnet::detail::CFixedBuffer<imnet::detail::kLargeBuffer> BufferType;
	typedef detail::ptr_vector<BufferType> BufferContainerType;
	typedef BufferContainerType::PointerType BufferPointerType;
	typedef std::unique_ptr<BufferType> UniquePtr;

	//与异步日志相关的状态
	std::atomic_bool __is_running;
	const int __flush_interval;

	//与冲刷文件相关的状态
	const string __basename;
	const off_t __threshold_roll_file_size;

	//前后端同步变量
	Thread __backend_thread;
	CountdownLatch __latch;
	Mutex __mutex;
	Condition __cond;
	
	//缓冲区
	UniquePtr __frontend_current_buffer;
	UniquePtr __frontend_alternative_buffer;
	BufferContainerType __to_write_buffer_container;

};

}//!namespace imnet



#endif
