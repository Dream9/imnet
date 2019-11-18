/*************************************************************************
	> File Name: aockaddr.cc
	> Author: Dream9
	> Brief:
	> Created Time: 2019年10月11日 星期五 11时02分32秒
 ************************************************************************/
#ifdef _MSC_VER
#include"sockaddr.h"
#include"socket_extension.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/sockaddr.h"
#include"core/tcp/socket_extension.h"
#include"dependent/logger/logger.h"
#endif

#include<assert.h>

#include<netdb.h>
#include<sys/socket.h>

namespace imnet {

namespace core {

//平台兼容性检验
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family's offset should be 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port's offset should be 2");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family's offset should be 0");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port's offset should be 2");

static_assert(sizeof(struct sockaddr_in6) == sizeof(Sockaddr), "Sockaddr's space should be same with struct sockaddr_in6");

//brief:获得主机序的端口号
inline uint16_t Sockaddr::getPortByHostorder() const {
    //becare:以下两个静态断言需要访问私有成员，因此放在这里
	static_assert(offsetof(Sockaddr, __ipv4) == 0, "__ipv4's offset should be 0");
    static_assert(offsetof(Sockaddr, __ipv6) == 0, "__ipv6's offset should be 0");

	return socket_ops::ntohs(__ipv4.sin_port);
}
//linux下netinent/in.h中关于这两个结构的定义
//struct sockaddr_in//ipv4结构
//{
//	__SOCKADDR_COMMON(sin_);
//	in_port_t sin_port;			/* Port number.  */
//	struct in_addr sin_addr;		/* Internet address.  */
//
//	/* Pad to size of `struct sockaddr'.  */////////////注意在LInux下面，为了保证与sockaddr结构大小一致，做了填充，通常为0
//	unsigned char sin_zero[sizeof(struct sockaddr) -
//		__SOCKADDR_COMMON_SIZE -
//		sizeof(in_port_t) -
//		sizeof(struct in_addr)];
//};
//
//#if !__USE_KERNEL_IPV6_DEFS
///* Ditto, for IPv6.  */
//struct sockaddr_in6///ipv6结构
//{
//	__SOCKADDR_COMMON(sin6_);/////即 sa_family_t sin6_family
//	in_port_t sin6_port;	/* Transport layer port # */
//	uint32_t sin6_flowinfo;	/* IPv6 flow information */
//	struct in6_addr sin6_addr;	/* IPv6 address */
//	uint32_t sin6_scope_id;	/* IPv6 scope-id */
//};
//#endif /* !__USE_KERNEL_IPV6_DEFS */

//其中上述定义中需要的两个宏为
//#define __SOCKADDR_COMMON(sa_prefix) sa_family_t sa_prefix##family
//
//
//#define __SOCKADDR_COMMON_SIZE  (sizeof (unsigned short int))

//brief:适合TcpServe的构造函数
//     默认将会产生监听所有ip地址的sockaddr
//     因此此种初始化的结果可直接用于服务端listen操作
//     如果需要产生用于客户端connect的地址，请使用本类的静态函数getaddrinfo产生
Sockaddr::Sockaddr(uint16_t port, bool is_ipv6_address, bool is_loopback_address) {
	memZero(&__ipv6, sizeof(__ipv6));
	if (is_ipv6_address) {
		__ipv6.sin6_family = AF_INET6;
		//__ipv6.sin6_port = port;
		__ipv6.sin6_addr = is_loopback_address ? in6addr_loopback : in6addr_any;//默认的地址内容分别为127.0.0.1或则和0.0.0.0
		__ipv6.sin6_port = socket_ops::htons(port);
	}
	else {
//注意INADDR_LOOPBACK采用的是小端法的记录方式，需要转换
//# define INADDR_LOOPBACK	((in_addr_t) 0x7f000001) /* Inet 127.0.0.1.  */
		__ipv4.sin_family = AF_INET;
		__ipv4.sin_addr.s_addr = is_loopback_address ? socket_ops::htonl(INADDR_LOOPBACK) : socket_ops::htonl(INADDR_ANY);
		__ipv4.sin_port = socket_ops::htons(port);
	}
}

//brief:通过点分十进制进行初始化
//     此时需要指定类型
Sockaddr::Sockaddr(CStringArg str, uint16_t port, bool is_ipv6_address) {
	memZero(&__ipv6, sizeof(__ipv6));
	if (is_ipv6_address) {
		socket_ops::inet_pton(str.c_str(), port, &__ipv6);
	}
	else {
		socket_ops::inet_pton(str.c_str(), port, &__ipv4);
	}
}

//约定：分发又本对象完成，重载在socket_ops中实现
string Sockaddr::inet_ntop()const {
	//必须要有足够的大小存储至多128位地址
	//即floor(128*（log2/log10))
	//通常此大小通过以下宏定义
//#define INET_ADDRSTRLEN 16
//#define INET6_ADDRSTRLEN 46//注意此大小已经考虑到了分隔符号.
	char buf[64];
	assert(sizeof buf > INET6_ADDRSTRLEN);

	if (__ipv6.sin6_family == AF_INET) {
		socket_ops::inet_ntop(&__ipv4, buf, sizeof buf);
	}
	else if (__ipv6.sin6_family == AF_INET6) {
		socket_ops::inet_ntop(&__ipv6, buf, sizeof buf);
	}
	else {
		LOG_ERROR << "inet_ntop():bad domain type" << __ipv6.sin6_family;
		buf[0] = '\0';
	}

	return buf;
}
string Sockaddr::inet_ntop_withport()const {
	//同理，还要加上port占据的最少4.8个字符，即5个，以及一个分隔符号:
	char buf[64];
	assert(sizeof buf > INET6_ADDRSTRLEN + 6);

	if (__ipv6.sin6_family == AF_INET) {
		//FIXME:应该使用withport版本
		//socket_ops::inet_ntop(&__ipv4, buf, sizeof buf);
		socket_ops::inet_ntop_withport(&__ipv4, buf, sizeof buf);
	}
	else if (__ipv6.sin6_family == AF_INET6) {
		//FIXME:应该使用withport版本
		//socket_ops::inet_ntop(&__ipv6, buf, sizeof buf);
		socket_ops::inet_ntop_withport(&__ipv6, buf, sizeof buf);
	}
	else {
		LOG_ERROR << "inet_ntop():bad domain type" << __ipv6.sin6_family;
		buf[0] = '\0';
	}

	//size_t len = strlen(buf);
	return buf;
}

uint32_t Sockaddr::getAddr()const {
	assert(__ipv4.sin_family == AF_INET);
	return __ipv4.sin_addr.s_addr;//设计上的失误
}

struct in6_addr Sockaddr::getAddr6()const {
	assert(__ipv6.sin6_family == AF_INET6);
	return __ipv6.sin6_addr;
	//memcpy(ans, &__ipv6.sin6_addr, sizeof(__ipv6.sin6_addr));
}

//brief:解析主机名到Sockaddr,
//return:是否获得正确解析的ip address
//parameter:hostname:主机名（可以是域名，也可以是数字地址），parser_ans:存放解析的结果，is_only_ipv4：默认只查询ipv4的地址
//becare:区别于使用getaddrinfo获得合适的用于本地监听的socket所需要的所有信息
//       如果产生适合listen的地址，请使用Sockaddr(uint16_t port, bool is_ipv6_address = false, bool is_loopback_address = false)初始化
bool Sockaddr::getaddrinfoForConnect(CStringArg hostname, Sockaddr* parser_ans, bool is_only_ipv4) {
	assert(parser_ans);

	struct addrinfo *ailist, *aip;
	struct addrinfo hint;

	//初始化hint的过滤条件
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//只查询ipv4地址
	}
	hint.ai_flags |= AI_ADDRCONFIG;


	int ret = ::getaddrinfo(hostname.c_str(), NULL, &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo的错误信息必须通过gai_strerror获取
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo的标准使用方式
	//遍历所有查询结果，找到即可返回，注意释放资源
	for (aip = ailist; aip; aip = aip->ai_next) {
		sa_family_t family = aip->ai_family;
		switch (family) {
		case AF_INET:
			//parser_ans->setData(static_cast<struct sockaddr_in>(*aip->ai_addr));
			parser_ans->setData(*static_cast<struct sockaddr_in*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		case AF_INET6:
			//parser_ans->setData(static_cast<struct sockaddr_in6>(*aip->ai_addr));
			parser_ans->setData(*static_cast<struct sockaddr_in6*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		default:
			continue;
		}
	}
	freeaddrinfo(ailist);
	return false;
}

//TODO:
bool Sockaddr::getaddrinfoForConnect(CStringArg hostname, CStringArg port_or_service, Sockaddr* parser_ans, bool is_only_ipv4) {
	//static_assert(false, "TODO");
	assert(parser_ans);

	struct addrinfo *ailist, *aip;
	struct addrinfo hint;

	//初始化hint的过滤条件
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//只查询ipv4地址
	}
	hint.ai_flags |= AI_ADDRCONFIG;


	int ret = ::getaddrinfo(hostname.c_str(), port_or_service.c_str(), &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo的错误信息必须通过gai_strerror获取
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo的标准使用方式
	//遍历所有查询结果，找到即可返回，注意释放资源
	for (aip = ailist; aip; aip = aip->ai_next) {
		sa_family_t family = aip->ai_family;
		switch (family) {
		case AF_INET:
			//parser_ans->setData(static_cast<struct sockaddr_in>(*aip->ai_addr));
			parser_ans->setData(*static_cast<struct sockaddr_in*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		case AF_INET6:
			//parser_ans->setData(static_cast<struct sockaddr_in6>(*aip->ai_addr));
			parser_ans->setData(*static_cast<struct sockaddr_in6*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		default:
			continue;
		}
	}
	freeaddrinfo(ailist);
	return false;
}

//brief:与getaddrinfoForConnect相对应地函数，但是默认地构造函数即可实现本功能
//      但本函数可以实现与协议无关地可重入地址
//return:是否获得正确解析的ip address
//parameter:port:这里强制采用端口号，可以减少解析过程，parser_ans:存放解析的结果，is_only_ipv4：默认只查询ipv4的地址
bool Sockaddr::getaddrinfoForBind(CStringArg port, Sockaddr* parser_ans, bool is_only_ipv4) {
	assert(parser_ans);

	//The addrinfo structure used by getaddrinfo() contains the following
	//	fields :

	//struct addrinfo {
	//	int              ai_flags;
	//	int              ai_family;
	//	int              ai_socktype;
	//	int              ai_protocol;
	//	socklen_t        ai_addrlen;
	//	struct sockaddr *ai_addr;
	//	char            *ai_canonname;
	//	struct addrinfo *ai_next;
	//};

	struct addrinfo *ailist, *aip;
	struct addrinfo hint;

	//初始化hint的过滤条件
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//只查询ipv4地址
	}
	hint.ai_flags |= AI_PASSIVE;//产生0.0.0.0地通配地址
	hint.ai_flags |= AI_ADDRCONFIG;

	//If AI_NUMERICSERV is specified in hints.ai_flags and service is not
	//NULL, then service must point to a string containing a numeric port 
	//number.This flag is used to inhibit the invocation of a name resolution
	//service in cases where it is known not to be required.
	hint.ai_flags |=  AI_NUMERICSERV;//强制port是端口号的字符串形式//可以较少解析过程

	int ret = ::getaddrinfo(NULL, port.c_str(), &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo的错误信息必须通过gai_strerror获取
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo的标准使用方式
	//遍历所有查询结果，找到即可返回，注意释放资源
	for (aip = ailist; aip; aip = aip->ai_next) {
		sa_family_t family = aip->ai_family;
		switch (family) {
		case AF_INET:
			//*parser_ans = Sockaddr(static_cast<struct sockaddr_in>(*aip->ai_addr));
			*parser_ans = Sockaddr(*static_cast<struct sockaddr_in*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		case AF_INET6:
			*parser_ans = Sockaddr(*static_cast<struct sockaddr_in6*>((void*)aip->ai_addr));
			freeaddrinfo(ailist);
			return true;
		default:
			continue;
		}
	}
	freeaddrinfo(ailist);
	return false;
}




}
}
