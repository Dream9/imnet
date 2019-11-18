/*************************************************************************
	> File Name: channel.h
	> Author: Dream9
	> Brief: 封装成selectable channel
	> Created Time: 2019年09月15日 星期日 09时53分22秒
 ************************************************************************/
#ifndef _CORE_CHANNEL_H_
#define _CORE_CHANNEL_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
#include"../../dependent/logger/timestamp.h"
#else 
#include"dependent/noncopyable.h"
#include"dependent/logger/timestamp.h"
#endif

#include<functional>
#include<memory>


namespace imnet {

class CTimestamp;

namespace core {

class Eventloop;//减少依赖

//brief:与poll兼容的事件类型
extern const int kNoneEvent;
extern const int kReadEvent;
extern const int kWriteEvent;

//brief:事件分发器
//     接受用户的回调函数注册，将关心的事件通过EventLoop通知Poller
//     根据实际发生的事件进行不同的回调处理
//becare:对于IO事件的回调强制在指定的EventLoop内完成，以此保证线程安全，
//     本类通常对用户隐藏，一般作为其他高层类的对象成员
//     本类不掌控fd的生命，如果事件处理和fd的生命存在冲突，需要通过weak_ptr管理其
//     生命期，在handleEvent()中尝试提升其控制权
//     此外，每个Channel只能在唯一的一个线程(loop线程)中被使用，这是设计的思想，因此本对象的
//     删除可以无锁的完成，删除的标准流程是:deactiveAll,然后removeFromEventloop
class Channel {
public:
	typedef std::function<void()> EventCallbackType;
	typedef std::function<void(CTimestamp)> ReadEventCallbackType;

	Channel(Eventloop *loop,int fd);
	~Channel();

	//brief:事件分发、注册(可能会注册到Eventloop中)及注销(注册之后的销毁处理)
	void handleEvent(CTimestamp receive_time);
	void removeFromEventloop();

	//brief:设置事件回调
	//becare:IO事件的回调应该位于特定的IO线程之中
	void setReadCallback(ReadEventCallbackType handle_func) {
		__readCallback = std::move(handle_func);
	}
	void setWriteCallback(EventCallbackType handle_func) {
		__writeCallback = std::move(handle_func);
	}
	void setCloseCallback(EventCallbackType handle_func) {
		__closeCallback = std::move(handle_func);
	}
	void setErrorCallback(EventCallbackType handle_func) {
		__errorCallback = std::move(handle_func);
	}
	
	//brief：关注事件状态修改与检查
	//becare:特别是isWriteActivated，由于poller采用了level triger，
	//       读事件只有在有数据等待发送时才需要关注，否则必须主动关闭，防止busy loop
	void activateRead() {
		__events |= kReadEvent;
		__update();
	}
	void deactivateRead() {
		__events &= ~kReadEvent;
		__update();
	}
	void activateWrite() {
		__events |= kWriteEvent;
		__update();
	}
	void deactivateWrite() {
		__events &= ~kWriteEvent;
		__update();
	}
	void deactivateAll() {
		__events = kNoneEvent;
		__update();
	}
	//brief:当前关注的事件
	bool isReadActivated() {
		return __events & kReadEvent;
	}
	bool isWriteActivated() {
		return __events & kWriteEvent;
	}
	bool isNoneActivated() {
		return __events == kNoneEvent;
	}
	//brief:设置当前发生的事件
	//becare:仅由Poller使用
	void setRevents(int events) {
		//__events = events;
		__revents = events;
	}
	//brief:设置索引
	//becare:仅有Poller使用
	void setIndex(int index) {
		__index_at_poller_vector = index;
	}

	//brief:向外界提供channel信息
	int getHandle()const {
		return __fd;
	}
	Eventloop* getEventloop()const {
		return __loop;
	}
	int getIndex()const {
		return __index_at_poller_vector;
	}
	int getEvents()const {
		return __events;//becare:pollfd中的events采用了short存储
	}
	

	//确保处理过程中，fd持续存在，比如延长TcpConnection的生命期
	void tieObjectHandle(const std::shared_ptr<void>& obj);

	//是否记录LogHup事件
	void disableLogHupEvent();
	void enableLogHupEvent();

	string eventsToString()const;
	string reventsToString()const;
	
private:
	void __update();
	void __handleEventWithGuard(CTimestamp receive_time);

	static void __logHup(int fd);
	static void __notlogHup(int fd);

	static string __toString(int fd, int events);

	//Channel所处的Eventloop
	Eventloop *__loop;

	//负责的描述及其分发状态
	const int __fd;
	int __events;
	int __revents;
	
	//与Poller的关联
	int __index_at_poller_vector;

	//必要时需要延长生命期
	std::weak_ptr<void> __weak_object_handle;
	bool __need_ensure_lived;

	void(*__ptr_handle_pollhup)(int);

	//断言:在已注册到Eventloop循环后，handleEvent()处理中，本对象不应被析构
#ifndef NDEBUG
	bool __isRegisteredToEventloop;
	bool __isHandlingEvent;
#endif

	//回调策略
	ReadEventCallbackType __readCallback;
	EventCallbackType __writeCallback;
	EventCallbackType __closeCallback;
	EventCallbackType __errorCallback;

};
}//!namespace core


}//!namespace imnet

#endif
