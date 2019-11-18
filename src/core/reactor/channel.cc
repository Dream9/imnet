/*************************************************************************
	> File Name: channel.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 09时53分37秒
 ************************************************************************/
#ifdef _MSC_VER
#include"channel.h"
#include"eventloop.h"
#include"../../dependent/logger/logger.h"
#else 
#include"core/reactor/channel.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#endif

#include<sstream>

#include<poll.h>

namespace imnet {

namespace core {

extern const int kNoneEvent = 0;
extern const int kReadEvent = POLLIN | POLLPRI;
extern const int kWriteEvent = POLLOUT;

extern const int kNew = -1;//导出到EPollPoller之中

Channel::Channel(Eventloop* loop, int fd)
	:__loop(loop),
	__fd(fd),
	__events(0),
	__revents(0),
	__index_at_poller_vector(kNew),
	__need_ensure_lived(false),
	__ptr_handle_pollhup(__logHup)
#ifndef NDEBUG
	, __isRegisteredToEventloop(false),
	__isHandlingEvent(false)
#endif
{

}

//brief:析构，主要是针对生命期的断言，以及确保Eventloop已经不在持有本对象
//becare:设计上一个Channel只用于一个Eventloop,而又同时采用了one loop per thread的多线程模型
//      以此理论上不应出现其他线程持有本对象句柄的可能
Channel::~Channel() {
#ifndef NDEBUG
	assert(!__isHandlingEvent);
	assert(!__isRegisteredToEventloop);
#endif
	if (__loop->isInLoopThread()) {
		assert(!__loop->hasChannel(this));
	}
}

//brief:当处理过程中fd对象的生命期可能短于Channel时，需要用户显示绑定其句柄
void Channel::tieObjectHandle(const std::shared_ptr<void>& obj) {
	__weak_object_handle = obj;
	__need_ensure_lived = true;
}

//brief:将本对象注册到Eventloop,而后者又负责update其持有的Poller对象
void Channel::__update() {
#ifndef NDEBUG
	__isRegisteredToEventloop = true;
#endif
	__loop->updateChannel(this);
}

//brief:关闭注册到Eventloop的channel所关联的fd时进行调用
void Channel::removeFromEventloop() {
	assert(isNoneActivated());
#ifndef NDEBUG
	__isRegisteredToEventloop = false;
#endif
	__loop->removeChannel(this);
}

//brief:在事件分发之前，确保处理的对象仍然存在
//becare:当存在生命期冲突时，需要主动调用tieObjectHandle，默认不存在冲突
void Channel::handleEvent(CTimestamp receive_time) {
	if (__need_ensure_lived) {
		std::shared_ptr<void> guard = __weak_object_handle.lock();
		if (guard) {
			__handleEventWithGuard(receive_time);
		}
	}
	else {
		__handleEventWithGuard(receive_time);
	}
}

#define _IF_EXIST_AND_EXECUTE_(x) \
if(x){\
    x();\
}
#define _IF_EXIST_AND_EXECUTE_WITH_PARAMETER_(x,y) \
if(x){\
    x(y);\
}
#define _HANDLE_EVENT_LOG_FORMAT_(x) "fd= "<< x <<" Channel::__handleEventWithGuard()"
//brief:事件分发
//becare:除了关注的事件之外，有几个事件是默认关注的
//       The field revents is an output parameter, filled by the kernel with
//       the events that actually occurred.The bits returned in revents can
//       include any of those specified in events, or one of the values
//       POLLERR, POLLHUP, or POLLNVAL.  (These three bits are meaningless in
//	     the events field, and will be set in the revents field whenever the
//	     corresponding condition is true.)
void Channel::__handleEventWithGuard(CTimestamp receive_time) {
#ifdef NDEBUG
	__isHandlingEvent = true;
#endif
	LOG_TRACE << reventsToString();

	//先处理错误相关的事件
	if ((__revents & POLLHUP) && !(__revents & POLLIN)) {
		__ptr_handle_pollhup(__fd);
		_IF_EXIST_AND_EXECUTE_(__closeCallback);
	}
	if (__revents & (POLLNVAL | POLLERR)) {
		if (__revents & POLLNVAL) {
			LOG_WARN << _HANDLE_EVENT_LOG_FORMAT_(__fd)"POLLNVAL";
		}
		_IF_EXIST_AND_EXECUTE_(__errorCallback);
	}
	//处理其他事件
	if (__revents &(POLLIN | POLLPRI | POLLRDHUP)) {
		_IF_EXIST_AND_EXECUTE_WITH_PARAMETER_(__readCallback,receive_time);
	}
	if (__revents & POLLOUT) {
		_IF_EXIST_AND_EXECUTE_(__writeCallback);
	}
#ifndef NDEBUG
	__isHandlingEvent = false;
#endif
}

#undef _IF_EXIST_AND_EXECUTE_
#undef _IF_EXIST_AND_EXECUTE_WITH_PARAMETER_
#undef _HANDLE_EVENT_LOG_FORMAT_

//brief:对于POLLHUP事件输出日志的设置及其策略
void Channel::disableLogHupEvent() {
	__ptr_handle_pollhup = Channel::__notlogHup;
}
void Channel::enableLogHupEvent() {
	__ptr_handle_pollhup = Channel::__logHup;
}
inline void Channel::__logHup(int fd) {
	LOG_WARN << "fd:" << fd << " Channel::handle_event() detect POLLHUP";//输出信息
}
inline void Channel::__notlogHup(int fd) {
	;//空操作
}

//brief:分别获得关注事件和已发生事件的string描述
string Channel::eventsToString()const {
	return __toString(__fd, __events);
}
string Channel::reventsToString()const {
	return __toString(__fd, __revents);
}

#define _CHECK_BIT_AND_SET_STRING(x,y,os) if(x & y){os<<#y" ";}
//brief:获得事件的string描述
string Channel::__toString(int fd, int events) {
	std::stringstream os;
	os << fd << ":";
	if(events == kNoneEvent){
		os << "None";
		return os.str();
	}
	_CHECK_BIT_AND_SET_STRING(events, POLLHUP, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLERR, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLNVAL, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLRDHUP, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLIN, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLPRI, os);
	_CHECK_BIT_AND_SET_STRING(events, POLLOUT, os);

	return os.str();
}
#undef _CHECK_BIT_AND_SET_STRING

}//!namespace core

}//!namesapce imnet

