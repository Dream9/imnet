/*************************************************************************
	> File Name: sockaddr.h
	> Author: Dream9
	> Brief:
	> Created Time: 2019��10��11�� ������ 11ʱ02��15��
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

//brief:ʵ��ipv4�Լ�ipv6��ַ�ļ���
class Sockaddr {
public:
	//brief��ͨ�����ڽ�������bind�ĵ�ַ
	Sockaddr(uint16_t port, bool is_ipv6_address = false, bool is_loopback_address = false);
	//brief:ͨ���������ӵĵ�ַ
	Sockaddr(CStringArg str, uint16_t port, bool is_ipv6_address = false);

	explicit Sockaddr() { memZero(&__ipv6, sizeof __ipv6); }//Ĭ�ϰ汾�Ĺ��캯��
	explicit Sockaddr(const struct sockaddr_in& address) :__ipv4(address) {}
	explicit Sockaddr(const struct sockaddr_in6& address) :__ipv6(address){}

	//�õ�ԭʼ��������,ע��Ϊ������
	sa_family_t getFamily() const {
		return __ipv4.sin_family;
	}
	uint16_t getPort()const {
		return __ipv4.sin_port;
	}
	//�õ�����POSIX��׼�ĺ�������
	const struct sockaddr* castToSockaddrPtr()const {
		//������ת��Ϊconst void*
		const void* ptr = &__ipv6;
		return static_cast<const struct sockaddr*>(ptr);
		//return static_cast<struct sockaddr*>(&__ipv6);
	}

	uint32_t getAddr()const;
	struct in6_addr getAddr6()const;


	//��ʽת��
	string inet_ntop() const;
	string inet_ntop_withport() const;
	uint16_t getPortByHostorder() const;

	void setData(const struct sockaddr_in6& data) {
		__ipv6 = data;
	}
	void setData(const struct sockaddr_in& data) {
		__ipv4 = data;
	}
	//�ṩgetaddrinfo�ķ�װ
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
