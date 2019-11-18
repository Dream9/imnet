/*************************************************************************
	> File Name: poller.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 09时54分22秒
 ************************************************************************/

#ifdef _MSC_VER
#include"poller.h"
#include"../../dependent/logger/timestamp.h"
#include"eventloop.h"
#include"channel.h"
#include"poll_poller.h"
#include"epoll_poller.h"
#else
#include"core/reactor/poller.h"
#include"dependent/logger/timestamp.h"
#include"core/reactor/eventloop.h"
#include"core/reactor/channel.h"
#include"core/reactor/poll_poller.h"
#include"core/reactor/epoll_poller.h"
#endif

#include<cstdlib>

namespace imnet {

namespace core {


Poller::Poller(Eventloop* loop) :__loop(loop){
}
Poller::~Poller() = default;

//brief:在eventloop线程中判断是否存在
bool Poller::hasChannel(Channel* channel)const {
	assertInLoopThread();
	assert(channel);
	assert(channel->getEventloop() == Poller::__loop);

	auto iter = __channels.find(channel->getHandle());
	return iter != __channels.end() && iter->second==channel;
}

inline void Poller::assertInLoopThread()const {
	__loop->assertInLoopThread();
}

//brief:根据环境变量选择对应的多路复用的方式
Poller* Poller::getPollerFromEnvironment(Eventloop* loop) {
	if (::getenv("IMNET_CORE_USE_POLL")) {
		return new detail::PollPoller(loop);
	}
	else {
		return new detail::EPollPoller(loop);
	}
}

}//!namespace core
}//!namespace imnet




