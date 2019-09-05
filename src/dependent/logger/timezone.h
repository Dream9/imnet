/*************************************************************************
	> File Name: timezone.h
	> Author: Dream9
	> Brief: 提供时区转换功能
	> Created Time: 2019年08月22日 星期四 09时17分14秒
 ************************************************************************/
#ifndef _IMNET_DEPENDENT_TIMEZONE_H_
#define _IMNET_DEPENDENT_TIMEZONE_H_

#ifdef __GNUC__
#include"dependent/logger/julianday.h"
#elif defined _MSC_VER
#include"julianday.h"
#endif

#include<memory>
#include<ctime>

namespace imnet {

//brief:支持时区转换的时间获取
//becare:需要通过tz database二进制文件指定时区信息
//       格式信息见man tzfile
//       所谓时区转换说白了就是根据时区name确定当前在GMT基础上偏移量gmt_offset，并标注是否为夏令时isdst
//       而tz database是由第三方机构提供的持续维护的有关时区的上述映射信息
class CTimezone {
public:

	explicit CTimezone(const char *timezone_file);
	CTimezone(int gmt_offset, const char *timezone_name);
	CTimezone() = default;
	
	bool isValid()const {
		return static_cast<bool>(__data);
	}

	int localtime_r(time_t seconds, struct tm &tm_answer)const;
	struct DataType;

private:
	std::shared_ptr<DataType> __data;
};
}// !namespace imnet
#endif // !_IMNET_DEPENDENT_TIMEZONE_H_


