/*************************************************************************
	> File Name: logger.h
	> Author: Dream9
	> Brief: 封装一个非线程安全的、非异步的、带固定缓冲区的日志类
	> Created Time: 2019年08月18日 星期日 09时31分00秒
 ************************************************************************/

#ifndef _IMNET_DEPENDENT_LOGGER_H_
#define _IMNET_DEPENDENT_LOGGER_H_

#ifdef __GNUC__
#include"dependent/logger/log_stream.h"
#include"dependent/logger/timestamp.h"
#elif defined _MSC_VER
#include"log_stream.h"
#include"timestamp.h"
#endif

namespace imnet {
//brief:前向声明，减少定义依赖性
class CTimezone;

//brief:日志类
//      同时定义了相关的宏，用于日志输出
//      核心在于，复合了CLogStream,
//      本类析构时，将内容刷新到STD_OUT(默认)
//becare:非线程安全，每次输出间都有可能被打断
//       日志输出格式为:"时间戳(带微秒) 线程号 日志级别 信息 - 文件名:行号\n"
class CLogger {
public:

	//brief:定义日志级别
	enum LogLevel {
		kTrace=0,
		kDebug,
		kInfo,
		kWarn,
		kError,
		kFatal,
		kNumberLogLevels,
	};

	//brief:截取文件名，去除路径
	//becare:主要针对__FILE__的字符串指针调整，并非真正的删除式去除
	class CSourceFileName {
	public:
		//brief:构造的过程中调整指针指向
		explicit CSourceFileName(const char *filename) :ptrName(filename) {
			const char *ptr_last_slash=strrchr(filename,'/');
			ptrName = ptr_last_slash ? ptr_last_slash + 1 : ptrName;
			len = static_cast<int>(strlen(ptrName));
		}
		//brief:针对数组的特殊处理
		//      主要在于减少strlen的计算
		//becare:前提是SIZE大小的数组正好被完全填充，否则可能带来一些unexpected的信息
		template<int SIZE>
		CSourceFileName(const char(&arr)[SIZE]):ptrName(arr),len(SIZE-1) {
			const char *ptr_last_slash = strrchr(ptrName, '/');
			if (ptr_last_slash) {
				ptrName = ptr_last_slash + 1;
				len -= static_cast<int>(ptrName - arr);
			}
		}

		//breif:数据成员
		const char *ptrName;
		int len;
	};

	using LevelType = LogLevel;
	using NameType = CSourceFileName;
	using FlushFuncType = void(*)();
	using OutputFuncType = void(*)(const char*, int);

	static LevelType getLogLevel();
	static void setLogLevel(LevelType level);
	static void setFlushFunc(FlushFuncType func);
	static void setOutputFunc(OutputFuncType func);
	static void setTimeZone(const CTimezone &tz);

	CLogStream& getStream() {
		return __implement._stream;
	}

	//CLogger(NameType filename, int lineno);
	CLogger(NameType filename, int lineno, LevelType level);
	CLogger(NameType filename, int lineno, LevelType level, const char * function_name);//额外增加__FUNCTION__信息
	CLogger(NameType filename, int lineno, bool is_abort);//由于系统调用问题导致可能调用abort()
	~CLogger();//析构时进行数据刷新

private:
	//brief:定义Impl,作为嵌套类，同时作为private，仅限本类使用
	class CImpl {
	public:
		using LevelType = CLogger::LevelType;
		using NameType = CLogger::NameType;
		using TimeType = CTimestamp;
		CImpl(LevelType level, int saved_errno, const NameType &filename, int lineno);
		void completeSuffix();
		void completePrefix();

		//brief:底层数据成员,记录日志所需要的信息
		CLogStream _stream;
		NameType _filename;
		int _line;
		LevelType _level;
		TimeType _time;
		
	};

	CImpl __implement;
};

//brief:全局的日志输出级别
extern CLogger::LevelType g_logger_level;
inline CLogger::LevelType CLogger::getLogLevel() {
	return g_logger_level;
}

//brief:以宏的形式提供流式输出
//becare:无论采用哪种初始化，最终日志格式是一致的
//       日志输出格式为:"时间戳(带微秒) 线程号 日志级别 信息 - 文件名:行号\n"
//       kTrace以及kDebug级别的日志会在信息前面追加__FUNCTION__
//       kFatal级别的日志会导致abort()调用
//       由于kTrace,kDebug和kInfo需要if判断全局输出等级，因此宏展开时，切记产生else不匹配问题
//       LOG_SYSERR以及LOG_SYSFATAL涉及到errno的处理
#define LOG_TRACE if(imnet::CLogger::getLogLevel()<=imnet::CLogger::kTrace)\
imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kTrace,__FUNCTION__).getStream()
#define LOG_DEG if(imnet::CLogger::getLogLevel()<=imnet::CLogger::kDebug)\
imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kDebug,__FUNCTION__).getStream()
#define LOG_INFO if(imnet::CLogger::getLogLevel()<=imnet::CLogger::kInfo)\
imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kInfo).getStream()

#define LOG_WARN imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kWarn).getStream()
#define LOG_ERROR imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kError).getStream()
#define LOG_FATAL imnet::CLogger(__FILE__,__LINE__,imnet::CLogger::kFatal).getStream()

#define LOG_SYSERR imnet::CLogger(__FILE__,__LINE__,false).getStream()
#define LOG_SYSFATAL imnet::CLogger(__FILE__,__LINE__,true).getStream()

//brief:确保non-nullptr的检查
#define ENSURE_NOTNULL(ptr) _ensureNotNull(__FILE__, __LINE__, \
"'"#ptr"' should be not nullptr",(val))

template<typename T>
T* _ensureNotNull(CLogger::NameType filename, int lineno, const char *message, T *ptr) {
	if (!ptr) {
		LOG_FATAL << message;
	}
	return ptr;
}

const char* getStrerror_r(int errnum);

}//namespace imnet

#endif

