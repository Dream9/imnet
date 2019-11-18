/*************************************************************************
	> File Name: http_context.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年11月12日 星期二 10时03分16秒
 ************************************************************************/
#ifdef _MSC_VER
#include"http_context.h"
#include"../core/tcp/buffer.h"
#else
#include"httpd/http_context.h"
#include"core/tcp/buffer.h"
#endif

namespace imnet {

namespace http {

const int HttpContext::kChunked = -5996;

static const int kFormatError = -1;
static const int kGoHead = 0;
static const int kAgain = 1;
static const int kFinish = 2;

//brief:解析新到达的请求内容，直到kDone
//return:小于0表示请求的格式错误，大于0表示期待更多数据或者请求数据已经传送完毕，
//      需要上层通过isDone()做进一步的判断
int HttpContext::parseRequest(Buffer* buf, CTimestamp timestamp) {
	assert(__state != kDone);
	
	int ret = 0;
	while (true) {
		switch (__state) {
		case kOnRequestLine:
			if (0 != (ret = __parseRequestLine(buf, timestamp)))
				return ret;
			break;
		case kOnHeader:
			if (0 != (ret = __parseHeader(buf)))
				return ret;
			break;
		case kOnBody:
			if (0 != (ret = __parseBody(buf)))
				return ret;
			break;
		default:
			break;
		}
	}
}

#define IF_NULL_ERROR(x) if(!x){return kFormatError;}
#define IF_NULL_MORE(x) if(!x){return kAgain;}
//brief:解析请求行
//becare:只有能一次性完整解析请求行后，才会将该数据从buf中移除，否则将一直保留
//return :1:期待更多数据, -1:请求格式非法, 0:请求行已经被正确分析
int HttpContext::__parseRequestLine(Buffer* buf, CTimestamp timestamp) {
	//示例
	//GET /s?id=1607768406052640198&wfr=spider&for=pc HTTP/1.1
    //Host: baijiahao.baidu.com
    //Connection: keep-alive
    //Cache-Control: max-age=0
    //Upgrade-Insecure-Requests: 1
    //User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.97 Safari/537.36
    //Sec-Fetch-User: ?1
    //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*; q=0.8,application/signed-exchange; v=b3
	//... ...

	const char* end = buf->findCrlf();
	IF_NULL_MORE(end);

	//进入到此处，该行必须是完整的合法的请求行
	const char* left = buf->peak();

	const char* space = static_cast<const char*>(::memchr(left, ' ', end - left));
	IF_NULL_ERROR(space);
	__request.setMethod(left, space);

	left = space + 1;
	//FIXME:left应该确保定位到第一个非空字符上
	while (::isspace(*left)) {
		++left;
	}
	space = static_cast<const char*>(::memchr(left, ' ', end - left));
	IF_NULL_ERROR(space);

	const char* parameter = static_cast<const char*>(::memchr(left, '?', space - left));
	//if (!parameter) {
	if (parameter) {
		__request.setPath(left, parameter);
		__request.setQuery(parameter + 1, space);
	}
	else {
		__request.setPath(left, space);
	}

	//http版本协议
	left = space + 1;
	//FIXME:确保left定位到非空字符上面
	while (::isspace(*left)) {
		++left;
	}
	if (end - left == sizeof("HTTP/1.0") && 0 != memcmp(left, "HTTP/1.", sizeof("HTTP/1.")))
		return kFormatError;
	__request.setVersion(*(end - 1) == '1' ? Version::kHttp11 :
			(*(end - 1) == '0' ? Version::kHttp10 : Version::kUnknown));

	//取出已经解析的数据
	buf->retrieveUntil(end + 2);
	__request.setTime(timestamp);

	//进入下一个状态
	__state = kOnHeader;
	return kGoHead;//请求行已经被正确提取
}

//brief:解析单条请求头的属性信息
//return:-1:格式错误，0：该行信息已完成，1：期待更多数据，2：全部请求已经解析完成
int HttpContext::__parseHeader(Buffer* buf) {
	assert(__state == kOnHeader);

	const char* end = buf->findCrlf();
	IF_NULL_MORE(end);
	const char* left = buf->peak();
	if (left == end) {
		//该行是请求头与请求体的分割部分,调整至下一状态
		buf->retrieve(2);

		return __dealBodyTransferType();
	}

	//进行到这里，该属性项必须是完整的
	const char* colon = static_cast<const char*>(memchr(left, ':', end - left));
	IF_NULL_ERROR(colon);
	__request.addHeader(left, colon, end);

	buf->retrieveUntil(end + 2);
	return kGoHead;
}

//brief:解析请求体
//return:-1:格式错误，1：期待更多数据，2全部请求解析完成，不会返回0
int HttpContext::__parseBody(Buffer* buf) {
	assert(__state == kOnBody);

	//根据请求头的属性采取不同的数据处理策略
	if (__body_flag == kChunked) {
		return __parseBodyChunked(buf);
	}

	//采用Content-Length的方式传输
	int buf_size = buf->getDataSize();
	int min_size = buf_size < __body_flag ? buf_size : __body_flag;
	__request.appendBody(buf->peak(), buf->peak() + min_size);

	__body_flag -= min_size;
	buf->retrieve(min_size);
	return __body_flag == 0 ? kFinish: kAgain;
}

//brief:根据请求方式已经属性信息决定状态变换，并设置请求体的传送方式
int HttpContext::__dealBodyTransferType() {
	//只有POST请求才会产生请求体
	if (__request.getMethod() != Method::kPost) {
	    __state = kDone;
	    return kFinish;
	}

	__state = kOnBody;
	if (__request.getHeader("Transfer-Encoding") == "Chunked") {
		__body_flag = kChunked;
	}
	else {
		string length = __request.getHeader("Content-Length");
		if (length.empty())
			return kFormatError;

		__body_flag = stoi(length);
	}
	return kGoHead;
}

//brief:
//return:-1:格式错误，1：期待更多，2：完成
int HttpContext::__parseBodyChunked(Buffer* buf) {
	//示例：
	//原始信息
	//25
	//This is the data in the first chunk

	//1A
	//and this is the second one
	//0

	//接收到的报文：(以16进制形式展示的ascii形式)
	//                           32 35 0d 0a 54 68 69            25..Thi
	//73 20 69 73 20 74 68 65 20 64 61 74 61 20 69 6e   s is the data in
	//20 74 68 65 20 66 69 72 73 74 20 63 68 75 6e 6b    the first chunk
	//0d 0a 0d 0a 31 41 0d 0a 61 6e 64 20 74 68 69 73   ....1A..and this//注意采用了16进制编码
	//20 69 73 20 74 68 65 20 73 65 63 6f 6e 64 20 6f    is the second o
	//6e 65 0d 0a 30 0d 0a 0d 0a                        ne..0....       //其中..代表crlf

	const char* end = buf->findCrlf();
	IF_NULL_ERROR(end);
	const char* left = buf->peak();

	
	//int length = stoi(string(left, end));
	int length = stoi(string(left, end), nullptr, 16);//注意采用的是16进制编码形式
	if (buf->getDataSize() < static_cast<size_t>(length + 4 + end - left)) {
		//形如 5\r\nabcde\r\n 
		return kAgain;
	}

	//运行到此处必须是完整合法的块，否则返回-1
	left = end + 2;
	end = left + length;

	if (*end != '\r' || *(end + 1) != '\n')
		return kFormatError;

	__request.appendBody(left, end);
	buf->retrieveUntil(end + 2);

	if (length == 0) {
		//最后一个空块，表示请求体结束
		__state = kDone;
		return kFinish;
	}

	return kAgain;
}

#undef IF_NULL_MORE
#undef IF_NULL_ERROR


}
}
