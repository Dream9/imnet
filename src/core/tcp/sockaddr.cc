/*************************************************************************
	> File Name: aockaddr.cc
	> Author: Dream9
	> Brief:
	> Created Time: 2019��10��11�� ������ 11ʱ02��32��
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

//ƽ̨�����Լ���
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family's offset should be 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port's offset should be 2");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family's offset should be 0");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port's offset should be 2");

static_assert(sizeof(struct sockaddr_in6) == sizeof(Sockaddr), "Sockaddr's space should be same with struct sockaddr_in6");

//brief:���������Ķ˿ں�
inline uint16_t Sockaddr::getPortByHostorder() const {
    //becare:����������̬������Ҫ����˽�г�Ա����˷�������
	static_assert(offsetof(Sockaddr, __ipv4) == 0, "__ipv4's offset should be 0");
    static_assert(offsetof(Sockaddr, __ipv6) == 0, "__ipv6's offset should be 0");

	return socket_ops::ntohs(__ipv4.sin_port);
}
//linux��netinent/in.h�й����������ṹ�Ķ���
//struct sockaddr_in//ipv4�ṹ
//{
//	__SOCKADDR_COMMON(sin_);
//	in_port_t sin_port;			/* Port number.  */
//	struct in_addr sin_addr;		/* Internet address.  */
//
//	/* Pad to size of `struct sockaddr'.  */////////////ע����LInux���棬Ϊ�˱�֤��sockaddr�ṹ��Сһ�£�������䣬ͨ��Ϊ0
//	unsigned char sin_zero[sizeof(struct sockaddr) -
//		__SOCKADDR_COMMON_SIZE -
//		sizeof(in_port_t) -
//		sizeof(struct in_addr)];
//};
//
//#if !__USE_KERNEL_IPV6_DEFS
///* Ditto, for IPv6.  */
//struct sockaddr_in6///ipv6�ṹ
//{
//	__SOCKADDR_COMMON(sin6_);/////�� sa_family_t sin6_family
//	in_port_t sin6_port;	/* Transport layer port # */
//	uint32_t sin6_flowinfo;	/* IPv6 flow information */
//	struct in6_addr sin6_addr;	/* IPv6 address */
//	uint32_t sin6_scope_id;	/* IPv6 scope-id */
//};
//#endif /* !__USE_KERNEL_IPV6_DEFS */

//����������������Ҫ��������Ϊ
//#define __SOCKADDR_COMMON(sa_prefix) sa_family_t sa_prefix##family
//
//
//#define __SOCKADDR_COMMON_SIZE  (sizeof (unsigned short int))

//brief:�ʺ�TcpServe�Ĺ��캯��
//     Ĭ�Ͻ��������������ip��ַ��sockaddr
//     ��˴��ֳ�ʼ���Ľ����ֱ�����ڷ����listen����
//     �����Ҫ�������ڿͻ���connect�ĵ�ַ����ʹ�ñ���ľ�̬����getaddrinfo����
Sockaddr::Sockaddr(uint16_t port, bool is_ipv6_address, bool is_loopback_address) {
	memZero(&__ipv6, sizeof(__ipv6));
	if (is_ipv6_address) {
		__ipv6.sin6_family = AF_INET6;
		//__ipv6.sin6_port = port;
		__ipv6.sin6_addr = is_loopback_address ? in6addr_loopback : in6addr_any;//Ĭ�ϵĵ�ַ���ݷֱ�Ϊ127.0.0.1�����0.0.0.0
		__ipv6.sin6_port = socket_ops::htons(port);
	}
	else {
//ע��INADDR_LOOPBACK���õ���С�˷��ļ�¼��ʽ����Ҫת��
//# define INADDR_LOOPBACK	((in_addr_t) 0x7f000001) /* Inet 127.0.0.1.  */
		__ipv4.sin_family = AF_INET;
		__ipv4.sin_addr.s_addr = is_loopback_address ? socket_ops::htonl(INADDR_LOOPBACK) : socket_ops::htonl(INADDR_ANY);
		__ipv4.sin_port = socket_ops::htons(port);
	}
}

//brief:ͨ�����ʮ���ƽ��г�ʼ��
//     ��ʱ��Ҫָ������
Sockaddr::Sockaddr(CStringArg str, uint16_t port, bool is_ipv6_address) {
	memZero(&__ipv6, sizeof(__ipv6));
	if (is_ipv6_address) {
		socket_ops::inet_pton(str.c_str(), port, &__ipv6);
	}
	else {
		socket_ops::inet_pton(str.c_str(), port, &__ipv4);
	}
}

//Լ�����ַ��ֱ�������ɣ�������socket_ops��ʵ��
string Sockaddr::inet_ntop()const {
	//����Ҫ���㹻�Ĵ�С�洢����128λ��ַ
	//��floor(128*��log2/log10))
	//ͨ���˴�Сͨ�����º궨��
//#define INET_ADDRSTRLEN 16
//#define INET6_ADDRSTRLEN 46//ע��˴�С�Ѿ����ǵ��˷ָ�����.
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
	//ͬ����Ҫ����portռ�ݵ�����4.8���ַ�����5�����Լ�һ���ָ�����:
	char buf[64];
	assert(sizeof buf > INET6_ADDRSTRLEN + 6);

	if (__ipv6.sin6_family == AF_INET) {
		//FIXME:Ӧ��ʹ��withport�汾
		//socket_ops::inet_ntop(&__ipv4, buf, sizeof buf);
		socket_ops::inet_ntop_withport(&__ipv4, buf, sizeof buf);
	}
	else if (__ipv6.sin6_family == AF_INET6) {
		//FIXME:Ӧ��ʹ��withport�汾
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
	return __ipv4.sin_addr.s_addr;//����ϵ�ʧ��
}

struct in6_addr Sockaddr::getAddr6()const {
	assert(__ipv6.sin6_family == AF_INET6);
	return __ipv6.sin6_addr;
	//memcpy(ans, &__ipv6.sin6_addr, sizeof(__ipv6.sin6_addr));
}

//brief:������������Sockaddr,
//return:�Ƿ�����ȷ������ip address
//parameter:hostname:��������������������Ҳ���������ֵ�ַ����parser_ans:��Ž����Ľ����is_only_ipv4��Ĭ��ֻ��ѯipv4�ĵ�ַ
//becare:������ʹ��getaddrinfo��ú��ʵ����ڱ��ؼ�����socket����Ҫ��������Ϣ
//       ��������ʺ�listen�ĵ�ַ����ʹ��Sockaddr(uint16_t port, bool is_ipv6_address = false, bool is_loopback_address = false)��ʼ��
bool Sockaddr::getaddrinfoForConnect(CStringArg hostname, Sockaddr* parser_ans, bool is_only_ipv4) {
	assert(parser_ans);

	struct addrinfo *ailist, *aip;
	struct addrinfo hint;

	//��ʼ��hint�Ĺ�������
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//ֻ��ѯipv4��ַ
	}
	hint.ai_flags |= AI_ADDRCONFIG;


	int ret = ::getaddrinfo(hostname.c_str(), NULL, &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo�Ĵ�����Ϣ����ͨ��gai_strerror��ȡ
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo�ı�׼ʹ�÷�ʽ
	//�������в�ѯ������ҵ����ɷ��أ�ע���ͷ���Դ
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

	//��ʼ��hint�Ĺ�������
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//ֻ��ѯipv4��ַ
	}
	hint.ai_flags |= AI_ADDRCONFIG;


	int ret = ::getaddrinfo(hostname.c_str(), port_or_service.c_str(), &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo�Ĵ�����Ϣ����ͨ��gai_strerror��ȡ
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo�ı�׼ʹ�÷�ʽ
	//�������в�ѯ������ҵ����ɷ��أ�ע���ͷ���Դ
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

//brief:��getaddrinfoForConnect���Ӧ�غ���������Ĭ�ϵع��캯������ʵ�ֱ�����
//      ������������ʵ����Э���޹صؿ������ַ
//return:�Ƿ�����ȷ������ip address
//parameter:port:����ǿ�Ʋ��ö˿ںţ����Լ��ٽ������̣�parser_ans:��Ž����Ľ����is_only_ipv4��Ĭ��ֻ��ѯipv4�ĵ�ַ
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

	//��ʼ��hint�Ĺ�������
	memZero(&hint, sizeof(addrinfo));
	hint.ai_socktype = SOCK_STREAM;
	if (is_only_ipv4) {
		hint.ai_family = AF_INET;//ֻ��ѯipv4��ַ
	}
	hint.ai_flags |= AI_PASSIVE;//����0.0.0.0��ͨ���ַ
	hint.ai_flags |= AI_ADDRCONFIG;

	//If AI_NUMERICSERV is specified in hints.ai_flags and service is not
	//NULL, then service must point to a string containing a numeric port 
	//number.This flag is used to inhibit the invocation of a name resolution
	//service in cases where it is known not to be required.
	hint.ai_flags |=  AI_NUMERICSERV;//ǿ��port�Ƕ˿ںŵ��ַ�����ʽ//���Խ��ٽ�������

	int ret = ::getaddrinfo(NULL, port.c_str(), &hint, &ailist);
	if (ret != 0) {
		//LOG_SYSERR << "getaddrinfo() failed.";
		//getaddrinfo�Ĵ�����Ϣ����ͨ��gai_strerror��ȡ
		LOG_ERROR << ::gai_strerror(ret);
		return false;
	}

	//getaddrinfo�ı�׼ʹ�÷�ʽ
	//�������в�ѯ������ҵ����ɷ��أ�ע���ͷ���Դ
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
