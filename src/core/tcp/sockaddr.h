/*************************************************************************
	> File Name: sockaddr.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019年10月11日 星期五 11时02分15秒
 ************************************************************************/
#ifndef _CORE_SOCKADDR_H_
#define _CORE_SOCKADDR_H_

#ifdef _MSC_VER
#include"../../dependent/string_piece.h"
#else
#include"dependent/string_piece.h"
#endif

#ifdef _WIN32
#include<WS2tcpip.h>
#else
#include<netinet/in.h>
#endif

namespace imnet {

namespace core {

//brief:实现ipv4以及ipv6地址的兼容
class Sockaddr {
public:
	//brief：通常用于建立用于bind的地址
	Sockaddr(uint16_t port, bool is_ipv6_address = false, bool is_loopback_address = false);
	//brief:通常用于链接的地址
	Sockaddr(CStringArg str, uint16_t port, bool is_ipv6_address = false);

	explicit Sockaddr() { memZero(&__ipv6, sizeof __ipv6); }//默认版本的构造函数
	explicit Sockaddr(const struct sockaddr_in& address) :__ipv4(address) {}
	explicit Sockaddr(const struct sockaddr_in6& address) :__ipv6(address){}

	//得到原始数据内容,注意为网络序
	sa_family_t getFamily() const {
		return __ipv4.sin_family;
	}
	uint16_t getPort()const {
		return __ipv4.sin_port;
	}
	//得到兼容POSIX标准的函数参数
	const struct sockaddr* castToSockaddrPtr()const {
		//必须先转换为const void*
		const void* ptr = &__ipv6;
		return static_cast<const struct sockaddr*>(ptr);
		//return static_cast<struct sockaddr*>(&__ipv6);
	}

	uint32_t getAddr()const;
	struct in6_addr getAddr6()const;


	//格式转换
	string inet_ntop() const;
	string inet_ntop_withport() const;
	uint16_t getPortByHostorder() const;

	void setData(const struct sockaddr_in6& data) {
		__ipv6 = data;
	}
	void setData(const struct sockaddr_in& data) {
		__ipv4 = data;
	}
	//提供getaddrinfo的封装
	static bool getaddrinfoForConnect(CStringArg hostname, Sockaddr* parser_result, bool is_only_ipv4 = true);
	static bool getaddrinfoForConnect(CStringArg hostname, CStringArg port_or_service, Sockaddr* parser_result, bool is_only_ipv4 = true);
	static bool getaddrinfoForBind(CStringArg port, Sockaddr* parser_result, bool is_only_ipv4 = true);

private:
	union {
		struct sockaddr_in __ipv4;
		struct sockaddr_in6 __ipv6;
	};
};

}
}

#endif
