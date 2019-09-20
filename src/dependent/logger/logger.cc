/*************************************************************************
	> File Name: logger.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月18日 星期日 09时31分18秒
 ************************************************************************/

#ifdef __GNUC__
#include"dependent/logger/logger.h"
#include"dependent/logger/timestamp.h"
#include"dependent/thread_local_variable.h"
#include"dependent/logger/timezone.h"

#elif defined(_MSC_VER)
#include"logger.h"
#include"timestamp.h"
#include"../thread_local_variable.h"
#include"timezone.h"
#endif


#include<cstdio>
#include<cstring>
#include<cerrno>

//正确日志格式的长度
static const int kCorrectLengthOfDateFormart = 26;

//追加部分信息
namespace imnet {
//brief:线程存储期变量
//function:缓存数据，线程安全
thread_local char tl_strerror_buffer[512];
thread_local char tl_cache_time[64];
thread_local time_t tl_cache_second;
thread_local string tl_cache_tid;//主要减少int->字符串的开销

//brief:从环境变量中确定日志输出级别
CLogger::LevelType getLogLevelFromEnvironment() {
	//不够健壮，防止恶性修改环境变量值
	//if (const char *c = ::getenv("IMNET_LOGLEVEL")) {
	//	return CLogger::LevelType(static_cast<int>(*c));
	//}
	if (::getenv("IMNET_LOG_TRACE")) {
		return CLogger::kTrace;
	}
	else if (::getenv("IMNET_LOG_DEBUG")) {
		return CLogger::kDebug;
	}
	return CLogger::kInfo;
}

CLogger::LevelType g_logger_level = getLogLevelFromEnvironment();//初始化全局级别

const char* LogLevelTypeNames[CLogger::kNumberLogLevels] = 
     { "TRACE ","DEBUG ","INFO  ","WARN  ","ERROR ","FATAL " };//各等级名称,注意包含分隔符，并且长度一致
const int kLogLevelTypeNameLength = strlen(LogLevelTypeNames[0]);

//brief:重载<<
inline CLogStream& operator<<(CLogStream &os, const CLogger::NameType &name) {
	os.append(name.ptrName, name.len);
	return os;
}

//brief:默认的日志写出
void defaultOutputFunctionForLogger(const char *msg, int len) {
	size_t n = fwrite(msg, 1, len, stdout);
	assert(n == implicit_cast<size_t>(len));
	if (static_cast<int>(n) != len) {
		fputs("Logger output error:lost message\n", stderr);
	}
}

//brief:默认的日志刷新
void defaultFlushFunctionForLogger() {
	fflush(stdout);
}

//brief:定义全局日志的输出、刷新函数以及时区
CLogger::OutputFuncType g_logger_output = defaultOutputFunctionForLogger;
CLogger::FlushFuncType g_logger_flush = defaultFlushFunctionForLogger;
CTimezone g_logger_timezone;

}//namespace imnet


//以下均为实现
using namespace imnet;

//brief:CLogger的不同构造实现
imnet::CLogger::CLogger(NameType filename, int lineno, LevelType level)
	:__implement(level,0,filename,lineno){
}
//需要输出function name的版本
imnet::CLogger::CLogger(NameType filename, int lineno, LevelType level, const char * function_name)
	:__implement(level,0,filename,lineno){
	__implement._stream << function_name << ' ';
}
//需要解析errno的版本
imnet::CLogger::CLogger(NameType filename, int lineno, bool is_abort)
	:__implement(is_abort?kFatal:kError,errno,filename,lineno){
}

//brief:追加日志起始信息
//      主要包括时间戳，线程信息，日志级别
void CLogger::CImpl::completePrefix() {

	///////////////处理时间戳//////////////////
	int microseconds = _time.getMicroseconds();
	time_t seconds = _time.getSeconds();

	//针对同一秒内的缓存处理
	if (seconds != tl_cache_second) {
		tl_cache_second = seconds;
		struct tm tm_answer;

		//TODO: CTimeZone,针对时区进行解析数据
		if (g_logger_timezone.isValid()) {
			g_logger_timezone.localtime_r(seconds,tm_answer);
		}
		else {
			::gmtime_r(&seconds, &tm_answer);
		}

		int len = snprintf(tl_cache_time, sizeof tl_cache_time, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
			tm_answer.tm_year + 1900, tm_answer.tm_mon, tm_answer.tm_mday, tm_answer.tm_hour,
			tm_answer.tm_min, tm_answer.tm_sec, microseconds);

		//FIXME:检查len值
		assert(len == kCorrectLengthOfDateFormart);
	}

#ifndef IMNET_LOG_OUTPUTNULL
	//_stream << CStringPiece(tl_cache_time, 27) << (g_logger_timezone.isValid() ? "": "Z");
	_stream << CStringPiece(tl_cache_time, kCorrectLengthOfDateFormart) << (g_logger_timezone.isValid() ? "": "Z");
#else
	//_stream << CStringPiece(tl_cache_time, 27);
	_stream << CStringPiece(tl_cache_time, kCorrectLengthOfDateFormart);
	if (!g_logger_timezone.valid()) {
		_stream << "Z";
	}
#endif

	///////////////处理线程信息//////////////////
	//_stream << thread_local_variable::gettid();//存在int转换到字符串开销
	if (__builtin_expect(tl_cache_tid.empty(), 0)) {
		//becare:分隔符
		tl_cache_tid = " " + std::to_string(thread_local_variable::gettid()) + " ";
	}
	_stream << tl_cache_tid;

	///////////////处理时间戳//////////////////
	_stream << CStringPiece(LogLevelTypeNames[_level], kLogLevelTypeNameLength);
}
//brief:追加日志末尾信息
void CLogger::CImpl::completeSuffix() {
	_stream << " - " << _filename << ':' << _line<<'\n';
}

//brief:构造函数
CLogger::CImpl::CImpl(CLogger::CImpl::LevelType level, int saved_errno, const CLogger::CImpl::NameType &filename, int line)
	:_stream(), _filename(filename), _line(line), _level(level), _time(CTimestamp::getNow()) {
	completePrefix();
	if (saved_errno) {
		_stream << getStrerror_r(saved_errno) << "(errno=" << saved_errno << ") ";
	}
}

//brief:提供线程安全的、不会溢出缓冲区的strerror
const char * imnet::getStrerror_r(int errnum)
{
	return strerror_r(errnum, tl_strerror_buffer, sizeof tl_strerror_buffer);
}

///////////CLogger部分/////////////

//brief:在析构中进行数据的刷新
CLogger::~CLogger() {
	__implement.completeSuffix();
	g_logger_output(__implement._stream.buffer().data(), 
		            __implement._stream.buffer().size());
	if (__implement._level == kFatal) {
		g_logger_flush();
		::abort();
	}
}

//brief:重新设置日志级别
void CLogger::setLogLevel(LevelType level) {
	g_logger_level = level;
}

void imnet::CLogger::setFlushFunc(FlushFuncType func){
	g_logger_flush = func;
}

void CLogger::setOutputFunc(OutputFuncType func) {
	g_logger_output = func;
}

void imnet::CLogger::setTimeZone(const CTimezone & tz){
	g_logger_timezone = tz;
}


