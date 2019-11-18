/*************************************************************************
	> File Name: poller.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年09月15日 星期日 09时54分14秒
 ************************************************************************/
#ifndef _CORE_POLLER_H_
#define _CORE_POLLER_H_

#ifdef _MSC_VER
#include"../../dependent/noncopyable.h"
//#include"../../dependent/logger/timestamp.h"
//#include"eventloop.h"
#else
#include"dependent/noncopyable.h"
#include"dependent/logger/timestamp.h"
#include"core/reactor/eventloop.h"
#endif

#include<map>
#include<vector>

namespace imnet {

class CTimestamp;

namespace core {

class Eventloop;
class Channel;

//brief:多路复用的封装
//     本类作为一种抽象基类，定义了基本接口
//     主要包括更新、删除、定位、多路复用的接口
//     同时提供一个静态成员函数负责从环境变量中获取指定的具体Poller实现
class Poller :noncopyable {
public:
	typedef std::vector<Channel*> ChannelList;
	Poller(Eventloop* loop);
	virtual ~Poller();

	//brief:api
	virtual void updateChannel(Channel* channel) = 0;
	virtual void eraseChannel(Channel* channel) = 0;
	virtual bool hasChannel(Channel* channel)const;
	virtual CTimestamp poll(int timeout_microseconds, ChannelList* activeChannels) = 0;//单位毫秒
	static Poller* getPollerFromEnvironment(Eventloop* loop);

	void assertInLoopThread()const;

protected:
	//为了能够使得子类可以访问
	typedef std::map<int, Channel*> ContainerType;
	ContainerType __channels;

	Eventloop* __loop;
};

}//!namespace core
}//!namespace imnet


#endif
