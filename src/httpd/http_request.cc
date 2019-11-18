/*************************************************************************
	> File Name: http_request.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分29秒
 ************************************************************************/
#ifdef _MSC_VER
#include"http_request.h"
#else
#include"httpd/http_request.h"
#endif

using namespace imnet;
using namespace imnet::http;

//brief:向属性表中添加新的属性值
//parameter:start:属性行的起始非空字符，colon：属性行中的冒号位置，end:属性行的尾后位置
//becare:start为合法字符，由用户(HttpContext)保证
void HttpRequest::addHeader(const char* start, const char* colon, const char* end) {
	string key(start, colon++);

	while (colon != end && ::isspace(*colon)) {
		++colon;
	}
	
	while (end != colon && ::isspace(*(end - 1))) {
		--end;
	}

	string value(colon, end);
	__headers.insert({ key,value });
}

//becare:注意方法的匹配应该交由本对象完成，而不是使用者
//return:请求方法不支持或者未知返回false
bool HttpRequest::setMethod(const char*start, const char* end) {
		//assert(__method == kUninit);

		string tmp(start, end);

		if (tmp == "OPTIONS") {
			__method = kOptions;
		}
		else if (tmp == "HEAD") {
			__method = kHead;
		}
		else if (tmp == "GET") {
			__method = kGet;
		}
		else if (tmp == "POST") {
			__method = kPost;
		}
		else if (tmp == "PUT") {
			__method = kPut;
		}
		else if (tmp == "DELETE") {
			__method = kDelete;
		}
		else if (tmp == "TRACE") {
			__method = kTrace;
		}

		return __method != kUninit;
}

//brief:
string HttpRequest::MethodToString(Method val) {
	switch (val)
	{
	case Method::kUninit:
		return "Uninit";
	case Method::kHead:
		return "HEAD";
	case Method::kGet:
		return "GET";
	case Method::kPut:
		return "PUT";
	case Method::kDelete:
		return "DELETE";
	case Method::kOptions:
		return "OPTIONS";
	case Method::kPost:
		return "POST";
	case Method::kTrace:
		return "TRACE";
	default:
		return "Unknown";
	}
}

