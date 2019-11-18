/*************************************************************************
	> File Name: dreamn.cc
	> Author: Dream9
	> Brief:
	> Created Time: 2019年11月18日 星期一 14时32分37秒
 ************************************************************************/

#include"httpd/http_server.h"
#include"httpd/http_request.h"
#include"httpd/http_response.h"
#include"core/reactor/eventloop.h"
#include"core/tcp/sockaddr.h"
#include"dependent/logger/logger.h"
#include"dependent/process_information.h"
#include"dependent/file_extension.h"//FIXME:该封装太简陋了

#include"unistd.h"

#include<limits.h>
#include<stdlib.h>
#include<unordered_map>

using namespace imnet;
using namespace imnet::core;
using namespace imnet::http;

//brief:工作目录
const char* work_diretory = "/mnt/share/imnet/doc/http_resource/";

void searcher(const HttpRequest& request, HttpResponse* response);

int main(int argc, char**argv){
	LOG_INFO << argv[0] << ",pid = " << process_information::getPid() <<", Listen at 5996.";

	if(0 != chdir(work_diretory)){
		LOG_SYSFATAL << "切换工作目录失败";
	}
	LOG_DEBUG << "pwd:" << work_diretory ;

	//使用HTTP1.0协议
	//::setenv("IMNET_HTTP10","1");

	Eventloop loop;
	Sockaddr address(5996);

	//测试默认的处理
	HttpServer server(&loop, "D9Searcher", address);
	server.setThreadNumber(2);
	server.setServiceCallback(searcher);
	server.start();

	loop.loop();
}

//brief:主页
char index_html[]= 
		"<html>\r\n\
		<head>\r\n\
		<meta http-equiv=\"Content-Language\" content=\"zh-cn\">\r\n\
		<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gbk\">\r\n\
		<title>Dream9 搜索引擎</title>\r\n\
		<style>\r\n\
		a {  color: rgb(0,0,150) ; Text-decoration:none  ; font-weight : none}\r\n\
		a:hover {color: rgb(250,40,40);text-decoration:underline; font-weight : none}\r\n\
		a:visited {color: rgb(80,0,80)}\r\n\
		</style>\r\n\
		</head>\r\n\
		\r\n\
		<body onLoad=setfocus() bgcolor=\"#FFFFFF\" text=\"#000000\">\r\n\
		<br><br><br><br>\r\n\
		<table border=\"0\" width=\"550\" cellspacing=\"0\" cellpadding=\"0\" align=\"center\" height=\"300\">\r\n\
		    <tr>\r\n\
		    <td width=\"100%\" height=\"150\" align=\"middle\">\r\n\
		    <img border=\"0\" src=\"dream9.jpg\" width=\"407\" height=\"385\"  >\r\n\
		    </td>   \r\n\
		    </tr>\r\n\
		    <tr>\r\n\
		        <form method=\"get\" action=\"/cgi-bin/search.cgi\" name=\"tw\">\r\n\
		        <td width=\"100%\" height=\"25\" align=\"center\">               \r\n\
		        <input type=\"text\" name=\"word\" size=\"55\">\r\n\
		        <input type=\"submit\" value=\" 搜索\">\r\n\
		        </td>                           \r\n\
		        <input type=\"hidden\" name=\"cdtype\" value=\"GB\">                         \r\n\
		        </form>                         \r\n\
		    </tr>                         \r\n\
		    <tr>\r\n\
		    <td align=center>\r\n\
		    <font size=2>&copy;2022\r\n\
		    <a href=\"10.0.2.15:5996\">Dream 9</a>\r\n\
		    </font>\r\n\
		    </td>\r\n\
		    </tr>\r\n\
		</table>\r\n\
		</body>\r\n\
		</html>";

//brief:404
char data_404[]=
		"<html>"
		"<head><title>界面被查水表了</title></head?"
		"<body><p>估计是回不来了，GOOD LUCK</p></body>\r\n"
		"</html>";

//brief:500
char data_500[]=
		"<html>"
		"<head><title>Sorry</title></head>\r\n"
		"<body><p>最近有点方，既然有缘，不如稍后重试</p></body>\r\n"
		"</html>";

//brief:MIME类型映射
//TODO:需要进一步完善
std::unordered_map<std::string,std::string> type_map = {
	{"jpg","image/jpeg"},{"jpeg","image/jpeg"},{"gif", "image/gif"},{"html","text/html; charset=utf-8"},
	{"htm", "text/html; charset=utf-8"}
};

//brief:得到该文件对应的MIME类型
string _getContentType(const string& filepath){
	size_t point_pos = filepath.rfind('.');
	if(string::npos == point_pos){
		//未知的二进制流
		return "application/octet-stream";
	}
	
	string tmp = filepath.substr(point_pos+1);
	auto iter = type_map.find(tmp);
	if(iter == type_map.end())
		return "application/octet-stream";

	return iter->second;

}

//brief:404请求
inline void _404_response(HttpResponse* response){
	response->setStatus(ResponseStatusCode::kNotFound);
	response->setContentType("text/html; charset=utf-8");
	response->addHeader("Server", "dreamn/imnet");
	response->setKeepAlive(false);

	response->setBody(data_404, data_404 + sizeof data_404 - 1);
}

//brief:500请求
inline void _500_response(HttpResponse* response){
	response->setStatus(ResponseStatusCode::kNotFound);
	response->setContentType("text/html; charset=utf-8");
	response->addHeader("Server", "dreamn/imnet");
	response->setKeepAlive(false);

	response->setBody(data_500, data_500 + sizeof data_500 - 1);
}

//brief:响应根目录请求
inline void _index_response(HttpResponse* response){
	response->setStatus(ResponseStatusCode::kOk);
	response->setContentType("text/html; charset=utf-8");
	response->addHeader("Server","dreamn/imnet");
	response->setBody(index_html,index_html + sizeof index_html);
}

//brief:响应文件请求
//becare:务必确保当前的工作目录已经调整到预定目录下
inline void _file_response(HttpResponse* response, const string& filepath){
	imnet::file_extension::Readfile data( "." + filepath);
	if(data.getErrno() != 0){
		//FIXME: 独立处理请求错误
		//defaultHttpService(request, response);
		_404_response(response);
		return ;
	}

	if(0 != data.readToString(INT_MAX, response->getBodyHandler())){
		_500_response(response);
		return ;
	}

	response->setStatus(ResponseStatusCode::kOk);
	response->setContentType(_getContentType(filepath));
	response->addHeader("Server","dreamn/imnet");
}

//brief:响应cgi请求
inline void _cgi_response(HttpResponse* response, const string& cgi){
	//TODO:!!!
	_500_response(response);
}

//brief:主业务逻辑
void searcher(const HttpRequest& request, HttpResponse* response){
	LOG_INFO << request.MethodToString(request.getMethod()) << request.getPath();

	string path = request.getPath();
	//static_assert(sizeof "/cgi-bin/" == 9, "那我数错了");
	static_assert(sizeof "/cgi-bin/" == 10, "那我数错了");

	if(path=="/"){
		_index_response(response);
		return ;
	}
	else if(path.substr(0,9) == "/cgi-bin/"){
		_cgi_response(response, path.substr(9));
		return ;
	}
	else{
		_file_response(response, path);
		return ;
	}

	//it will never be here
	defaultHttpService(request, response);
}

