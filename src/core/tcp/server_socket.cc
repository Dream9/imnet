/*************************************************************************
	> File Name: server_socket.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 11时17分33秒
 ************************************************************************/
#ifdef _MSC_VER
#include"server_socket.h"
#include"socket_extension.h"
#include"sockaddr.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/server_socket.h"
#include"core/tcp/socket_extension.h"
#include"core/tcp/sockaddr.h"
#include"dependent/logger/logger.h"
#endif

#include<netinet/tcp.h>//或者<linux/tcp.h>
//通过在usr/include/下使用grep -r TCP_NODELAY搜素得到

namespace imnet {

namespace core {

ServeSocket::ServeSocket(const Sockaddr& addr)
	:__sockfd(socket_ops::createNonblockingTcpSocket(addr.getFamily())) {
}

//becare:务必保证资源的有效
ServeSocket::~ServeSocket() {
	assert(__sockfd >= 0);

	socket_ops::close(__sockfd);
}

//将请求重新传参出去即可
void ServeSocket::bind(const Sockaddr& addr) {
	socket_ops::bind(__sockfd, addr.castToSockaddrPtr());
}

void ServeSocket::listen() {
	socket_ops::listen(__sockfd);
}

int ServeSocket::accept(Sockaddr* peeraddr) {
	//外界无法直接访问peeraddr的内容，因此需要一个临时变量
	struct sockaddr_in6 ans;
	memZero(&ans, sizeof ans);

	int out = socket_ops::accept(__sockfd, &ans);
	if (out >= 0) {
		peeraddr->setData(ans);
	}

	return out;
}

void ServeSocket::shutdownWrite() {
	socket_ops::shutdownWrite(__sockfd);
}

/////////////////////////////////////////////////////////////
#define SYSCALL_CHECK_ERROR(ret) if (ret < 0) {\
	LOG_SYSERR << "fd:" << __sockfd << ",setsockopt() failed.";\
	}
//状态设置
void ServeSocket::setTcpNoDelay(bool on) {
	int value = on;
#ifdef _WIN32
	int ret = ::setsockopt(__sockfd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&value), sizeof value);
#else
	int ret = ::setsockopt(__sockfd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof value);
#endif
	SYSCALL_CHECK_ERROR(ret);
}

void ServeSocket::setKeepAlive(bool on) {
	int value = on;
#ifdef _WIN32
	int ret = ::setsockopt(__sockfd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&value), sizeof value);
#else
	int ret = ::setsockopt(__sockfd, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof value);
#endif
	SYSCALL_CHECK_ERROR(ret);
}

void ServeSocket::setReuseAddr(bool on) {
	int value = on;
#ifdef _WIN32
	int ret = ::setsockopt(__sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&value), sizeof value);
#else
	int ret = ::setsockopt(__sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof value);
#endif
	SYSCALL_CHECK_ERROR(ret);
}

void ServeSocket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
	int value = on;
	int ret = ::setsockopt(__sockfd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof value);
	SYSCALL_CHECK_ERROR(ret);
#else
	if (on) {
		LOG_ERROR << "SO_REUSEPORT is not supported at this platform.";
	}
#endif
}

//brief:通过getsockopt获取有关信息
//return：true:读取并设置信息完成，否则为失败
bool ServeSocket::getTcpInfo(struct tcp_info* parser_ans) const {
	assert(parser_ans!=nullptr);

	socklen_t len = sizeof(struct tcp_info);
	memZero(parser_ans,len);
	return ::getsockopt(__sockfd, SOL_TCP, TCP_INFO, parser_ans, &len) == 0;
}

bool ServeSocket::getTcpInfo(char* str, int len)const {
	assert(str!=nullptr);
	assert(len>=0);

	struct tcp_info tcpi;
	bool out = getTcpInfo(&tcpi);
	if (out) {
		
		snprintf(str, len, "unrecovered=%u "
			"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
			"lost=%u retrans=%u rtt=%u rttvar=%u "
			"sshthresh=%u cwnd=%u total_retrans=%u",
			tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
			tcpi.tcpi_rto,          // Retransmit timeout in usec
			tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
			tcpi.tcpi_snd_mss,
			tcpi.tcpi_rcv_mss,
			tcpi.tcpi_lost,         // Lost packets
			tcpi.tcpi_retrans,      // Retransmitted packets out
			tcpi.tcpi_rtt,          // Smoothed round trip time in usec
			tcpi.tcpi_rttvar,       // Medium deviation
			tcpi.tcpi_snd_ssthresh,
			tcpi.tcpi_snd_cwnd,
			tcpi.tcpi_total_retrans);  // Total retransmits for entire connection

	}

	return out;
	
}

#undef SYSCALL_CHECK_ERROR

}
}
