/*************************************************************************
	> File Name: acceptor.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时32分12秒
 ************************************************************************/
#ifdef _MSC_VER
#include"acceptor.h"
#include"sockaddr.h"
#include"../reactor/eventloop.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/acceptor.h"
#include"core/tcp/sockaddr.h"
#include"core/reactor/eventloop.h"
#include"dependent/logger/logger.h"
#endif

#ifdef _WIN32
#include<io.h>
#include<process.h>
#else
#include<unistd.h>
#endif

#include<fcntl.h>

namespace imnet {

namespace core {

Acceptor::Acceptor(Eventloop* loop, const Sockaddr& listen_address, Acceptor::OnConnectionCallbackType call, bool need_reuse_port)
	:__loop(loop),
	__listen_fd(listen_address),
	__new_connection_callback(call),
	__channel(loop,__listen_fd.getHandle()),
	__is_listening(false),
	__idle_fd_for_exhaust(::open("/dev/null",O_RDONLY|O_CLOEXEC)){
	//
	assert(__idle_fd_for_exhaust >= 0);

	__listen_fd.bind(listen_address);//在此处bind,还是放到到listen里面？
	__listen_fd.setReusePort(need_reuse_port);
	__listen_fd.setReuseAddr(true);
	//由上层负责调整
	//__listen_fd.setKeepAlive(true);
	//__listen_fd.setTcpNoDelay(true);

	//注册事件处理例程到__loop之中
	__channel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

//brief:根据地址产生监听描述符，设置部分属性，并提前占据一个描述符的位置
//     启用新的构造函数
Acceptor::Acceptor(Eventloop* loop, const Sockaddr& listen_address) 
	:__loop(loop),
	__listen_fd(listen_address),
	__new_connection_callback(),
	__channel(loop,__listen_fd.getHandle()),
	__is_listening(false),
	__idle_fd_for_exhaust(::open("/dev/null",O_RDONLY|O_CLOEXEC)){
	//
	assert(__idle_fd_for_exhaust >= 0);

	__listen_fd.bind(listen_address);//在此处bind,还是放到到listen里面？
	__listen_fd.setReuseAddr(true);
	//由上层负责调整
	//__listen_fd.setKeepAlive(true);
	//__listen_fd.setTcpNoDelay(true);

	//注册事件处理例程到__loop之中
	__channel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
	//__channel下线的标准流程
	__channel.deactivateAll();
	__channel.removeFromEventloop();

	::close(__idle_fd_for_exhaust);
}

//brief:启动该监听
//becare:确保本函数只在loop线程中使用
void Acceptor::listen() {
	//bind在构造时完成
	//__listen_fd.bind(addr);
	__loop->assertInLoopThread();

	__is_listening = true;
	__listen_fd.listen();
	__channel.activateRead();
}

//brief:__listen_fd可读之后的处理例程
//     正对fd用尽采用预留fd的方式处理之
//becare:确保本函数只在loop线程中使用
//     如果__new_connection_callback存在，那么建立的新连接connect_fd的生命期交由用户定义的函数处理
//     否则会立刻关闭该连接
void Acceptor::handleRead() {
	__loop->assertInLoopThread();

	Sockaddr peer_address;
	int connect_fd = __listen_fd.accept(&peer_address);

	//事件的用户处理分发
	if (connect_fd >= 0) {
		if (__new_connection_callback) {
			__new_connection_callback(connect_fd, peer_address);
		}
		else {
			//上层未指定处理例程
			LOG_WARN << "Unspecified onConnectionCallback handler.";
			::close(connect_fd);
		}
	}
	else {
		//accept失败处理：
		LOG_SYSERR << __FUNCTION__;
		
		//处理描述符用尽的错误
		//EMFILE The per - process limit on the number of open file descriptors
		//has been reached.
		if (errno == EMFILE) {
			::close(__idle_fd_for_exhaust);
			int tmp_connect = ::accept(__listen_fd.getHandle(), NULL, NULL);
			::close(tmp_connect);
			__idle_fd_for_exhaust = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
		}
	}
}

#define GENERATE_CODE(x) \
	void Acceptor::set##x(bool on){\
	__listen_fd.set##x(on);\
	}

GENERATE_CODE(KeepAlive);
GENERATE_CODE(TcpNoDelay);
GENERATE_CODE(ReusePort);

#undef GENERATE_CODE

}//!namespace core
}//!namespace imnet
