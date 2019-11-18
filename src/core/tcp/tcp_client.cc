/*************************************************************************
	> File Name: tcp_client.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时33分48秒
 ************************************************************************/
#ifdef _MSC_VER
#include"tcp_client.h"
#include"tcp_connection.h"
#include"connector.h"
#include"../reactor/eventloop.h"
#include"../reactor/channel.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/tcp_client.h"
#include"core/tcp/tcp_connection.h"
#include"core/tcp/connector.h"
#include"core/reactor/eventloop.h"
#include"core/reactor/channel.h"
#include"dependent/logger/logger.h"
#endif

#include<limits>

namespace imnet {

namespace detail{

//brief:替代TcpConnection::__removeConnection，防止生命期的冲突
void removeConnection(core::Eventloop* loop, const core::TcpConnectionPtr& con){
	loop->pushBackIntoQueue(std::bind(&core::TcpConnection::connectionDestroyed, con));
}

}//!namesapce detail

namespace core {

TcpClient::TcpClient(Eventloop* loop, const Sockaddr& address, const string& name)
	:__loop(loop),
	__basename(name),
	__server(address),
	__count(1),
	__connector(new Connector(loop,address)),
	__need_connect(true),
	__need_retry(false),
	__on_connect(defaultConnectionCallback),//采用默认的处理方式
	__on_message(defaultMessageCallback)//default
{
	__connector->setConnectionCallback(std::bind(&TcpClient::__newConnection, this, std::placeholders::_1));
	LOG_TRACE << "TcpClient:" << __basename << "ctor";
}

//brief:清理工作
//becare：这里的清理要比TcpServer复杂，因为后者的生命期一定大于等于其管理的TcpConnection,
//     而对于TcpClient而言，则不一定：1.连接不一定已经建立 2.连接不一定没有被其他对象持有
//     相关的处理主要是延缓析构发生/不再使用析构对象
TcpClient::~TcpClient(){
	TcpConnectionPtr con;
	bool unique = true;
	{
		MutexUnique_lock lock(__mutex);
		con = __connection;
		unique = con.use_count() == 1 ? true : false;

	}

	if(con){
		//brief:处理连接已经建立的情况
		//becare:根据TcpConnection 关闭的流程图，会调用TcpClient的__removeConnection函数
		//       需要重新调整流程
		CloseCallbackType new_call = 
			std::bind(detail::removeConnection, __loop, std::placeholders::_1);
		__loop->pushBackIntoLoop(std::bind(&TcpConnection::setCloseCallback, con, new_call));

		if(unique){
			//brief:这里采用了弱管理的方式，即TcpConnection 的生命可以超过TcpClient
			con->forceClose();
		}
	}
	else{
		//brief:处理连接尚未建立的情况
		//becare:由于调用Connector::stop可能导致TcpClient析构导致Connector也被清理，因此需要延缓析构
		__connector->stop(__connector);
	}
}

//brief:开始连接
//becare:race condition
void TcpClient::connect() {
	__need_connect.store(true);

	//{
	//	MutexUnique_lock lock(__mutex);
	//	if (__connection || __connector->isConnected()) {
	//		LOG_TRACE << "TcpClient:" << __basename << "has been connected.";
	//		return;
	//	}
	//}

	//brief:对于已经建立的连接，start的调用是无效的
	__connector->start();
}

//brief:停止建立连接
void TcpClient::stopconnect() {
	__need_connect.store(false);
	__connector->stop();
}

//brief:断开已有的连接
//becare:race condition
void TcpClient::disconnect() {
	__need_connect.store(false);
	{
		MutexUnique_lock lock(__mutex);
		if (__connection)
			__connection->shutdown();
	}
}

//brief:
void TcpClient::redirect(const Sockaddr& new_address) {
	LOG_WARN << "TODO";
	//TODO:
}

//brief:本函数会被传递给Connector,负责接管sockfd，并封装为TcpConnection
//becare:不同于TcpServer需要选择一个工作线程，本函数采用的线程即为本对象所属的线程
void TcpClient::__newConnection(int sockfd) {
	__loop->assertInLoopThread();

	//获得名称
	char buf[32];
	static_assert(sizeof buf>= std::numeric_limits<int>::digits10 + 2, "name buf is not enough.");

	snprintf(buf,sizeof buf, "#%d", __count);
	++__count;
	string name = __basename + buf;

	//获得地址
	Sockaddr local_address(socket_ops::getLocalSockaddrFromFd(sockfd));
	Sockaddr peer_address(socket_ops::getPeerSockaddrFromFd(sockfd));

	//封装为TcpConnection
	TcpConnectionPtr con = std::make_shared<TcpConnection>(__loop, name,
		sockfd, local_address, __server);
	LOG_INFO << "TcpClient:" << __basename << "establishes connect:" << name << ",local:"
		<< local_address.inet_ntop_withport() << ",peer:" << peer_address.inet_ntop_withport();

	//设置连接属性
	con->setConnectionCallback(__on_connect);
	con->setWriteCompleteCallback(__on_write_complete);
	con->setMessageCallback(__on_message);
	con->setCloseCallback(std::bind(&TcpClient::__removeConnection, this, std::placeholders::_1));

	//由本对象接管其生命
	{
		MutexUnique_lock lock(__mutex);
		__connection = con;
	}
	//becare:不须选择线程，故可以完成连接
	con->connectionEstablished();
}

//brief:TcpConnection关闭时清理流程的一部分，负责清理在TcpClient中的相关信息
//     当__need_retry被置位并且__need_connect依然有效时，业务交互尚未完成，需要自动重连
//becare:无论自动重连与否，无标保证connection的析构在其成员函数调用之后
void TcpClient::__removeConnection(const TcpConnectionPtr& connection) {
	__loop->assertInLoopThread();
	assert(connection->getEventloop() == __loop);

	{
		MutexUnique_lock lock(__mutex);
		__connection.reset();
	}

	//brief:保证析构发生在外部
	__loop->pushBackIntoQueue(std::bind(&TcpConnection::connectionDestroyed, connection));

	//自动重连，处理意外掉线等情况
	if (__need_connect && __need_retry) {
		//
		//__loop->pushBackIntoLoop(std::bind(&TcpClient::connect, this));
		//brief:首次connect导致Connector::start调用，自动重连时，需要调用restartInLoop
		//becare:
		LOG_INFO << "TcpClient:" << __basename << "will re-connect to " << __server.inet_ntop_withport();
		__connector->restartInLoop();
	}


}


}//!namespace core
}//!namespace imnet
