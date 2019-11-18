/*************************************************************************
	> File Name: poll_poller.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月07日 星期一 11时27分00秒
 ************************************************************************/
#ifndef _CORE_POLLPOLLER_H_
#define _CORE_POLLPOLLER_H_

#include"poller.h"

//定义位于<poll.h>
struct pollfd;

namespace imnet {

namespace detail {

//brief:采用poll(2)实现的派生
class PollPoller :public core::Poller {
public:
	PollPoller(core::Eventloop* loop);
	~PollPoller() override;

	void updateChannel(core::Channel* channel)override;
	void eraseChannel(core::Channel* channel)override;
	CTimestamp poll(int timeout_microseconds, ChannelList* active_channels) override;

private:
	//将活跃的Channel进行填充
	void __fillActiveChannels(ChannelList* active_channels,int number_happened_events);

	//brief:为了配合poll的结构
	typedef std::vector<struct pollfd> PollfdContainer;
	PollfdContainer __pollfds;
};

}//!namespace detail
}//!namesapce imnet


#endif
