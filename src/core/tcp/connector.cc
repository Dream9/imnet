/*************************************************************************
	> File Name: connector.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019��10��29�� ���ڶ� 15ʱ08��32��
 ************************************************************************/

#ifdef _MSC_VER
#include"connector.h"
#include"socket_extension.h"
#include"../reactor/eventloop.h"
#include"../reactor/channel.h"
#include"../../dependent/logger/logger.h"
#include"../../dependent/weak_callback.h"
#else
#include"core/tcp/connector.h"
#include"core/tcp/socket_extension.h"
#include"core/reactor/eventloop.h"
#include"core/reactor/channel.h"
#include"dependent/logger/logger.h"
#include"dependent/weak_callback.h"
#endif


#include<errno.h>

namespace imnet {

namespace detail{

//brief:
void delayDtor(const core::ConnectorPtr& connector){
	;//ֻ��Ϊ���ӻ�connector����������
}

}//!namespace detail

namespace core {

//brief:�˱�ʱ�䳣������λ����
const int Connector::kMaxBackoff = 500 * 128;//��ʱ7�κ�ﵽ��ֵ
const int Connector::kInitBackoff = 500;

//brief:
Connector::Connector(Eventloop* loop, const Sockaddr& address)
	:__loop(loop),
	__serve_address(address),
	__state(kDisconnected),
	__backoff(kInitBackoff),
	__need_connecting(false)
{
}

//brief:
//becare:�������������֮ǰ���зַ������ᵼ��
Connector::~Connector() {
	if (__channel) {
		LOG_ERROR << "Connector is destroyed while its Channel still exists";
	}
}

//brief:ֹͣ�������ӵĳ���
//becare:�����������ڲ�ͬ�߳��е���
//      bing�޷��Ƶ������������أ���˱�����ʾ�ƶ�
void Connector::stop() {
	__need_connecting.store(false);
	//becare������Ҫ�ƶ�this�ڲ����б���
	//__loop->pushBackIntoLoop(std::bind((void(Connector::*)(Connector*))&Connector::__stopInLoop, this));
	__loop->pushBackIntoLoop(std::bind((void(Connector::*)())&Connector::__stopInLoop, this));
}
void Connector::stop(const ConnectorPtr& self) {
	__need_connecting.store(false);
	__loop->pushBackIntoLoop(
			std::bind((void(Connector::*)(const ConnectorPtr&))&Connector::__stopInLoop, this, self));
}
//beace:�������ǰ�ȫ��
void Connector::__stopInLoop() {
	__loop->assertInLoopThread();
	
	if (__state == kConnecting) {
		__setState(kDisconnected);
		int sockfd = __getFdAndRestChannel();
		socket_ops::close(sockfd);
	}
}
void Connector::__stopInLoop(const ConnectorPtr& self) {
	__loop->assertInLoopThread();
	
	if (__state == kConnecting) {
		__setState(kDisconnected);
		int sockfd = __getFdAndRestChannel();
		socket_ops::close(sockfd);
	}

	//brief:�ӻ����������
	__loop->pushBackIntoQueue(std::bind(detail::delayDtor, self));
}
//brief����ʼ��Ŀ���ַ��������
//becare:�ú��������ڲ�ͬ�߳��е���
void Connector::start() {
	__need_connecting = true;
	__loop->pushBackIntoLoop(std::bind(&Connector::__startInLoop, this));
}
//becare:restart�����״̬,��˱���ֻ����IO�߳��е���
void Connector::restartInLoop() {
	__loop->assertInLoopThread();

	__need_connecting = true;
	__setState(kDisconnected);
	__backoff = kInitBackoff;

	__loop->pushBackIntoLoop(std::bind(&Connector::__startInLoop, this));
}
//becare:�����ж��ٸ��̵߳�����start,__startInLoopҲ��˳��ִ�еģ������ھ�������
void Connector::__startInLoop() {
	__loop->assertInLoopThread();

	//��ֹ����߳�start��������
	if (!isDisconnected())
		return;

	if (false == __need_connecting) {
		LOG_INFO << "Quit connecting";
		return;
	}

	__connect();
}
void Connector::__connect(){

	int sockfd = socket_ops::createNonblockingTcpSocket(__serve_address.getFamily());
	int ret = socket_ops::connect(sockfd, __serve_address.castToSockaddrPtr());
	int saved_errno = ret == 0 ? 0 : errno;

	//������
//	ERRORS         top
//		The connect() function shall fail if:
//
//	EADDRNOTAVAIL
//	The specified address is not available from the local machine.
//
//	EAFNOSUPPORT
//	The specified address is not a valid address for the address
//	family of the specified socket.
//
//	EALREADY
//	A connection request is already in progress for the specified
//	socket.
//
//	EBADF  The socket argument is not a valid file descriptor.
//
//	ECONNREFUSED
//	The target address was not listening for connections or
//	refused the connection request.
//
//	EINPROGRESS
//	O_NONBLOCK is set for the file descriptor for the socket and
//	the connection cannot be immediately established; the
//	connection shall be established asynchronously.
//
//	EINTR  The attempt to establish a connection was interrupted by
//	delivery of a signal that was caught; the connection shall be
//	established asynchronously.
//
//	EISCONN
//	The specified socket is connection - mode and is already
//	connected.
//
//	ENETUNREACH
//	No route to the network is present.
//
//	ENOTSOCK
//	The socket argument does not refer to a socket.
//
//	EPROTOTYPE
//	The specified address has a different type than the socket
//	bound to the specified peer address.
//
//	ETIMEDOUT
//	The attempt to connect timed out before a connection was made.
//
//	If the address family of the socket is AF_UNIX, then connect() shall
//	fail if:
//
//	EIO    An I / O error occurred while reading from or writing to the
//		file system.
//
//		ELOOP  A loop exists in symbolic links encountered during resolution
//		of the pathname in address.
//
//		ENAMETOOLONG
//		The length of a component of a pathname is longer than
//	{ NAME_MAX }.
//
//		ENOENT A component of the pathname does not name an existing file or
//		the pathname is an empty string.
//
//		ENOTDIR
//		A component of the path prefix of the pathname in address
//		names an existing file that is neither a directory nor a
//		symbolic link to a directory, or the pathname in address
//		contains at least one non - <slash> character and ends with one
//		or more trailing <slash> characters and the last pathname
//		component names an existing file that is neither a directory
//		nor a symbolic link to a directory.
//
//		The connect() function may fail if:
//
//	EACCES Search permission is denied for a component of the path
//	prefix; or write access to the named socket is denied.
//
//	EADDRINUSE
//	Attempt to establish a connection that uses addresses that are
//	already in use.
//
//	ECONNRESET
//	Remote host reset the connection request.
//
//	EHOSTUNREACH
//	The destination host cannot be reached(probably because the
//		host is down or a remote router cannot reach it).
//
//	EINVAL The address_len argument is not a valid length for the address
//	family; or invalid address family in the sockaddr structure.
//
//	ELOOP  More than{ SYMLOOP_MAX } symbolic links were encountered during
//	resolution of the pathname in address.
//
//	ENAMETOOLONG
//	The length of a pathname exceeds{ PATH_MAX }, or pathname
//	resolution of a symbolic link produced an intermediate result
//	with a length that exceeds{ PATH_MAX }.
//
//	ENETDOWN
//	The local network interface used to reach the destination is
//	down.
//
//	ENOBUFS
//	No buffer space is available.
//
//	EOPNOTSUPP
//	The socket is listening and cannot be connected.

	switch (saved_errno) {
		//��Ҫ��һ������
	case 0://none errno
	case EINTR://shall be established asynchronously.
	case EINPROGRESS://shall be established asynchronously.
	case EISCONN://has been established
		__connecting(sockfd);
		break;

		//��Ҫָ����������
	case EAGAIN://�˿��þ�
	case EADDRINUSE://address in use
	case EADDRNOTAVAIL://is not available from the local machine
	case ECONNREFUSED://target is not listening or refused th request
	case ENETUNREACH ://no route to the network
		__retryConnect(sockfd);
		break;

		//�޷���ȵĴ���
		//TODO:ϸ�ִ���
	default:
		LOG_SYSERR << "Connector connect() failed";
		socket_ops::close(sockfd);
		break;
	}
}

//brief:�ȴ����ӿ�д�����Խ�������������һ���ļ���
void Connector::__connecting(int sockfd) {
	assert(!__channel);

	__setState(kConnecting);
	__channel.reset(new Channel(__loop, sockfd));

	__channel->setErrorCallback(std::bind(&Connector::handleError, this));
	__channel->setWriteCallback(std::bind(&Connector::handleWrite, this));

	__channel->activateWrite();
}

//brief:�ӳ�������ָ���˱�
void Connector::__retryConnect(int sockfd) {
	socket_ops::close(sockfd);
	__setState(kDisconnected);

	if (!__need_connecting) {
		LOG_INFO << "Quit connecting";
		return;
	}
	LOG_INFO << "Connector will retry connect to " << __serve_address.inet_ntop_withport()
		<< " in " << __backoff << " ms";

	__loop->runAfter(makeWeakCallback(shared_from_this(),&Connector::__startInLoop), __backoff/1000.0);
	__backoff = std::min(kMaxBackoff, __backoff << 1);
}

//brief:Channel��׽��������Ҫ����Channel���󣬲�ͨ���ӳ����������������
//becare:������δ���Channel����Ӧ�ñ�����
//      ����Channel�¼����û���stop�������첽�ģ����ע�⴦���ͻ
void Connector::handleError() {
	if (!__need_connecting || __state==kDisconnected) {
		//�û��ڱ�ֹͣ����������
		LOG_TRACE << "User quits the connection request";
		//�����һ����������__need_connecting�ѱ���Ϊfalse,retryʱ���Լ�����
	}

	int sockfd = __getFdAndRestChannel();
	int err = socket_ops::getErrno(sockfd);
	LOG_TRACE << "Connector's Channel caught error:" << err << ::strerror(err);

	//__setState(kDisconnected);
	__retryConnect(sockfd);
}

//brief:�ڽ��������ӿ�д����״̬����������__on_connect,�������ӵĿ���Ȩ�͸��ϲ�
//becare:������δ���Channel����Ӧ�ñ�����
void Connector::handleWrite() {
	if (!__need_connecting || __state==kDisconnected) {
		//�û��ڱ�ֹͣ����������
		LOG_TRACE << "User quits the connection request";
		return;
	}

	int sockfd = __getFdAndRestChannel();//��Channel�������Ѿ�������
	int err = socket_ops::getErrno(sockfd);
	if (err) {
		//���ڴ���
		LOG_TRACE << "Connector's Channel caught error:" << err << ::strerror(err);
		__retryConnect(sockfd);
	}
	else if (socket_ops::isSelfConnect(sockfd)) {
		//Ӧ������������
		LOG_TRACE << "Connector's Channel detect self-connect";
		__retryConnect(sockfd);
	}
	else {
		//�Ѿ�������ȷ������
		if (__need_connecting) {
			__setState(kConnected);//����״̬
			__on_connect(sockfd);
		}
		else {
			//�û��ڱ𴦵�����stop,����ֹͣ�˸�����
			LOG_TRACE << "User quits an established connection:" << sockfd;
			socket_ops::close(sockfd);
		}

	}
}

//brief:�������������������Channel
//becare:��Ϊ����������Channel�ַ�����֮�б����õģ����Channel����Դ�ͷű����ڱ�����֮��
//      ���Ҳ����ʹ��pushBackIntoLoop
int Connector::__getFdAndRestChannel() {
	if (!__channel)
		return -1;

	int out = __channel->getHandle();
	__channel->deactivateAll();
	__channel->removeFromEventloop();
	//__channel.reset();
	__loop->pushBackIntoQueue(std::bind(&Connector::__resetChannelNextLoop, this));

	return out;
}

//brief:�ͷ���Դ
void Connector::__resetChannelNextLoop() {
	__channel.reset();
}


}//!namespace core
}//!namespace imnet
