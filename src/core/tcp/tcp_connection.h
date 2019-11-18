/*************************************************************************
	> File Name: tcp_connection.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时33分00秒
 ************************************************************************/
#ifndef _CORE_TCP_CONNECTION_H_
#define _CORE_TCP_CONNECTION_H_

#ifdef _MSC_VER
#include"buffer.h"
#include"callback_types.h"
#include"sockaddr.h"
#include"../../dependent/noncopyable.h"
#include"../../dependent/logger/timestamp.h"
#include"../../dependent/string_piece.h"
#else
#include"core/tcp/buffer.h"
#include"core/tcp/callback_types.h"
#include"core/tcp/sockaddr.h"
#include"dependent/noncopyable.h"
#include"dependent/logger/timestamp.h"
#include"dependent/string_piece.h"
#endif

#include<boost/any.hpp>

#include<memory>

struct tcp_info;

namespace imnet {

namespace core {


class Eventloop;
class Channel;
class ServeSocket;

//brief:用于描述一次连接
//     本类是对用户公开的，用户可以使用本类中的相关函数，完成业务逻辑
//becare:有关CloseCallback的所有函数不期望用户调用
class TcpConnection : noncopyable,
	public std::enable_shared_from_this<TcpConnection>{
public:
	TcpConnection(Eventloop* loop, const string& name, int sockfd,
				  const Sockaddr& local_address, const Sockaddr& peer_address,
			      ConnectionCallbackType connect_call = defaultConnectionCallback,
				  MessageCallbackType message_call = defaultMessageCallback);
	~TcpConnection();

	//
	Eventloop* getEventloop()const {
		return __loop;
	}
	string getName()const {
		return __name;
	}
	int getHandle()const;//beacre:为了减少不必要的头文件引入，置于源文件中inline实现
	//{
	//	return __socket->getHandle();
	//}
	Sockaddr getLocalAddress()const {
		return __local_address;
	}
	Sockaddr getPeerAddress()const {
		return __peer_address;
	}
	bool isConnected()const {
		return __state == kConnected;
	}
	bool isDisconnected()const {
		return __state == kDisconnected;
	}
	bool getTcpInfo(struct tcp_info*)const;
	
	//用户写接口
	//void send(const string& str);//因为存在string到CStringPiece的转换,这里存在歧义
	//
	//TODO:将buf直接传入
	void send(core::Buffer* buf);
	void send(const CStringPiece& str);
	void send(const void* message, size_t len);

	void shutdown();//读关闭
	void forceClose();//强制关闭
	void forceClose(double seconds);

	void setTcpNoDelay(bool on);//禁用Negal
	
	//用户控制读关注，从而控制流量
	void stopRead();
	void startRead();

	void setContext(const boost::any& data) {
		__context = data;
	}
	const boost::any& getContext()const {
		return __context;
	}
	//brief:便于修改参数，
	boost::any* getContextPtr() {
		return &__context;
	}

	string getStateToString()const;


	//brief:用户设置回调例程
	//becare:ConnectionCallback会在连接建立以及关闭时调用(作为分发处理流程的一部分)，
	//     注意用户不可修改流程，这是其唯一的注入接口，因此此函数期待能够同时处理
	//     连接建立以及关闭状态
    void setConnectionCallback(ConnectionCallbackType call) {
		__connect_call = call;
	}
	void setWriteCompleteCallback(WriteCompleteCallbackType call) {
		__write_complete_call = call;
	}
	void setErrorCallback(ErrorCallbackType call) {
		__error_call = call;
	}
	void setHighWaterCallback(HighWaterCallbackType call, size_t threshold) {
		__high_water_call = call;
		__high_water_threshold = threshold;
	}
	void setMessageCallback(MessageCallbackType call) {
		__message_call = call;
	}


	//becare:Close事件的处理是对顶层用户隐藏的，它具有自己固定的流程
	//      本函数只期待被TcpServer/TcpClient调用，用户禁止使用!!!!!!!!
	void setCloseCallback(CloseCallbackType call) {
		__close_call = call;
	}

	//becare:本函数只期待被TcpServer/TcpClient调用一次，用户不应该直接使用
	void connectionEstablished();
	//becare:本函数只期待被TcpServer/TcpClinet调用一次，用户不应该直接使用
	void connectionDestroyed();


private:
	//brief:连接状态
	enum State {
		kConnecting,//从对象生成到__channel激活读关注中间的过程
		kConnected,//激活读关注之后，未调用shutdown或者handleClose之前
		kDisconnecting,//用户主动调用shutdown之后，将会尝试关闭写端，或推迟到写缓冲清空后关闭写端
		kDisconnected//对方主动关闭或者读到EOF或者强制关闭之后，本对象的状态
	};

	//brief:配合Channel的事件分发
	//becare:handleClose()并没有向用户开放接口
	void handleRead(CTimestamp timestamp);//becare:应该具有处理EOF和Error的能力
	void handleWrite();
	void handleClose();
	void handleError();

	void __setState(State state) {
		__state = state;
	}

	//brief:用户主动关闭
	//becare:会将本对象的状态改变为kDisconnecting
	void __shutdownInloop();//等待数据全部写出后关闭
	void __forceCloseInloop();//下一轮loop中会强制关闭，指清理本对象，会导致对方收到RST/FIN

	//brief:保证在IO线程中发送数据
	void __sendInloop(const void* data, size_t len);
	void __sendInloop(const string& str);//保证数据不被析构

	void __stopReadInloop();
	void __startReadInloop();

	Eventloop* __loop;
	string __name;
	std::unique_ptr<ServeSocket> __socket;
	std::unique_ptr<Channel> __channel;

	const Sockaddr __local_address;
	const Sockaddr __peer_address;

	//状态
	State __state;

	//缓冲区
	Buffer __input_buffer;
	Buffer __output_buffer;

	//回调
	ConnectionCallbackType __connect_call;
	MessageCallbackType __message_call;
	CloseCallbackType __close_call;//becare:不期望用户修改
	WriteCompleteCallbackType __write_complete_call;
	ErrorCallbackType __error_call;
	HighWaterCallbackType __high_water_call;

	//int __high_water_threshold;
	size_t __high_water_threshold;
	boost::any __context;//可以用来存储与TcpConnection绑定的特定数据，比如客户id等信息

};

}
}

#endif
