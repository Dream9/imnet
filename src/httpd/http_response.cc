/*************************************************************************
	> File Name: http_response.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分45秒
 ************************************************************************/
#ifdef _MSC_VER
#include"http_response.h"
#include"../core/tcp/buffer.h"
#else
#include"httpd/http_response.h"
#include"core/tcp/buffer.h"
#endif

#include<stdio.h>

const char* httpd_version = nullptr;

namespace {

//初始化时，从环境中加载IMNET_HTTP10,存在则设置httpd的协议类型为1.0否则为1.1
class InitGlobalVersion { 
public:
	InitGlobalVersion() {
		if (!::getenv("IMNET_HTTP10"))
			httpd_version = "HTTP/1.1 %d ";
		else
			httpd_version = "HTTP/1.0 %d ";
	}
};

InitGlobalVersion __init_http_version;

}//!namespace

namespace imnet {

namespace http {

#define SINGLE_CRLF "\r\n"
#define DOUBLE_CRLF SINGLE_CRLF SINGLE_CRLF

//brief:得到响应信息并把结果直接置于输出buf中
void HttpResponse::appendToBuffer(Buffer* output) {
	assert(httpd_version != nullptr);
	//设置响应头
	char buf[64];
	int len = ::snprintf(buf, sizeof buf, httpd_version, __status);
	assert(len > 0);
	output->append(buf, static_cast<size_t>(len));
	output->append(HttpResponse::ResponseStatusCodeToString(__status));
	output->append(SINGLE_CRLF, sizeof(SINGLE_CRLF) - 1);

	//设置响应属性
	if (__keep_alive) {
		int len= ::snprintf(buf, sizeof buf, "Content-Length: %lu\r\n", __body.size());
		output->append(buf, static_cast<size_t>(len));
		output->append("Connection: Keep-Alive\r\n", sizeof("Connection: Keep-Alive\r\n") - 1);
	}
	else {
		//针对立刻关闭的连接，信息不需要边界（断开就是边界）
		output->append("Connection: Close\r\n", sizeof("Connection: Close\r\n") - 1);
	}

	for (const auto& item : __headers) {
		output->append(item.first);
		output->append(": ", sizeof(": ") - 1);
		output->append(item.second);
		output->append(SINGLE_CRLF, sizeof(SINGLE_CRLF) - 1);
	}

	output->append(SINGLE_CRLF, sizeof(SINGLE_CRLF) - 1);
	//output->append(DOUBLE_CRLF, sizeof(DOUBLE_CRLF));
	//设置响应体
	output->append(__body);
	
}

string HttpResponse::ResponseStatusCodeToString(ResponseStatusCode val) {
	switch (val)
	{
	case imnet::http::kInvalid:
		return "Invalid";
		break;
	case imnet::http::kOk:
		return "OK";
		break;
	case imnet::http::kMovedPermanently:
		return "Moved Permanently";
		break;
	case imnet::http::kBadRequest:
		return "Bad Request";
		break;
	case imnet::http::kForbidden:
		return "Forbidden";
		break;
	case imnet::http::kNotFound:
		return "Not Found";
		break;
	case imnet::http::kServerError:
		return "Internal Server Error";
		break;
	default:
		return "Unknown Error";
		break;
	}

}


}//!namespace http
}//!namespace imnet

