/*************************************************************************
	> File Name: blocking_queue.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月03日 星期二 09时18分38秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_BLOCKING_QUEUE_H_
#define _IMNET_DEPENDENT_BLOCKING_QUEUE_H_

#ifdef __GNUC__
#incldue"dependent/advanced/mutexlock.h"
#include"dependent/noncopyable.h"
#include"dependent/advanced/condition.h"
#elif defined _MSC_VER
#include"condition.h"
#include"mutexlock.h"
#include"../noncopyable.h"
#endif

#include<deque>
#include<assert.h>

namespace imnet {

//brief:线程安全的阻塞队列
//     可用于任务队列，作为event loop的补充方案
template<typename T,typename ContainerType=std::deque<T>>
class BlockingQueue {
public:
	BlockingQueue()
		:__mutex(), __cond(__mutex),__container() {

	}

	//brief:标准routine
	//brief:取出
	T take() {
		MutexUnique_lock(__mutex);
		while (__container.empty()) {
			__cond.wait();
		}

		T out(std::move(__container.front()));
		__container.pop_front();
		return std::move(out);
	}

	//brief:添加
	void put(const T& v) {
		MutexUnique_lock(__mutex);
		__container.emplace_back(v);
		__cond.notifyall();
	}
	void put(T&& v) {
		MutexUnique_lock(__mutex);
		__container.emplace_back(std::move(v));
		__cond.notifyall();
	}

	//becare:size操作是不被推荐的
	//      既不能向外部暴露mutex，又想获得准确size是冲突的
	size_t size() {
		MutexUnique_lock(__mutex);
		return __container.size();
	}

private:
	//becare:__mutex的声明顺序应该在最前面
	Mutex __mutex;
	Condition __cond;

	ContainerType __container;
};
}



#endif
