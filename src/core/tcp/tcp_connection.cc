/*************************************************************************
	> File Name: tcp_connection.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时33分11秒
 ************************************************************************/
#ifdef _MSC_VER
#include"tcp_connection.h"
#include"../reactor/eventloop.h"
#include"../reactor/channel.h"
#include"../reactor/eventloop_thread_pool.h"
#include"../../dependent/logger/logger.h"
#include"sockaddr.h"
#include"./server_socket.h"
#include"socket_extension.h"
#include"../../dependent/string_piece.h"
#include"../../dependent/weak_callback.h"
#else
#include"core/tcp/sockaddr.h"
#include"core/tcp/server_socket.h"
#include"core/tcp/tcp_connection.h"
#include"core/tcp/socket_extension.h"
#include"core/reactor/eventloop.h"
#include"core/reactor/channel.h"
#include"core/reactor/eventloop_thread_pool.h"
#include"dependent/logger/logger.h"
#include"dependent/weak_callback.h"
#endif

#ifdef _WIN32
#include<io.h>
typedef int ssize_t;
#elif defined __LINUX__
#include<unistd.h>
#endif

namespace imnet {

namespace core {

//brief:简单的记录到日志
void defaultConnectionCallback(const TcpConnectionPtr& tcp) {
	LOG_TRACE << tcp->getLocalAddress().inet_ntop_withport() << "-" << tcp->getPeerAddress().inet_ntop_withport() << ":"
		<< (tcp->isConnected() ? "connected" : "disconnected");
}
//brief:简单的丢弃所有数据
//FIXME:不应将Buffer位于隐藏空间之中
//void defaultMessageCallback(const TcpConnectionPtr& tcp, detail::Buffer* buf, const CTimestamp& timestamp) {
void defaultMessageCallback(const TcpConnectionPtr& tcp, core::Buffer* buf, const CTimestamp& timestamp) {
	buf->retrieveAll();
}

//brief:构建TcpConnection,最主要的是配置事件分发Channel
//becare:在各种回调函数设置完成以及channel读事件激活之前，本类所处的状态为kConnecting
//      全部状态的设置仅在TcpServe中完成
TcpConnection::TcpConnection(Eventloop* loop, const string& name, int sockfd,
							 const Sockaddr& local_address, const Sockaddr& peer_address,
							 ConnectionCallbackType connect_call,
							 MessageCallbackType message_call)
	:__loop(loop),
	__name(name),
	__socket(new ServeSocket(sockfd)),
	__channel(new Channel(loop,sockfd)),
	__local_address(local_address),
	__peer_address(peer_address),
	__state(kConnecting),
	__connect_call(connect_call),
	__message_call(message_call)
{
	//配置事件分发
	__channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
	__channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	__channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
	__channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));

	//全部配置完毕之后方可激活读
	//__channel->activateRead(); 
	__socket->setKeepAlive(true);

	LOG_DEBUG << "TcpConnection:" << name << " fd:" << sockfd << "is working.";
}

//brief:析构，显式放在这里，因为该析构函数会被扩展展开变大
TcpConnection::~TcpConnection() {
	assert(__state == kDisconnected);
}

//brief:获得句柄，放在这里主要是为了不在头文件中额外引入server_socket.h
inline int TcpConnection::getHandle()const {
	return __socket->getHandle();
}

//brief:用于__channel分发处理读事件
//becare:处理读事件的例程必须有能力处理EOF的情况，这是用户的责任，
//      即POLLIN和POLLHUP事件同时发生时，会在handleRead中同时完成handleClose的任务
//      此外还能相应错误处理
void TcpConnection::handleRead(CTimestamp timestamp) {
	__loop->assertInLoopThread();

	int saved_errno = 0;
	int ret = __input_buffer.readFrom(__socket->getHandle(), &saved_errno);

	if (ret > 0) {
		//数据到达
		//becare:__message_call总是存在一份默认实现
		__message_call(shared_from_this(), &__input_buffer, timestamp);
	}
	else if (ret == 0) {
		//EOF
		handleClose();
	}
	else {
		//error
		errno = saved_errno;
		LOG_SYSERR << "TcpConnection:" << __name
			<< " fd:" << __socket->getHandle() << "handleRead failed.";
		handleError();
	}
}

//brief:主要流程为修改TcpConnection的状态，停止Channel的的事件分发(通知Poller)，
//     调用注册的处理函数，该函数通常是被TcpServer设置的，通过bind绑定到了
//     TcpServer::__removerConnectionCallback,它完成相关清理工作(TcpServe部分)并注册调用
//     TcpConncetion::finalCleanupCallback（通过注册，保证对象不会再调用handleRead/handleClose期间销毁）
//     在最后的清理中会调用用户的connectionCallback
//becare:关闭响应处理是不对顶层用户开放的，它的逻辑流程是固定的
void TcpConnection::handleClose() {
	__loop->assertInLoopThread();
	assert(__state == kConnected || __state == kDisconnecting);

	//状态修改会顺带过程调用
	__state = kDisconnected;
	__channel->deactivateAll();
	__connect_call(shared_from_this());

	__close_call(shared_from_this());
}

//breif:处理错误信息
void TcpConnection::handleError() {
	int err = socket_ops::getErrno(__socket->getHandle());
	LOG_ERROR << "TcpConnection:" << __name << ",handleError:"
		<< ::strerror(err);
}

//brief:将写缓冲区的内容发送到对方
//becare:由于采用了水平触发模式，仅在写缓冲区数据存在时才激活关注写事件
//      因此一旦数据发送完毕，应该停止关注，防止轮询
void TcpConnection::handleWrite() {
	__loop->assertInLoopThread();

	if (!__channel->isWriteActivated()) {
		LOG_TRACE << "This connection has been down.";
		return;
	}

	ssize_t ret = socket_ops::write(__socket->getHandle(), __output_buffer.peak(),__output_buffer.getDataSize());

	//FIXME:0不是错误
	//if(ret=0){
	if(ret>=0){
		__output_buffer.retrieve(ret);
		if (__output_buffer.getDataSize() == 0) {
			//写缓冲已经清空
			__channel->deactivateWrite();
			if (__write_complete_call) {
				//触发低水位回调
				//Fixme:使状态的修改必须再本轮任务完成之后
				//__loop->pushBackIntoLoop(std::bind(__write_complete_call, shared_from_this()));
				__loop->pushBackIntoQueue(std::bind(__write_complete_call, shared_from_this()));
			}
			if (__state == kDisconnecting) {
				//根据状态迁移图，当缓冲清空并且处于kDisconnecting时，应主动调用关闭写端
				__shutdownInloop();
			}
		}
		//else {
		//	//仍然未请空写缓冲
		//	//LOG_TRACE << "Data is not cleared";
		//}
	}
	else {
		//写入数据错误
		LOG_SYSERR << "TcpConnection(" << __name << ")handleWrite failed.";
	}
}

//brief：由用户用于主动关闭写端
//becare:他一定会修改连接的状态，但是关闭写端可能会延迟到写缓冲清空之后
void TcpConnection::shutdown() {
	//__loop->assertInLoopThread();
	//assert(__state == kConnected);
	if (__state != kConnected)
		return;

	__setState(kDisconnecting);
	
	__loop->pushBackIntoLoop(std::bind(&TcpConnection::__shutdownInloop, this));
	
}

//brief；在写缓冲为空时关闭写端
void TcpConnection::__shutdownInloop() {
	__loop->assertInLoopThread();

	if (!__channel->isWriteActivated()) {
		//如果当前并没有激活写事件，说明写缓冲为空
		__socket->shutdownWrite();
	}
}

#define ENSURE_TCP_CONNECTED(x) \
	if(x != kConnected){\
		LOG_DEBUG << "TcpConnection:" << __name << " state is " << getStateToString();\
		return;\
	}
#define ENSURE_TCP_CONNECTED_WITH_WARN(x) \
	if(x != kConnected){\
		LOG_WARN << "TcpConnection:" << __name << " state is " << getStateToString()\
				 <<",data is discarded.";\
		return;\
	}

//brief:用户级的发送数据
//becare:确保所有的IO操作只在loop线程中完成即可
void TcpConnection::send(const void* data, size_t len) {
	ENSURE_TCP_CONNECTED(__state);

	if (__loop->isInLoopThread()) {
		__sendInloop(data, len);
	}
	else {
		//FIXME:延后发送数据的话，必须防止数据被析构掉
		void(TcpConnection::*f)(const string&) = &TcpConnection::__sendInloop;
		__loop->pushBackIntoLoop(std::bind(f, this, string(static_cast<const char*>(data), len)));
	}
}

void TcpConnection::send(const CStringPiece& str) {
	return send(str.begin(), str.size());
}

void TcpConnection::send(Buffer* buf) {
	//FIXME:
	send(buf->peak(), buf->getDataSize());
	buf->retrieveAll();
}
//void TcpConnection::send(const string& str) {
//	return send(str.c_str(), str.size());
//}

//brief:在IOloop线程中完成操作
//     在用户设置了高水位与低水位回调后，当缓冲区达到阈值时会触发回调，触发机制为边沿触发
//becare:当此时并没有积压数据时会尝试直接write数据，根据剩余量判断是否激活写关注以及是否追加缓冲区
//     否则，只能追加数据到缓冲区，并保证激活写关注
void TcpConnection::__sendInloop(const void* data, size_t len) {
	__loop->assertInLoopThread();
	ENSURE_TCP_CONNECTED_WITH_WARN(__state);

	size_t data_pos = 0;

	//在没有积压数据时，尝试能否一次write完成数据传送，可以减少handleWrite回调
	if (!__channel->isWriteActivated() && __output_buffer.getDataSize() == 0) {
		ssize_t ret = ::write(__socket->getHandle(), data, len);
		if (ret >= 0) {
			data_pos = static_cast<size_t>(ret);
			if (static_cast<size_t>(ret) == len && __write_complete_call) {
				//数据已经被清空，触发低水位回调:
				//__loop->pushBackIntoLoop(std::bind(__write_complete_call, shared_from_this()));
				//Fixme:保证本轮调整结束后再修改状态
				__loop->pushBackIntoQueue(std::bind(__write_complete_call, shared_from_this()));
			}
		}
		else {
			//write出错，大部分的错误是可以通过积压到缓冲区等待下次调用来处理

			//ERRORS         top
			//EAGAIN The file descriptor fd refers to a file other than a socket
			//and has been marked nonblocking(O_NONBLOCK), and the write
			//would block.See open(2) for further details on the
			//O_NONBLOCK flag.

			//EAGAIN or EWOULDBLOCK
			//The file descriptor fd refers to a socket and has been marked
			//nonblocking(O_NONBLOCK), and the write would block.
			//POSIX.1 - 2001 allows either error to be returned for this case,
			//and does not require these constants to have the same value,
			//so a portable application should check for both possibilities.

			//EBADF  fd is not a valid file descriptor or is not open for writing.

			//EDESTADDRREQ
			//fd refers to a datagram socket for which a peer address has
			//not been set using connect(2).

			//EDQUOT The user's quota of disk blocks on the filesystem containing
			//the file referred to by fd has been exhausted.

			//EFAULT buf is outside your accessible address space.

			//EFBIG  An attempt was made to write a file that exceeds the
			//implementation - defined maximum file size or the process's file
			//size limit, or to write at a position past the maximum allowed
			//offset.

			//EINTR  The call was interrupted by a signal before any data was
			//written; see signal(7).

			//EINVAL fd is attached to an object which is unsuitable for writing;
			//or the file was opened with the O_DIRECT flag, and either the
			//address specified in buf, the value specified in count, or the
			//file offset is not suitably aligned.

			//EIO    A low - level I / O error occurred while modifying the inode.
			//This error may relate to the write - back of data written by an
			//earlier write(), which may have been issued to a different
			//file descriptor on the same file.Since Linux 4.13, errors
			//from write - back come with a promise that they may be reported
			//by subsequent.write() requests, and will be reported by a
			//subsequent fsync(2) (whether or not they were also reported by
			//	write()).An alternate cause of EIO on networked filesystems
			//is when an advisory lock had been taken out on the file
			//descriptor and this lock has been lost.See the Lost locks
			//section of fcntl(2) for further details.

			//ENOSPC The device containing the file referred to by fd has no room
			//for the data.

			//EPERM  The operation was prevented by a file seal; see fcntl(2).

			//EPIPE  fd is connected to a pipe or socket whose reading end is
			//closed.When this happens the writing process will also
			//receive a SIGPIPE signal.  (Thus, the write return value is
			//	seen only if the program catches, blocks or ignores this
			//	signal.)

			//Other errors may occur, depending on the object connected to fd.

			if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
				//除了上述可被忽略的错误，其他错误应该被warning
				LOG_SYSERR << "TcpConnection-" << __name << ":_sendInLoop warning.";
				if (errno == EBADF || errno == EPIPE || errno == ECONNRESET) {
					//这几类错误会直接丢弃数据
					return;
				}
			}
		}
	}

	//根据剩余情况以及缓冲区容量调整回调
	//if (data_pos > 0) {
	if (data_pos - len > 0) {
		size_t bufdata = __output_buffer.getDataSize();
		size_t tmp = bufdata + len - data_pos;
		if (__high_water_call && bufdata < __high_water_threshold && tmp>__high_water_threshold) {
			//这里采用了上升边缘触发的策略
			//Fixme:保证本轮调整结束后再修改状态
			//__loop->pushBackIntoLoop(std::bind(__high_water_call, shared_from_this(), tmp));
			__loop->pushBackIntoQueue(std::bind(__high_water_call, shared_from_this(), tmp));
		}

		__output_buffer.append(static_cast<const char*>(data) + data_pos, len - data_pos);

		if (!__channel->isWriteActivated()) {
			__channel->activateWrite();
		}
	}

}

//brief:转接函数，目的在于防止数据在loop发送之前就被析构，因此需要通过一个temp变量保存在栈上面
//     CStringPiece由于只是提供统一接口而不掌控数据因此也不可以
void TcpConnection::__sendInloop(const string& str) {
	return __sendInloop(str.c_str(), str.size());
}

#undef ENSURE_TCP_CONNECTED
#undef ENSURE_TCP_CONNECTED_WITH_WARN

//brief:获得连接状态
string TcpConnection::getStateToString()const {
	switch (__state) {
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	case kDisconnected:
		return "kDisconnected";
	default:
		return "Unknown";
	}
}

//brief:TcpConnection设置完毕后，完成最终状态调整
void TcpConnection::connectionEstablished() {
	__loop->assertInLoopThread();
	assert(__state == kConnecting);

	__setState(kConnected);
	__channel->tieObjectHandle(shared_from_this());
	__channel->activateRead();

	//调用用户回调
	//__connect_call存在一份默认实现
	__connect_call(shared_from_this());
	//if(__connect_call)
	//	__connect_call(shared_from_this());
}

//brief:断开tcp的最后一个清理例程，理论上它最后一个持有TcpConnectionPtr
//     将Channel对象从loop中清除
//becare:为了防止对象的析构发生在其内部成员函数之前，无标保证本清理例程位于
//     事件处理函数完成之后，即必须通过pushBackIntoQueue注册本清理例程
void TcpConnection::connectionDestroyed() {
	__loop->assertInLoopThread();

	if (__state == kConnected) {
		__state = kDisconnected;
		__channel->deactivateAll();
		__connect_call(shared_from_this());
	}

	__channel->removeFromEventloop();	
}


//brief:强制关闭
void TcpConnection::forceClose() {
	if (__state == kConnected || __state == kDisconnecting) {
		__setState(kDisconnecting);
		//__loop->pushBackIntoLoop(std::bind(&TcpConnection::__forceCloseInloop, shared_from_this());
		//涉及到状态修改，务必保证本轮唤醒调用结束后再被更改，否则导致当前的状态紊乱
		__loop->pushBackIntoQueue(std::bind(&TcpConnection::__forceCloseInloop, shared_from_this()));
	}
}

//brief:延迟执行
//becare:注册一个弱调用，仅当lock成功之后才会执行
void TcpConnection::forceClose(double seconds) {
	if (__state == kConnected || __state == kDisconnecting) {
		__setState(kDisconnecting);
		__loop->runAfter(makeWeakCallback(shared_from_this(), &TcpConnection::forceClose),seconds);
	}
}

//brief:确保再loop线程中主动清理自己
//becare:本对象应该再初始化完毕之后方可被调用
void TcpConnection::__forceCloseInloop() {
	__loop->assertInLoopThread();

	if (__state == kConnected || __state == kDisconnecting) {
		handleClose();
	}
}

//brief：用户主动关闭读关注事件
void TcpConnection::stopRead() {
	__loop->pushBackIntoLoop(std::bind(&TcpConnection::__stopReadInloop, this));
}
void TcpConnection::__stopReadInloop() {
	__loop->assertInLoopThread();

	if (__channel->isReadActivated())
		__channel->deactivateRead();
}

//brief:用户重新开启读关注
void TcpConnection::startRead() {
	__loop->pushBackIntoLoop(std::bind(&TcpConnection::__startReadInloop, this));
}
void TcpConnection::__startReadInloop() {
	__loop->assertInLoopThread();

	if (!__channel->isReadActivated())
		__channel->activateRead();
}

//brief:
void TcpConnection::setTcpNoDelay(bool on) {
	__socket->setTcpNoDelay(on);
}

//brief:
bool TcpConnection::getTcpInfo(struct tcp_info* answer)const {
	return __socket->getTcpInfo(answer);
}



//brief:


}//!namespace core
}//!namespace imnet
