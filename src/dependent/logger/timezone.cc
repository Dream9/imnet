#include "timezone.h"
/*************************************************************************
	> File Name: timezone.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年08月22日 星期四 09时17分25秒
 ************************************************************************/
#ifdef __GNUC__
#include"dependent/logger/timezone.h"
#include"dependent/logger/julianday.h"
#include"dependent/noncopyable.h"
#elif defined _MSC_VER
#include"timezone.h"
#include"julianday.h"
#include"../noncopyable.h"
#endif

#include<vector>
#include<string>
#include<stdexcept>
#include<assert.h>
#include<algorithm>

#include<endian.h>


//启用Berkeley Unix: BSD 和 SunOS. 
#define _BSD_SOURCE
#ifdef _BSD_SOURCE
#pragma message("_BSD_SOURCE is defined")
#else
#pragma message("_BSD_SOURCE is not defined")
#endif


namespace imnet {
namespace detail {


//becare: tz database的工作方式是给出了不同地区在不同时间段所采取的时间调整策略（通常是政治问题）
//其文件大体组织为：
//#Rule NAME FROM TO   TYPE IN  ON        AT   SAVE LETTER/S
//Rule  US   1918 1919  -   Mar lastSun  2:00  1:00 D
//Rule  US   1918 1919  -   Oct lastSun  2:00  0    S
//Rule  US   1942 only  -   Feb 9        2:00  1:00 W # War
//Rule  US   1945 only  -   Aug 14      23:00u 1:00 P # Peace
//Rule  US   1945 only  -   Sep 30       2:00  0    S
//Rule  US   1967 2006  -   Oct lastSun  2:00  0    S
//Rule  US   1967 1973  -   Apr lastSun  2:00  1:00 D
//Rule  US   1974 only  -   Jan 6        2:00  1:00 D
//Rule  US   1975 only  -   Feb 23       2:00  1:00 D
//Rule  US   1976 1986  -   Apr lastSun  2:00  1:00 D
//Rule  US   1987 2006  -   Apr Sun>=1   2:00  1:00 D
//Rule  US   2007 max   -   Mar Sun>=8   2:00  1:00 D
//Rule  US   2007 max   -   Nov Sun>=1   2:00  0    S
//Reformatted a Bit
//From	To	On	At	Action
//1918	1919	last Sunday	in March	02:00 local	go to daylight saving time
//in October	return to standard time
//1942 only	February 9th	go to “war time”
//1945 only	August 14th	23:00 UT	rename “war time” to “peace
//time;” clocks don’t change
//September 30th	02:00 local	return to standard time
//1967	2006	last Sunday	in October
//1973	in April	go to daylight saving time
//1974 only	January 6th
//1975 only	February 23rd
//1976	1986	last Sunday	in April
//1987	2006	first Sunday
//2007	present	second Sunday in March
//first Sunday in November	return to standard time
//
//即，先列出不同的调整规则Ri（该地区曾经使用过的），然后不同时间段(按照升序)对应不同的规则
//这也是struct Transition中各个项目的意义

//brief:参照tz database组织原则，见上方，或者见  imnet/doc/tz-database/tz-how-to.html
struct Transition {
	time_t gmt_time;//实行修改的起始gmt时间
	time_t localtime;//实行修改的起始local时间
	int localtime_index;//对应的规则，即Ttinfo信息

	Transition(time_t gmt, time_t local, int index)
		:gmt_time(gmt), localtime(local), localtime_index(index) {
	}
};

//brief:方便lower_bound进行查找所处的时间范围
struct CompareForTransition {

	bool operator()(const Transition& lhs, const Transition& rhs) const{
		return lhs.gmt_time < rhs.gmt_time;
		//if (compareGmt)
		//	return lhs.gmttime < rhs.gmttime;
		//else
		//	return lhs.localtime < rhs.localtime;
	}

	bool equal(const Transition& lhs, const Transition& rhs) const{
		return lhs.gmt_time == rhs.gmt_time;
		//if (compareGmt)
		//	return lhs.gmttime == rhs.gmttime;
		//else
		//	return lhs.localtime == rhs.localtime;
	}
};

//brief:tz database中有关时间调整的规则信息,主要是用于单独拿出来压缩空间，也方便观察
//man tzfile中：
//*tzh_typecnt ttinfo entries, each defined as follows :
//
//struct ttinfo {
//	int32_t       tt_gmtoff;
//	unsigned char tt_isdst;
//	unsigned char tt_abbrind;
//};
//
//Each  structure  is written as a four - byte signed integer value for tt_gmtoff,
//in a standard byte order, followed by a one - byte value for tt_isdst and a one -
//byte  value  for tt_abbrind.In each structure, tt_gmtoff gives the number of
//seconds to be added to UT, tt_isdst tells whether tm_isdst should  be  set  by
//localtime(3) and tt_abbrind  serves  as  an index into the array of timezone
//abbreviation bytes that follow the ttinfo structure(s) in the file.
struct Ttinfo {
	time_t gmtoff;//调整量
	bool isdst;//本次调整是否是因为夏令时
	int abbrind;//规则名称的索引

	Ttinfo(int offset, bool dst, int index)
		:gmtoff(offset), isdst(dst), abbrind(index) {
	}
};
}//!namespace detail

//brief:组织规则同上
//      使用方法就是在vec_transition中找到当前时间（seconds）是位于变化的那一个部分
//      然后决定采用那种规则
//      又有transition数据是按照时间顺序升序，故可以lower_bound进行查找
struct CTimezone::DataType {
	std::vector<detail::Transition> vec_transition;//记录了所有的变化,含有对未来的预测，但是并不准确
	std::vector<detail::Ttinfo> vec_ttinfo;//记录了所有规则
	std::vector<string> vec_tzname;//记录了规则名称
	std::string abbreviation;//记录了该时区的缩写
};

namespace detail{

//brief：根据秒数填充struct tm
inline void fillHMS(size_t seconds, struct tm &tm_to_fill) {
	tm_to_fill.tm_sec = seconds % 60;
	size_t tmp = seconds / 60;
	tm_to_fill.tm_min = tmp % 60;
	tm_to_fill.tm_hour = tmp / 60;
}

//brief:采用RAII管理的的file处理
//becare:采用了对象语义，资源不可复制
//       注意fopen是可能失败的，使用前务必进行isValid检验
class ScopedFile:noncopyable {
public:
	ScopedFile(const char *filename,const char *flag)
		:fp(fopen(filename, flag)) {
	}

	~ScopedFile() {
		if (fp)
			::fclose(fp);
	}

	bool isValid()const {
		return fp;
	}
	//FILE* getFile() {
	//	return __fp;
	//}
//private:
	FILE *fp;
};

//brief:针对tzfile提供基础的读取方式，对fread的封装
//      复合了ScopedFile对象
//becare:二进制文件采用大端法记录数据
//      需要调整字节序
class CReadTzfile {
public:
	CReadTzfile(const char *filename)
		:__readfp(filename, "rb") {

	}

	//becare:fopen是可能失败的，需要做检查
	bool isValid()const {
		return __readfp.isValid();
	}

	//brief:
	//becare:会导致malloc调用，两次,RVO之后可能好点
	string readBytes(int n) {
		//char *buf=new char[n];
		char *buf = static_cast<char*>(malloc(n));
		size_t number = ::fread(buf, 1, n, __readfp.fp);
		if (number != static_cast<size_t>(n)) {
			free(buf);
			throw std::logic_error("Something was error when fread some bytes");
		}

		string out(buf,n);
		free(buf);
		return out;
	}

	//brief:
	//becare:二进制文件tzfile采用大端法存储数据
	//* Six four - byte integer values written in a standard byte order(the
	//	high - order byte of the value is written first).These values are,
	//	in order :
	//  ... ...
	//	Each  structure  is  written as a four - byte signed integer value for
	//	tt_gmtoff, in a standard byte order, followed by  a  one - byte  value
	//	for  tt_isdst  and  a one - byte value for tt_abbrind.
	int32_t readInt32() {
		int32_t out;
		size_t number = ::fread(&out, 1, sizeof(int32_t), __readfp.fp);
		if (number != sizeof(int32_t))
			throw std::logic_error("Something was error when fread a int32_t");
		return be32toh(out);
	}

	//brief:
	uint8_t readUInt8() {
		uint8_t out;
		size_t number = ::fread(&out, 1, sizeof(uint8_t), __readfp.fp);
		if (number != sizeof(uint8_t))
			throw std::logic_error("Something was error when fread a uint8_t");
		return out;
	}

private:
	ScopedFile __readfp;
};

//brief:解析tzfile文件信息
//info tzfile内容如下
//TZFILE(5)                     Linux Programmer's Manual                    TZFILE(5)
//
//NAME
//tzfile - timezone information
//
//DESCRIPTION
//The  timezone  information files used by tzset(3) are typically found under a
//directory with a name like / usr / share / zoneinfo.These  files  begin  with  a
//44 - byte header containing the following fields :
//
//*The magic four - byte ASCII sequence “TZif” identifies the file as a timezone
//information file.
//
//* A byte identifying the version of the file's format (as of 2017, either  an
//ASCII NUL, or “2”, or “3”).
//
//* Fifteen bytes containing zeros reserved for future use.
//
//* Six  four - byte  integer  values written in a standard byte order(the high -
//	order byte of the value is written first).These values are, in order :
//
//tzh_ttisgmtcnt
//The number of UT / local indicators stored in the file.
//
//tzh_ttisstdcnt
//The number of standard / wall indicators stored in the file.
//
//tzh_leapcnt
//The number of leap seconds for which data entries are stored in  the
//file.
//
//tzh_timecnt
//The  number of transition times for which data entries are stored in
//the file.
//
//tzh_typecnt
//The number of local time types for which data entries are stored  in
//the file(must not be zero).
//
//tzh_charcnt
//The  number  of bytes of timezone abbreviation strings stored in the
//file.
//
//The  above header is followed by the following fields, whose lengths
//vary depend on the contents of the header :
//
//*tzh_timecnt four - byte signed integer values  sorted  in  ascending
//order.These values are written in standard byte order.Each is
//used as a transition time(as returned by time(2))  at  which  the
//rules for computing local time change.
//
//* tzh_timecnt one - byte unsigned integer values; each one tells which
//of the different types of local time types described in  the  file
//is  associated with the time period starting with the same - indexed
//transition time.These values serve  as  indices  into  the  next
//field.
//
//* tzh_typecnt ttinfo entries, each defined as follows :
//
//struct ttinfo {
//	int32_t       tt_gmtoff;
//	unsigned char tt_isdst;
//	unsigned char tt_abbrind;
//};
//
//Each  structure  is  written as a four - byte signed integer value for
//tt_gmtoff, in a standard byte order, followed by  a  one - byte  value
//for  tt_isdst  and  a one - byte value for tt_abbrind.In each struc‐
//ture, tt_gmtoff gives the number of  seconds  to  be  added  to  UT,
//tt_isdst  tells  whether  tm_isdst should be set by localtime(3) and
//tt_abbrind serves as an index into the array of  timezone  abbrevia‐
//tion bytes that follow the ttinfo structure(s) in the file.
//
//*      tzh_leapcnt  pairs  of  four - byte values, written in standard
//byte order; the first value of each pair gives  the  nonnega‐
//tive  time(as  returned  by time(2)) at which a leap second
//occurs; the second gives the total number of leap seconds  to
//be applied during the time period starting at the given time.
//The pairs of values are sorted in ascending  order  by  time.
//Each  transition  is  for one leap second, either positive or
//negative; transitions always separated by at  least  28  days
//minus 1 second.
//
//*      tzh_ttisstdcnt  standard / wall  indicators, each  stored as a
//one - byte value; they tell whether the transition times  asso‐
//ciated  with local time types were specified as standard time
//or wall clock time, and are used when a timezone file is used
//in handling POSIX - style timezone environment variables.
//
//*      tzh_ttisgmtcnt UT / local indicators, each stored as a one - byte
//value; they tell whether the transition times associated with
//local  time types were specified as UT or local time, and are
//used when a timezone file is  used  in  handling  POSIX - style
//timezone environment variables.
//
//The localtime(3) function uses the first standard - time ttinfo struc‐
//ture in the file(or simply  the  first  ttinfo  structure  in  the
//	absence  of a standard - time structure) if either tzh_timecnt is zero
//	or the time argument is less than the first transition time recorded
//	in the file.
// .................



//brief:解析tzfile文件
//      编译时-D _TEST_TIMEZONE_NEED_DETAIL_INFO_ 用于调试信息
bool parseTzfile(const char *tzfile, CTimezone::DataType *data) {
	CReadTzfile f(tzfile);
	if (!f.isValid())
		return false;
	//
	try {
		//TODO:
		////////////以下为解析文件逻辑//////////////
		string head = f.readBytes(4);
		if (head != "TZif")
			throw std::logic_error("tzfile bad head");
		string version = f.readBytes(1);
		f.readBytes(15);

		int32_t isgmtcnt = f.readInt32();
		int32_t isstdcnt = f.readInt32();
		int32_t leapcnt = f.readInt32();
		int32_t timecnt = f.readInt32();
		int32_t typecnt = f.readInt32();
		int32_t charcnt = f.readInt32();
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
		printf("TZif version:%s isgmtcnt:%d isstdcnt:%d leapcnt:%d timecnt:%d typecnt:%d charcnt:%d\n",
				version,isgmtcnt,isstdcnt,leapcnt,timecnt,typecnt,charcnt);
#endif

		std::vector<int32_t> trans;
		std::vector<int> localtimes;
		trans.reserve(timecnt);
		for (int i = 0; i < timecnt; ++i)
		{
			trans.push_back(f.readInt32());
		}
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
		printf("trans data:\n");
		std::for_each(trans.begin(),trans.end(),[](int32_t v){printf("%d\n",v);});
		printf("\n");
#endif

		for (int i = 0; i < timecnt; ++i)
		{
			uint8_t local = f.readUInt8();
			localtimes.push_back(local);
		}
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
		printf("localtimes data:\n");
		std::for_each(localtimes.begin(),localtimes.end(),[](uint8_t v){printf("%d\n",v);});
		printf("\n");
#endif

		for (int i = 0; i < typecnt; ++i)
		{
			int32_t gmtoff = f.readInt32();
			uint8_t isdst = f.readUInt8();
			uint8_t abbrind = f.readUInt8();
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
			printf("gmtoff:%u isdst:%d abbrind:%u:\n",gmtoff,isdst,abbrind);
#endif
			data->vec_ttinfo.push_back(Ttinfo(gmtoff, isdst, abbrind));
		}


		for (int i = 0; i < timecnt; ++i)
		{

			int localIdx = localtimes[i];
			time_t localtime = trans[i] + data->vec_ttinfo[localIdx].gmtoff;
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
			printf("Transition data: gmt:%d localtime:%d localtime_index:%u\n",trans[i],localtime,localIdx);
#endif
			data->vec_transition.push_back(Transition(trans[i], localtime, localIdx));
		}

		data->abbreviation = f.readBytes(charcnt);
#ifdef _TEST_TIMEZONE_NEED_DETAIL_INFO_
		printf("%s\n",data->abbreviation.c_str());
#endif

		// leapcnt
		for (int i = 0; i < leapcnt; ++i)
		{
			// int32_t leaptime = f.readInt32();
			// int32_t cumleap = f.readInt32();
		}
		// FIXME
		(void)isstdcnt;
		(void)isgmtcnt;
		////////////以上为解析文件逻辑//////////////

	}
	catch (std::logic_error & e) {
		fprintf(stderr,"%s\n",e.what());
		return false;
	}

	return true;
}

//brief:根据变化记录和当前时间定位应用规则
//      时区数据由用户指定，也就是只能在用户制定的时区内进行时区转换
const Ttinfo* findLocaltime(const CTimezone::DataType &data, Transition sentry) {//,CompareForTransition comp)
	//TODO:业务逻辑
	const Ttinfo* local = NULL;

	CompareForTransition comp;
	if (data.vec_transition.empty() || comp(sentry, data.vec_transition.front()))
	{
		// FIXME: should be first non dst time zone
		local = &data.vec_ttinfo.front();
	}
	else
	{
		std::vector<Transition>::const_iterator transI = std::lower_bound(data.vec_transition.begin(),
			data.vec_transition.end(),
			sentry,
			comp);
		if (transI != data.vec_transition.end())
		{
			if (!comp.equal(sentry, *transI))
			{
				assert(transI != data.vec_transition.begin());
				--transI;
			}
			local = &data.vec_ttinfo[transI->localtime_index];
		}
		else
		{
			// FIXME: use TZ-env
			local = &data.vec_ttinfo[data.vec_transition.back().localtime_index];
		}
	}

	return local;
}
}//namespace detail

}//namespace imnet



using namespace imnet;

//brief:解析tzfile，数据记录在__data之中
//      如果出错，指针为空
CTimezone::CTimezone(const char * timezone_file)
	:__data(new CTimezone::DataType){
	if (!detail::parseTzfile(timezone_file, __data.get())) {
		__data.reset();
	}
}

//brief:人为指定时区信息，并放到__data之中
//becare:传入的gmt_offset单位为秒
//       实例：CST对应UTC+8时区，故应该传入8*60*60,"CST"
CTimezone::CTimezone(int gmt_offset, const char *timezone_name)
	:__data(new CTimezone::DataType) {
	__data->vec_ttinfo.emplace_back(detail::Ttinfo(gmt_offset, false, 0));
	__data->abbreviation = timezone_name;
}

//brief:
//return: -1 for error
int CTimezone::localtime_r(time_t seconds, struct tm &tm_answer)const {
	memZero(&tm_answer, sizeof tm_answer);

	detail::Transition sentry(seconds, 0, 0);
	const detail::Ttinfo *local_ttinfo = detail::findLocaltime(*__data, sentry);

	if (!local_ttinfo)
		return -1;

	time_t local_seconds = seconds + local_ttinfo->gmtoff;
	::gmtime_r(&local_seconds, &tm_answer);
	tm_answer.tm_isdst = local_ttinfo->isdst;
	//
	//如果程序中定义了_BSD_SOURCE测试宏，那么有glibc定义的tm结构还会包括两个字段，
	//一个为long int tm_gmtoff，用于表示时间超出UTC以东的秒数，
	//一个为const char* tm_zone，用于表示时区的缩写
	//
#ifdef _BSD_SOURCE
	tm_answer.tm_gmtoff = local_ttinfo->gmtoff;
	tm_answer.tm_zone = &__data->abbreviation[local_ttinfo->abbrind];
#endif
	return 0;
}


