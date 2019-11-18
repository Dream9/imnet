/*************************************************************************
	> File Name: epoll_poller.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月07日 星期一 11时27分50秒
 ************************************************************************/
#ifndef _CORE_EPOLL_POLLER_H_
#define _CORE_EPOLL_POLLER_H_

#ifdef _MSC_VER
#include"poller.h"

#else
#include"core/reactor/poller.h"
#endif

struct epoll_event;

namespace imnet {

namespace detail {

//brief:通过epoll作为底层多路复用的Poller派生策略
class EPollPoller :public core::Poller {
public:
	EPollPoller(core::Eventloop* loop);
	~EPollPoller() override;

	void updateChannel(core::Channel* channel) override;
	void eraseChannel(core::Channel* channel) override;
	CTimestamp poll(int timeout_microseconds, ChannelList* active_channels)override;

private:
	void __fillActiveChannels(ChannelList* active_channels, int number_of_events);
	void __epollCtl(core::Channel* channel, int operation);

	int __epollfd;

	//brief:负责记录epoll_wait的返回结果
	typedef std::vector<struct epoll_event> EpolleventContainer;
	EpolleventContainer __epoll_events;
};

}
}

#endif
