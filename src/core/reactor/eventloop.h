/*************************************************************************
	> File Name: eventloop.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 09时51分30秒
 ************************************************************************/
#ifndef _CORE_EVENTLOOPl_H_
#define _CORE_EVENTLOOPl_H_

#ifdef _MSC_VER
#include"../../dependent/thread_local_variable.h"
#include"../../dependent/logger/timestamp.h"
#include"../../dependent/advanced/mutexlock.h"
#include"timerwork_id.h"
#else
#include"dependent/thread_local_variable.h"
#include"dependent/logger/timestamp.h"
#include"dependent/advanced/mutexlock.h"
#include"core/reactor/timerwork_id.h"
#endif

#include<functional>
#include<vector>
#include<atomic>
#include<memory>

#include<boost/any.hpp>

#ifdef _WIN32
typedef long pid_t;
#endif

namespace imnet {

namespace core {

class Poller;
class Channel;
class TimerworkQueue;

//brief:one loop per thread中的loop封装
//     负责Reactor的总体事件检测与处理
//     其
class Eventloop{
public:
	typedef std::function<void()> CallbackFunctionType;

	Eventloop();
	~Eventloop();

	//brief:控制循环
	void loop();
	void quit();

	//brief:工作注入接口
	//becare:确保线程安全
	void pushBackIntoLoop(CallbackFunctionType);//有可能立刻执行
	void pushBackIntoQueue(CallbackFunctionType);//会等到本次调用完成之后，下一轮唤醒执行
	void wakeupEventloop();
	size_t getQueueSize()const;
	
	//向IO线程中注入/清除定时任务
	TimerworkId runAt(CallbackFunctionType work, CTimestamp when);
	TimerworkId runEvery(CallbackFunctionType work, double interval_seconds);
	TimerworkId runAfter(CallbackFunctionType work, double seconds);
	void cancel(TimerworkId* timerwork_id);
	void cancel(TimerworkId timerwork_id);

	//将Channel信息更新到Poller
	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);
	bool hasChannel(Channel* channel);

	//brief:确保在同一个IO线程内完成任务
	bool isInLoopThread()const {
		return __thread_id == thread_local_variable::gettid();
	}
	void assertInLoopThread() {
		if (!isInLoopThread()) {
			__abortForOutOfLoopThread();
		}
	}

	int64_t getLoopIterationCount() const {
		return __loop_iteration_count;
	}

	bool isHandlingEvent()const {
		return __is_handling_event;
	}

	//brief:设置以及获取环境上下文
	void setContext(const boost::any& context) {
		__context = context;
	}
	void setContext(const boost::any&& context) {
		__context = std::move(context);
	}

	const boost::any& getContext()const {
		return __context;
	}
	boost::any* getMutableContext() {
		return &__context;
	}

	//brief:one loop per thread的另一个角度
	static Eventloop* getCurrentThreadEventloopHandler();

private:
	typedef std::vector<Channel*> ChannelContainerType;

	void __abortForOutOfLoopThread();
	void __debugActiveChannelInformation();

	void handleRead();//负责处理__wakeup_loop_fd产生的事件
	void __processPendingWork();//处理注册的任务

	//brief:状态信息
	pid_t __thread_id;
	bool __is_looping;
	bool __is_handling_event;
	bool __is_handling_work_queue;
	std::atomic_bool __need_quit_loop;

	int64_t __loop_iteration_count;
	CTimestamp __poll_return_time;

	boost::any __context;//辅助用户，可以存储任意类型数据的上下文环境

	//brief:复合
	std::unique_ptr<Poller> __poller;//负责等待事件
	std::unique_ptr<TimerworkQueue> __timer_work_queue;//负责超时工作队列

	int __wakeup_loop_fd;
	std::unique_ptr<Channel> __wakeup_loop_channel;//响应第三方唤醒

	ChannelContainerType __activeChannels;//由Poller负责维护的当前事件集合
	Channel* __current_working_on_channel;

	mutable Mutex __mutex;//保护__pending_work
	std::vector<CallbackFunctionType> __pending_work;
};


}//!namespace core

}//!namespace imnet


#endif
