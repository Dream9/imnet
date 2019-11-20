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
#include"response.h"//搜索引擎的相关头文件
#include"query.h"

#include<sys/time.h>
#include<unistd.h>

#include<time.h>
#include<limits.h>
#include<stdlib.h>
#include<unordered_map>

using namespace imnet;
using namespace imnet::core;
using namespace imnet::http;

//brief:工作目录
const char* work_diretory = "/mnt/share/imnet/doc/http_resource/";

void searcher(const HttpRequest& request, HttpResponse* response);
void _init_thread_func(Eventloop* loop);
int _search_response(const string& word, string& response);

//FIXME:
//brief:全局采用统一的搜索配置
Query* global_query_agent;
int page_no = 1;
const char *rawpath= "/mnt/share/Dreamn/DreamSE/src/doc/raw.test";
std::unordered_map<int, int>doc_index;
int total_doc;
const char *doc_index_path = "/mnt/share/Dreamn/DreamSE/src/doc/doc.index.test";
unordered_map<string, string>inverse_index;
const char *inverse_index_file = "/mnt/share/Dreamn/DreamSE/src/doc/inverse.index.test";
Dictinary dict;
const char *dict_path = "/mnt/share/Dreamn/DreamSE/src/doc/word.dict";
score_container ans;
std::vector<string> words;
std::vector<float> idf;

int main(int argc, char**argv){
	LOG_INFO << argv[0] << ",pid = " << process_information::getPid() <<", Listen at 5996.";


	///////////////////////////////////////////////////////////
	//
	//
	//初始化查询环境
	chdir("/mnt/share/Dreamn/DreamSE/src/test/");
	Query query_agent;
	query_agent.EnvironmentInit(doc_index,doc_index_path,inverse_index,inverse_index_file);
	total_doc = doc_index.size();
	query_agent.GetDictinary(dict, dict_path);
	global_query_agent = &query_agent;
	//
	//
	///////////////////////////////////////////////////////////

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
	server.setThreadInitCallback(_init_thread_func);
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
void _cgi_response(HttpResponse* response, const HttpRequest& request){
	string cgi = request.getPath().substr(9);

	//检查请求的cgi程序
	size_t head_len = sizeof "search.cgi" -1;
	if(cgi.size() < head_len ||
			0 != memcmp(cgi.c_str(),"search.cgi", head_len )){
		_404_response(response);
		return ;
	}

	//获取结果
	//if(0!= _search_response(cgi.substr(head_len), response->getBodyHandler())){
	if(0!= _search_response(request.getQuery(), response->getBodyHandler())){
		_500_response(response);
		return ;
	}
	
	response->setStatus(ResponseStatusCode::kOk);
	response->setContentType("text/html; charset=gbk");
	response->addHeader("Server","dreamn/imnet");
}

//brief:查询并响应结果
int _search_response(const string& word, string& response){

	timeval begin_tv;
	gettimeofday(&begin_tv,nullptr);

	//FIXME:采用Apache的实现方式？
	//global_query_agent->GetInput();//原本是针对Apache的CGI 程序
	int ret = global_query_agent->SetQueryParameter(word);
	if(ret<0)
		return -1;
	
	global_query_agent->HandleQueryString(dict);
	global_query_agent->GetAnswer(total_doc,inverse_index,ans,words,idf);
	timeval end_tv;
	gettimeofday(&end_tv,nullptr);
	float used_msec = (end_tv.tv_sec-begin_tv.tv_sec)*1000 
		+((float)(end_tv.tv_usec-begin_tv.tv_usec))/(float)1000; 

	//结果反馈
	Response feedback;
	feedback.ResponseTop(global_query_agent->m_query_string.c_str(), response);
	feedback.ResponseResult(global_query_agent->m_query_string.c_str(), 
			ans, page_no, rawpath, doc_index,words,idf, response);

	feedback.ResponseSelectPage(ans.size(),used_msec,1,"456asd1f98sdf", response);

	return 0;
	//todo:最后的字符串表示缓存的该词的信息id

}

//brief:主业务逻辑
void searcher(const HttpRequest& request, HttpResponse* response){
	LOG_INFO << request.MethodToString(request.getMethod()) << request.getPath();

	//TODO
	if(request.getMethod()!= Method::kGet){
		//仅保留GET请求
		_500_response(response);
		return ;
	}

	string path = request.getPath();
	//static_assert(sizeof "/cgi-bin/" == 9, "那我数错了");
	static_assert(sizeof "/cgi-bin/" == 10, "那我数错了");

	if(path=="/"){
		_index_response(response);
		return ;
	}
	else if(path.substr(0,9) == "/cgi-bin/"){
		//FIXME:request中还含有其他有用信息
		//_cgi_response(response, path.substr(9));
		_cgi_response(response, request);
		return ;
	}
	else{
		_file_response(response, path);
		return ;
	}

	//it will never be here
	defaultHttpService(request, response);
}

//为每个线程进行业务逻辑的初始化
void _init_thread_func(Eventloop* loop){
	loop->pushBackIntoLoop(std::bind(chdir,work_diretory));
}
