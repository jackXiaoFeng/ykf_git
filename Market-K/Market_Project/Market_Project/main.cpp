#pragma once
// main.cpp : 
//一个简单的例子，介绍QdpFtdcMduserApi和QdpFtdcMduserSpi接口的使用。

#include <stdio.h>
#include <string.h>
#include <float.h>
#include "QdpFtdcMdApi.h"
#include <ctype.h>
#include "DBOperation.h"  
#include <iostream>  
#include <time.h>
#include <ctype.h>
#include <process.h> 
#include <hiredis.h>
#include <ocilib.h>
#include "cJSON.h"
#include "MyLogger.h"
#include "Config.h"

#define prefix ""

#define NO_QFORKIMPL //这一行必须加才能正常使用

#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"ociliba.lib")
#pragma comment(lib,"ocilibm.lib")
#pragma comment(lib,"ocilibw.lib")
using namespace std;
CRITICAL_SECTION g_cs;

MyLogger * myLoger;

HANDLE hMutex;
CDBOperation dbOper;

redisContext *rc;
redisReply *reply;

char   dateTime[100];

//当前整点时间戳
int nowTimestamp_Zero;
//当前时间戳多出整点多少
int	nowTimestamp_Surplus;

int week_v = 0;
int month_v = 0;

//market tcp
char marketAddress[50];
//oracle
char oracle_DomainName[50];
int  oracle_Port;
char oracle_UserID[50];
char oracle_Password[50];

//redis
char redis_DomainName[50];
int  redis_Port;

char charlist[50][50];
int holidaysNumber;

int MomdayTime;
int nowDayOfWeek;

int startMonTime;
int nowDayOfMonth;

int agtd_week;
int autd_week;
int au100g_week;
int mautd_week;

int agtd_month;
int autd_month;
int au100g_month;
int mautd_month;

unsigned   Counter;
//连接redis
void ConnrectionRedis()
{
	// 连接Redis
	rc = redisConnect(redis_DomainName, redis_Port);
	if (rc == NULL || rc->err) {
		if (rc) {
			printf("Connection error: %s\n", rc->errstr);
			redisFree(rc);
		}
		else {
			printf("Connection error: can't allocate redis context\n");
		}
		//exit(1);
		//return;
	}
	else
	{
		printf("%s连接redis成功!\n", prefix);
	}
	//freeReplyObject(reply);

	// 释放rc资源
	//redisFree(rc);
}

//连接Oracle
void ConnrectionOracle()
{
	char str[500];
	sprintf_s(str, "Provider = OraOLEDB.Oracle.1; User ID = %s; Password = %s; Data Source = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = %s)(PORT = %d))(CONNECT_DATA = (SERVICE_NAME = ORCL))); Persist Security Info = False", oracle_UserID, oracle_Password, oracle_DomainName, oracle_Port);
	bool bConn = dbOper.ConnToDB(str, oracle_UserID, oracle_Password);
	if (false == bConn)
	{
		printf("连接数据库出现错误\n");
		//return;
	}
	else
	{
		printf("连接oracle成功!\n");
	}
}


void err_handler(OCI_Error *err)
{
	printf("Error ORA-%05i - msg : %s\n", OCI_ErrorGetOCICode(err), OCI_ErrorGetString(err));
}

//字符串转化为时间戳
time_t StringToDatetime(const char *strTime)
{
	if (NULL == strTime)
	{
		return 0;
	}
	tm tm_;
	int year, month, day, hour, minute, second;
	sscanf(strTime, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
	tm_.tm_year = year - 1900;
	tm_.tm_mon = month - 1;
	tm_.tm_mday = day;
	tm_.tm_hour = hour;
	tm_.tm_min = minute;
	tm_.tm_sec = second;
	tm_.tm_isdst = 0;

	time_t t_ = mktime(&tm_); //已经减了8个时区  
	return t_; //秒时间  
}

//时间戳转化为字符串
void DatetimeToString(time_t time)
{
	time_t t = time + 28800;//8h 时区差
	struct tm *p;
	p = gmtime(&t);
	strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", p);
}

void stamp_to_standard(int stampTime, char *a, char *week)
{
	time_t tick = (time_t)stampTime;
	struct tm tm;
	char s[100];
	char w[100];

	//tick = time(NULL);
	tm = *localtime(&tick);
	//strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &tm);
	strftime(s, sizeof(s), "%H:%M", &tm);
	strftime(w, sizeof(w), "%A", &tm);

	//printf("时间戳%d: 日期时分：%s：%s\n", (int)tick, s, w);
	strcpy(a, s);
	strcpy(week, w);
}

int compare_time_60(int time, int time_interval_local, char *marketDate)
{
	int endTime_60;
	char s[30];
	if (nowTimestamp_Surplus >= 7200 && nowTimestamp_Surplus < 9060)
	{
		sprintf_s(s, "%s %s", marketDate, "09:30:00");
		endTime_60 = (int)StringToDatetime(s);
	}
	else
	{
		if ((nowTimestamp_Surplus >= 9000 && nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >= 41400 && nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >= 55800 && nowTimestamp_Surplus < 56700))
		{
			//特殊收盘时间k线不加间隔
			endTime_60 = time;
		}
		else
		{
			endTime_60 = time + time_interval_local;
		}

		//去除分钟数
		endTime_60 = endTime_60 - int(endTime_60) % int(time_interval_local);

		if (nowTimestamp_Surplus >= 7200 && nowTimestamp_Surplus < 56700)
		{
			if (time % (60 * 60) < 1800)
			{
				endTime_60 = endTime_60 - 30 * 60;
			}
			else
			{
				endTime_60 = endTime_60 + 30 * 60;
			}
		}
	}
	return endTime_60;
}

int compare_time_240(char *marketDate)
{
	int endTime_240;
	char s[30];

	//00:00 10:30 15:30 00:00
	if (nowTimestamp_Surplus >= 0 && nowTimestamp_Surplus < 37800)
	{
		sprintf_s(s, "%s %s", marketDate, "10:30:00");
		endTime_240 = (int)StringToDatetime(s);
	}
	else if (nowTimestamp_Surplus >= 37800 && nowTimestamp_Surplus < 56700)
	{
		endTime_240 = nowTimestamp_Zero + 55800;
	}
	else
	{
		endTime_240 = nowTimestamp_Zero + 86400;
	}

	return endTime_240;
}

int get_time_week(time_t lt, tm *ptr)
{
	//int endTime_week;
	//00:00 10:30 15:30 00:00
	nowDayOfWeek = 0;
	MomdayTime = 0;

	int wday = ptr->tm_wday;
	nowDayOfWeek = (wday == 0) ? 6 : wday - 1; //今天是本周的第几天。周一=0，周日=6
	int nowTime = (int)lt;
	/*time_t now;
	int nowTime = (int)time(&now);
	nowTime = 1489687830;*/
	//周一
	MomdayTime = nowTime - nowDayOfWeek * 24 * 60 * 60;

	//周五 
	//int SaturdayTime = MomdayTime + 4 * 24 * 60 * 60;

	//如果是周六则算为下周
	/*char week[20] = "";
	strftime(week, sizeof(week), "%A", ptr);
	if (strcmp(week, "Saturday") == 0)
	{
	SaturdayTime = SaturdayTime + 7 * 24 * 60 * 60;
	}
	endTime_week = SaturdayTime - (int(SaturdayTime) + 28800) % int(86400);*/

	MomdayTime = MomdayTime - (int(MomdayTime) + 28800) % int(86400);

	return MomdayTime;
}

int get_time_month(tm *ptr)
{
	//int endTime_month;
	nowDayOfMonth = 0;
	startMonTime = 0;
	//月k
	char jointDate[30] = "";
	int mday = ptr->tm_mday;
	nowDayOfMonth = mday - 1; //今天是本月的第几天。1-31
							  //月初
	sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1, 1, 0, 0, 0);
	startMonTime = (int)StringToDatetime(jointDate);

	//月末
	/*sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1 + 1, 1, 0, 0, 0);
	int endMon = StringToDatetime(jointDate);
	endMon = endMon - 24 * 60 * 60;
	endTime_month = endMon - (int(endMon) + 28800) % int(86400);*/

	return startMonTime;
}

// 获取时间段的成交量
int get_sum_v(int num, char *InstrumentID, int time)
{
	int sum = 0;
	char tempName[100] = "";
	sprintf_s(tempName, "%s_%s", InstrumentID, "f1_day");

	for (int x = 0; x < num; x++)
	{
		int timeV = time + x * 24 * 60 * 60;
		reply = (redisReply *)redisCommand(rc, "HGET  %s %d", tempName, timeV);
		if (reply != NULL && reply->str != NULL)
		{
			cJSON *root = cJSON_Parse(reply->str);
			int  total_v = cJSON_GetObjectItem(root, "v")->valueint;
			sum += total_v;
			//printf("最小time=%d--最大time=%d=dds-%d=实时V=%d==最大V=%d==间隔V=%d--\n", min_time, max_time, dds,v, total_v, fenshiInterval_v);
		}
		freeReplyObject(reply);
	}
	return sum;
}

char* substring(char* ch, int pos, int length)
{
	char* pch = ch;
	//定义一个字符指针，指向传递进来的ch地址。  
	char* subch = (char*)calloc(sizeof(char), length + 1);
	//通过calloc来分配一个length长度的字符数组，返回的是字符指针。  
	int i;
	//只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性。  
	pch = pch + pos;
	//是pch指针指向pos位置。  
	for (i = 0; i<length; i++)
	{
		subch[i] = *(pch++);
		//循环遍历赋值数组。  
	}
	subch[length] = '\0';//加上字符串结束符。  
	return subch;       //返回分配的字符数组地址。  
}

//删除特殊字符  || 替换其他特殊字符
void delAndReplace(char *str, char *d, char oldChar, char newChar)
{
	int i = 0, j = 0;
	while (str[i] != '\0')
	{
		//字母小写变大写
		str[i] = toupper(str[i]);

		if (str[i] == oldChar)
		{
			str[i] = newChar;
		}
		if ((str[i] != d[0]) & (str[i] != d[1]) & (str[i] != d[2]))
		{
			str[j++] = str[i++];
		}
		else
		{
			i++;
		}
	}
	str[j] = '\0';
}

//unsigned   __stdcall   SecondThreadFunc(void*   pArguments)
//{
//	Play_Param *pp = (Play_Param *)pArguments;
//
//	int aa = pp->sliderRange;
//
//
//	while (Counter   <   1000000)
//		Counter++;
//
//	printf("SecondThreadFunc-%d--%d\n", aa, Counter);
//	_endthreadex(0);
//	return   0;
//}

BOOL isHolidaysFunction(int i, const char charlist[50][50], char *nowDate)
{
	BOOL isH = false;
	int len = i;
	for (int j = 0; j < len; j++)
	{
		const char * a = NULL;
		if (strcmp(nowDate, charlist[j]) == 0)
		{
			isH = true;
			break;
		}
		else
		{
			isH = false;
		}
	}

	return isH;
}

//输出行情信息
void printf_Market(CQdpFtdcDepthMarketDataField *pMarketData)
{
	// 客户端按需处理返回的数据
	printf(
		"\n<<<<<< \n交易日:%s,  结算组代码:%s,  结算编号:%d,\
            \n合约代码:%s,  最后修改时间:%s,  最后修改毫秒:%d,  交易所代码:%s，\
            \n昨结算:%f,  昨收盘:%f,  昨持仓量:%f,  昨虚实度:%f,\
            \n今开盘:%f,  最高价:%f,  最低价:%f,  今收盘:%f,\
            \n涨停板价:%f,  跌停板价:%f,  今结算:%f,  今虚实度:%f,\
            \n最新价:%f,  数量:%d,  成交金额:%f,  持仓量:%f,\
            \n申买价一:%f,  申买量一:%d,  申卖价一:%f,  申卖量一:%d,\
            \n申买价二:%f,  申买量二:%d,  申卖价二:%f,  申卖量二:%d,\
            \n申买价三:%f,  申买量三:%d,  申卖价三:%f,  申卖量三:%d,\
            \n申买价四:%f,  申买量四:%d,  申卖价四:%f,  申卖量四:%d,\
            \n申买价五:%f,  申买量五:%d,  申卖价五:%f,  申卖量五:%d,\n>>>>>\
            \n\n",

		//交易
		pMarketData->TradingDay,
		pMarketData->SettlementGroupID,
		pMarketData->SettlementID,

		pMarketData->InstrumentID,
		pMarketData->UpdateTime,
		pMarketData->UpdateMillisec,
		pMarketData->ExchangeID,

		//昨
		pMarketData->PreSettlementPrice,
		pMarketData->PreClosePrice,
		pMarketData->PreOpenInterest,
		pMarketData->PreDelta,

		//今
		pMarketData->OpenPrice,
		pMarketData->HighestPrice,
		pMarketData->LowestPrice,
		pMarketData->ClosePrice,

		pMarketData->UpperLimitPrice,
		pMarketData->LowerLimitPrice,
		pMarketData->SettlementPrice,
		pMarketData->CurrDelta,

		//其他
		pMarketData->LastPrice,
		pMarketData->Volume,
		pMarketData->Turnover,
		pMarketData->OpenInterest,

		//申一
		pMarketData->BidPrice1,
		pMarketData->BidVolume1,
		pMarketData->AskPrice1,
		pMarketData->AskVolume1,

		//二
		pMarketData->BidPrice2,
		pMarketData->BidVolume2,
		pMarketData->AskPrice2,
		pMarketData->AskVolume2,

		//三
		pMarketData->BidPrice3,
		pMarketData->BidVolume3,
		pMarketData->AskPrice3,
		pMarketData->AskVolume3,

		//四
		pMarketData->BidPrice4,
		pMarketData->BidVolume4,
		pMarketData->AskPrice4,
		pMarketData->AskVolume4,

		//五
		pMarketData->BidPrice5,
		pMarketData->BidVolume5,
		pMarketData->AskPrice5,
		pMarketData->AskVolume5
	);
}
unsigned int __stdcall ThreadFunc(void* pM)
{
	CQdpFtdcDepthMarketDataField *pMarketData = (CQdpFtdcDepthMarketDataField *)pM;
	//临界区
	EnterCriticalSection(&g_cs);

	try {

		if (rc == NULL)
		{
			//重连
			ConnrectionRedis();
		}

		//输出行情
		//printf_Market(pMarketData);

		//行情更新时间为  本地日期年月日 拼接行情时分秒
		char nowtDate[30] = "";
		char marketDate[12] = "";
		char market_time_str[30] = "";
		char r_Market_time_str[30] = "";
		char time_hms[9] = "";

		struct tm *ptr;
		time_t lt;
		lt = time(NULL);
		ptr = localtime(&lt);
		//如果是双休节假日 则用行情日期
		int wday = ptr->tm_wday;
		wday = (wday == 0) ? 6 : wday - 1; //今天是本周的第几天。周一=0，周日=6	  
										   //fix 1:年月日用本地年月日拼接行情时分秒 因为行情时间晚八点之后日期会变为下一天
										   //本地日期+行情时分秒 用于计算 分钟k
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d", ptr);
		strcpy(market_time_str, nowtDate);
		strcpy(time_hms, pMarketData->UpdateTime);
		strcat(market_time_str, " ");
		strcat(market_time_str, time_hms);
		int market_Updatetimes = (int)StringToDatetime(market_time_str);


		//因行情时间有延迟 故在23:59:59秒 拼接的时候会出现本地日期已经过一天 行情时间还是上一天的bug 所以判断如果行情时间戳大于本地时间戳和大于不只一小时条件 就减去一个交易日  但是日周月k则不必减去直接就给取余数不影响
		int now = (int)lt;
		bool isDayChange = false;
		if (market_Updatetimes > now && (market_Updatetimes - now) > 3600)
		{
			market_Updatetimes -= 24 * 60 * 60;
			isDayChange = true;
		}

		//行情日期+行情时分秒 用于计算日/周/月k
		int r_Market_Updatetimes;
		strcpy(marketDate, pMarketData->TradingDay);
		sprintf_s(marketDate, "%s-%s-%s", substring(marketDate, 0, 4), substring(marketDate, 4, 2), substring(marketDate, 6, 2));

		if ((strcmp(marketDate, nowtDate) != 0) || isDayChange)
		{
			strcpy(r_Market_time_str, marketDate);
			strcat(r_Market_time_str, " ");
			strcat(r_Market_time_str, time_hms);
			r_Market_Updatetimes = (int)StringToDatetime(r_Market_time_str);
		}
		else
		{
			r_Market_Updatetimes = market_Updatetimes;
		}


		struct tm *r_ptr;
		time_t r_lt;
		//9:00-11:30 13:30-15:30 20:00-24:00 00:00-02:00
		/*#20:00 = 72000,
		2:30 = 9000,2:31 = 9060,
		9:00 = 32400,
		11:30 = 41400, 11:31=4160
		13:30 = 48600,
		15:30 = 55800 15:30 = 55860 15:45 = 56700  日周月k结算收盘价在15:45分才结束所以 要增加到15:45分
		*/
		//根据行情更新时间来确定是否是在开盘价之内 是否存redis
		int dds = market_Updatetimes;
		nowTimestamp_Surplus = (int(dds) + 28800) % int(86400);
		nowTimestamp_Zero = dds - nowTimestamp_Surplus;

		//停盘时间加一分钟 是为了结束时30:00减去前一个时间段 算出成交量
		if ((nowTimestamp_Surplus >= 72000 || nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >= 32400 && nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >= 48600 && nowTimestamp_Surplus < 56700))
		{
			//printf("开盘时间");56700
		}
		else
		{
			//printf("休盘时间\n");
			//休盘时间 直接跳出回调函数
			//_endthreadex(0);
			LeaveCriticalSection(&g_cs);
			return   0;
		}

		BOOL isHolidays = isHolidaysFunction(holidaysNumber, charlist, nowtDate);
		if (isHolidays || wday > 5 || (wday == 5 && nowTimestamp_Surplus > 9060))
		{
			LeaveCriticalSection(&g_cs);
			return 0;
		}

		//是否是停盘最后一分钟
		bool isLastMinutes = false;
		if ((nowTimestamp_Surplus >= 9000 && nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >= 41400 && nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >= 55800 && nowTimestamp_Surplus < 56700))
		{
			//特殊收盘时间k线不加间隔
			isLastMinutes = true;
		}

		//是否是盘中开盘一分钟（不包括晚20:00 开盘） 
		bool isStarMinutes = false;
		if ((nowTimestamp_Surplus >= 32400 && nowTimestamp_Surplus < 32460) ||
			(nowTimestamp_Surplus >= 48600 && nowTimestamp_Surplus < 48660))
		{
			//按照 开盘前的成交量计算 确保开盘成交量准确性
			isStarMinutes = true;
		}

		//合约标示去除多余字符 并转成大写字符
		char origin_InstrumentID[31] = "";
		char InstrumentID[30] = "";
		strcpy(origin_InstrumentID, pMarketData->InstrumentID);


		//品种克数计算 影响均价  总额除以总手数
		int grammage = 1;
		if (strcmp(origin_InstrumentID, "Ag(T+D)") == 0)
		{
			grammage = 100;
			strcpy(InstrumentID, "AGTD");
		}
		else if (strcmp(origin_InstrumentID, "Au(T+D)") == 0)
		{
			grammage = 100000;
			strcpy(InstrumentID, "AUTD");
		}
		else if (strcmp(origin_InstrumentID, "Au100g") == 0)
		{
			grammage = 10000;
			strcpy(InstrumentID, "AU100G");
		}
		else if (strcmp(origin_InstrumentID, "mAu(T+D)") == 0)
		{
			grammage = 10000;
			strcpy(InstrumentID, "MAUTD");
		}
		printf("%sK线种类：%s拼接时间：%s时间戳：%d==\n", prefix, InstrumentID, time_hms, market_Updatetimes);

		//定义并赋值，存储redis行情信息
		double maxl = pMarketData->HighestPrice;
		double minl = pMarketData->LowestPrice;
		double sl = pMarketData->OpenPrice;
		double el = pMarketData->LastPrice;
		double closeP = pMarketData->ClosePrice;
		int v = pMarketData->Volume;
		double turnover = pMarketData->Turnover;

		//存储信息
		char myhash[50] = "";
		char myhash_v[50] = "";
		char value[200] = "";

		char *nameArray[10] = { "","","" ,"","","" };
		nameArray[0] = "f1_minutes";
		nameArray[1] = "f5_minutes";
		nameArray[2] = "f15_minutes";
		nameArray[3] = "f30_minutes";
		nameArray[4] = "f60_minutes";
		nameArray[5] = "f240_minutes";
		nameArray[6] = "fenshi_1_minutes";
		nameArray[7] = "f1_day";
		nameArray[8] = "f1_week";
		nameArray[9] = "f1_month";

		int time_interval_local_array[10] = {
			60,
			5 * 60,
			15 * 60,
			30 * 60,
			60 * 60,
			240 * 60,
			60,
			24 * 60 * 60,
			3 * 24 * 60 * 60,
			32 * 24 * 60 * 60
		};

		char name[30] = "";
		int time_interval_local;

		lt = time_t(market_Updatetimes);
		ptr = localtime(&lt);

		r_lt = time_t(r_Market_Updatetimes);
		r_ptr = localtime(&r_lt);

		//去除时间戳秒数
		int ss = int(market_Updatetimes) % int(60);
		market_Updatetimes = market_Updatetimes - ss;

		r_Market_Updatetimes = r_Market_Updatetimes - ss;

		int r_TempTime = r_Market_Updatetimes - (int(r_Market_Updatetimes) + 28800) % int(86400);

		//程序重启 全局周 月 初始化0 只计算一次周 月成交量
		if (agtd_week == 0 ||
			autd_week == 0 ||
			au100g_week == 0 ||
			mautd_week == 0)
		{
			int st_w = get_time_week(r_lt, r_ptr);
			//根据日成交量 计算今天之前周成交量
			int week = get_sum_v(nowDayOfWeek, InstrumentID, st_w);

			week_v = week;
			if (strcmp(InstrumentID, "AGTD") == 0)
			{
				agtd_week = week;
			}
			else if (strcmp(InstrumentID, "AUTD") == 0)
			{
				autd_week = week;
			}
			else if (strcmp(InstrumentID, "AU100G") == 0)
			{
				au100g_week = week;
			}
			else if (strcmp(InstrumentID, "MAUTD") == 0)
			{
				mautd_week = week;
			}
		}
		else
		{
			if (strcmp(InstrumentID, "AGTD") == 0)
			{
				week_v = agtd_week;
			}
			else if (strcmp(InstrumentID, "AUTD") == 0)
			{
				week_v = autd_week;
			}
			else if (strcmp(InstrumentID, "AU100G") == 0)
			{
				week_v = au100g_week;
			}
			else if (strcmp(InstrumentID, "MAUTD") == 0)
			{
				week_v = mautd_week;
			}
		}

		if (agtd_month == 0 ||
			autd_month == 0 ||
			au100g_month == 0 ||
			mautd_month == 0)
		{
			//月k
			int st_m = get_time_month(r_ptr);
			//根据日成交量 计算今天之前月成交量
			int month = get_sum_v(nowDayOfMonth, InstrumentID, st_m);

			month_v = month;
			if (strcmp(InstrumentID, "AGTD") == 0)
			{
				agtd_month = month;
			}
			else if (strcmp(InstrumentID, "AUTD") == 0)
			{
				autd_month = month;
			}
			else if (strcmp(InstrumentID, "AU100G") == 0)
			{
				au100g_month = month;
			}
			else if (strcmp(InstrumentID, "MAUTD") == 0)
			{
				mautd_month = month;
			}

		}
		else
		{
			if (strcmp(InstrumentID, "AGTD") == 0)
			{
				month_v = agtd_month;
			}
			else if (strcmp(InstrumentID, "AUTD") == 0)
			{
				month_v = autd_month;
			}
			else if (strcmp(InstrumentID, "AU100G") == 0)
			{
				month_v = au100g_month;
			}
			else if (strcmp(InstrumentID, "MAUTD") == 0)
			{
				month_v = mautd_month;
			}
		}

		int endTime = 0;
		int fenshi_i = 6;
		//判断分时
		//int len = sizeof(nameArray) / sizeof(char*);
		for (int i = 0; i < 10; ++i)
		{
			//设定k线名字和时间间隔
			strcpy(name, nameArray[i]);
			sprintf_s(myhash, "%s%s_%s", prefix, InstrumentID, name);
			sprintf_s(myhash_v, "%s%s_%s_%s", prefix, InstrumentID, name, "CurrentV");
			time_interval_local = time_interval_local_array[i];


			if (i == 4)
			{
				//60分钟 K
				endTime = compare_time_60(market_Updatetimes, time_interval_local, marketDate);
			}
			else if (i == 5)
			{
				//240分钟 K
				endTime = compare_time_240(marketDate);
			}
			else if (i == 7)
			{
				//日 K
				endTime = r_TempTime;
			}
			else if (i == 8)
			{
				//周 k
				//int st = get_time_week(r_lt, r_ptr);
				////根据日成交量 计算今天之前周成交量
				//week_v = get_sum_v(nowDayOfWeek, InstrumentID, st);

				//以当前行情时间作为key
				endTime = r_TempTime;
			}
			else if (i == 9)
			{
				//月k
				//int st = get_time_month(r_ptr);
				////根据日成交量 计算今天之前月成交量
				//month_v = get_sum_v(nowDayOfMonth, InstrumentID, st);

				//以当前行情时间作为key
				endTime = r_TempTime;
			}
			else
			{
				// 1 5 15 30
				int local = isLastMinutes ? 0 : time_interval_local;
				if (nowTimestamp_Surplus > 55860 && nowTimestamp_Surplus < 56700)
				{
					endTime = nowTimestamp_Zero + 55800;
				}
				else
				{
					endTime = market_Updatetimes + local;
				}
				endTime = endTime - int(endTime) % int(time_interval_local);
			}


			char str_el[20] = "";
			sprintf_s(str_el, "%lf", el);
			//判断
			if (strlen(str_el) != 0)
			{
				reply = (redisReply *)redisCommand(rc, "HKEYS %s", myhash);
				if (reply &&  reply->elements>0) {
					//释放HKEYS 的reply 
					freeReplyObject(reply);

					//获取redis《记录哈希表》中数据 成交量v  用当前行情成交量v 减去记录v 就是这个时间段v的值
					reply = (redisReply *)redisCommand(rc, "GET %s", myhash_v);
					if (reply == NULL)
					{
						printf("redis-获取%s失败", myhash_v);
						//_endthreadex(0);
						LeaveCriticalSection(&g_cs);
						return   0;
					}
					//printf("redis-获取%s\n", reply->str);
					cJSON *root = cJSON_Parse(reply->str);
					double current_maxl;
					double current_minl;
					double current_sl;

					double origin_maxl;
					double origin_minl;
					double origin_sl;
					double origin_el;
					double origin_turnover;


					//判断是否是下个周 月k 间隔不确定 判断
					bool isNext = false;

					int current_v = cJSON_GetObjectItem(root, "v")->valueint;
					int max_time = cJSON_GetObjectItem(root, "dated")->valueint;
					int interval_v = cJSON_GetObjectItem(root, "interval_v")->valueint;
					origin_el = cJSON_GetObjectItem(root, "el")->valuedouble;

					//收盘价为零 就把上一个收盘价赋值给收盘价 
					if (el == 0.0)
					{
						el = origin_el;
					}


					if (i == fenshi_i)
					{
						origin_turnover = cJSON_GetObjectItem(root, "turnover")->valuedouble;
					}
					else
					{
						current_maxl = cJSON_GetObjectItem(root, "maxl")->valuedouble;
						current_minl = cJSON_GetObjectItem(root, "minl")->valuedouble;
						current_sl = cJSON_GetObjectItem(root, "sl")->valuedouble;

						origin_maxl = current_maxl;
						origin_minl = current_minl;
						origin_sl = current_sl;

						current_maxl = current_maxl > el ? current_maxl : el;
						current_minl = current_minl < el ? current_minl : el;
					}

					if (i == 9 && max_time != endTime)
					{
						lt = time_t(max_time);
						ptr = localtime(&lt);
						int max_Month = ptr->tm_mon + 1;

						lt = time_t(endTime);
						ptr = localtime(&lt);
						int end_Month = ptr->tm_mon + 1;

						if (max_Month != end_Month)
						{
							isNext = true;
						}
					}
					freeReplyObject(reply);

					//新插入数据 是否要更新记录数据
					int upDateCurrent = true;

					char value_d[200] = "";
					const char* command1 = NULL;
					const char* command2 = NULL;
					const char* command3 = NULL;
					const char* command4 = NULL;
					const char* command5 = NULL;

					if (endTime - max_time >= time_interval_local || isNext)
					{
						int now_time_v = v - interval_v - current_v;
						//晚8:00 清盘成交量==0；
						if (now_time_v < 0)
						{
							if ((nowTimestamp_Surplus >= 72000 && nowTimestamp_Surplus < 72060))
							{
								now_time_v = 0;
								v = 0;
							}
							else
							{
								now_time_v = v;
							}
						}

						if (i == 7)
						{
							now_time_v = v;
						}
						else if (i == 8)
						{
							now_time_v = v + week_v;
						}
						else if (i == 9)
						{
							now_time_v = v + month_v;
						}

						//printf("<<v=%d<<<<<interval_v=%d<<<<<current_v=%d<<<<<<<<：%d：>>>>>>>>>>>>>>>>\n", v, interval_v, current_v, now_time_v);
						if (i == fenshi_i)
						{
							float el_meanline;
							if (turnover == 0)
							{
								el_meanline = pMarketData->PreClosePrice;
							}
							else
							{
								el_meanline = (float)(turnover / v);
								el_meanline = el_meanline / grammage;
							}
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%lf,\"dated\":%d}", el, now_time_v, el_meanline, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;
							sl = sl == 0 ? el : sl;
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, sl, closeP, now_time_v, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, el, el, now_time_v, endTime);

						}
						/*reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, endTime, value);
						if (NULL == reply)
						{
						cout << "exe command fail" <<value<< endl;
						}
						freeReplyObject(reply);*/
						sprintf_s(value_d, "HMSET %s %d %s", myhash, endTime, value);
						command1 = value_d;
						redisAppendCommand(rc, command1);


						v = isStarMinutes ? interval_v + current_v : v;

						//更新v
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, now_time_v, v, turnover, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;
							sl = sl == 0 ? el : sl;
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, sl, closeP, now_time_v, v, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, el, el, now_time_v, v, endTime);
						}
						/*reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						freeReplyObject(reply);*/

						sprintf_s(value_d, "SET %s %s", myhash_v, value);
						command2 = value_d;
						redisAppendCommand(rc, command2);


						//更新上一个插入值
						endTime = max_time;
						//不更新记录值
						upDateCurrent = false;
					}
					/*else
					{*/
					//时间间隔内 update_v = v - interval_v;

					int  update_v;
					if (i == 7)
					{
						update_v = upDateCurrent ? v : current_v;
					}
					else if (i == 8)
					{
						update_v = upDateCurrent ? v + week_v : current_v;
					}
					else if (i == 9)
					{
						update_v = upDateCurrent ? v + month_v : current_v;
					}
					else
					{
						update_v = v - interval_v;
						if (update_v < 0)
						{
							//换一个交易日 则为上个间隔记录值
							update_v = current_v;
						}
					}

					//printf("小于时间间隔内的手数: %d==总手数%d=v_changeNum=%d--sum_el-%f\n\n", v, update_v, v_changeNum, sum_el);

					//更新k线数据（成交量算手数 更新1
					//有新间隔插入 则除了成交量 其余值全部用原记录值
					if (upDateCurrent)
					{
						if (i == fenshi_i)
						{
							float el_meanline;
							if (turnover == 0)
							{
								el_meanline = pMarketData->PreClosePrice;
							}
							else
							{
								el_meanline = (float)(turnover / v);
								el_meanline = el_meanline / grammage;
							}
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%f,\"dated\":%d}", el, update_v, el_meanline, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;

							//防止最小 开盘数据为零
							minl = minl == 0 ? el : minl;
							sl = sl == 0 ? el : sl;

							if (i == 7)
							{
								sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", maxl, minl, sl, closeP, update_v, endTime);
							}
							else
							{
								sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", current_maxl, current_minl, current_sl, closeP, update_v, endTime);
							}
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", current_maxl, current_minl, current_sl, el, update_v, endTime);
						}
					}
					else
					{
						if (i == fenshi_i)
						{
							float el_meanline;
							if (origin_turnover == 0)
							{
								el_meanline = pMarketData->PreClosePrice;
							}
							else
							{
								el_meanline = (float)(origin_turnover / (current_v + interval_v));
								el_meanline = el_meanline / grammage;
							}
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%f,\"dated\":%d}", origin_el, update_v, el_meanline, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", origin_maxl, origin_minl, origin_sl, origin_el, update_v, endTime);
						}
					}

					/*reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, max_time, value);
					freeReplyObject(reply);*/

					sprintf_s(value_d, "HMSET %s %d %s", myhash, endTime, value);
					command3 = value_d;
					redisAppendCommand(rc, command3);

					//更新记录数据（开盘价，成交量 不更新）相当于k线数据最后一条
					if (upDateCurrent)
					{
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, update_v, interval_v, turnover, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;
							//防止最小 开盘数据为零
							minl = minl == 0 ? el : minl;
							sl = sl == 0 ? el : sl;
							if (i == 7)
							{
								sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", maxl, minl, sl, closeP, update_v, interval_v, endTime);
							}
							else
							{
								sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", current_maxl, current_minl, current_sl, closeP, update_v, interval_v, endTime);
							}
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", current_maxl, current_minl, current_sl, el, update_v, interval_v, endTime);
						}
						/*reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						freeReplyObject(reply);*/

						sprintf_s(value_d, "SET %s %s", myhash_v, value);
						command4 = value_d;
						redisAppendCommand(rc, command4);

						//周 月k最后时间戳 需要显示当日时间戳 遇到下周/月 则跳为下周/月 故要删除前一个日时间戳
						if (i == 8 || i == 9)
						{
							if (endTime != max_time)
							{
								/*reply = (redisReply *)redisCommand(rc, "HDEL %s %d", myhash, max_time);
								freeReplyObject(reply);*/

								sprintf_s(value_d, "HDEL %s %d", myhash, max_time);
								command5 = value_d;
								redisAppendCommand(rc, command5);
							}
						}
					}
					if (command1 != NULL)
					{
						if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
							printf("Failed to execute command[%s] with Pipeline.\n", command1);
							freeReplyObject(reply);
						}
						freeReplyObject(reply);
					}
					if (command2 != NULL)
					{
						if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
							printf("Failed to execute command[%s] with Pipeline.\n", command2);
							freeReplyObject(reply);
						}
						freeReplyObject(reply);
					}
					if (command3 != NULL)
					{
						if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
							printf("Failed to execute command[%s] with Pipeline.\n", command3);
							freeReplyObject(reply);
						}
						freeReplyObject(reply);
					}
					if (command4 != NULL)
					{
						if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
							printf("Failed to execute command[%s] with Pipeline.\n", command4);
							freeReplyObject(reply);
						}
						freeReplyObject(reply);
					}
					if (command5 != NULL)
					{
						if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
							printf("Failed to execute command[%s] with Pipeline.\n", command5);
							freeReplyObject(reply);
						}
						freeReplyObject(reply);
					}

					//}
					free(root);
				}
				else {
					if (reply == NULL)
					{
						//printf("没有 《记录哈希表》 和 《k线哈希表》\n");
						//释放HKEYS 的reply 
						freeReplyObject(reply);

						int tempV = v;
						if (i == 8)
						{
							tempV = tempV + week_v;
						}
						else if (i == 9)
						{
							tempV = tempV + month_v;
						}

						//没有 《k线哈希表》 则创建表 并设置各参数默认值
						//记录值默认 都是LastPrice v默认总成交量 endTime默认行情去秒数时间
						if (i == fenshi_i)
						{
							float el_meanline;
							if (turnover == 0)
							{
								el_meanline = pMarketData->PreClosePrice;
							}
							else
							{
								el_meanline = (float)(turnover / v);
								el_meanline = el_meanline / grammage;
							}
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%lf,\"dated\":%d}", el, tempV, el_meanline, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;
							sl = sl == 0 ? el : sl;
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, sl, closeP, tempV, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, el, el, tempV, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, endTime, value);
						//printf("第一条哈希表插入信息：HMSET: %s\n\n\n", reply->str);
						freeReplyObject(reply);

						//记录值默认 都是LastPrice v 和 interval_v 默认总成交量 endTime默认行情去秒数时间
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, v, v, turnover, endTime);
						}
						else if (i == 7 || i == 8 || i == 9)
						{
							closeP = closeP == 0 ? el : closeP;
							sl = sl == 0 ? el : sl;
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, sl, closeP, v, v, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, el, el, v, v, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						//printf("第一条SET: %s\n", reply->str);
						freeReplyObject(reply);
					}
				}
			}
			else
			{
				printf("品种：==%s==暂无行情数据，不能插入redis\n\n\n", InstrumentID);
			}
		}

		//明细
		char detailName[30] = "fenshi_1_minutes_detail";

		char detail_myhash[50];
		sprintf_s(detail_myhash, "%s%s_%s", prefix, InstrumentID, detailName);

		reply = (redisReply *)redisCommand(rc, "HKEYS %s", detail_myhash);
		//printf("=========HKEYS: %d\n", reply->elements);
		if (reply &&  reply->elements>0) {

			int max_time;
			int min_time;

			//更新
			int *array;
			min_time = atoi(reply->element[0]->str);
			max_time = atoi(reply->element[0]->str);

			int len = reply->elements;
			array = (int*)calloc(len, sizeof(int));
			for (int i = 0; i < len; i++)
			{
				array[i] = atoi(reply->element[i]->str);
				if (array[i] < min_time)
				{
					min_time = array[i];
				}

				if (array[i] > max_time)
				{
					max_time = array[i];
				}

				//printf("i=%d--%d\n", i, array[i]);
			}

			free(array);
			//释放HKEYS 的reply 
			freeReplyObject(reply);

			reply = (redisReply *)redisCommand(rc, "HGET  %s %d", detail_myhash, max_time);
			if (reply == NULL ||  reply->str == NULL)
			{
				//_endthreadex(0);
				LeaveCriticalSection(&g_cs);
				return   0;
			}

			cJSON *root = cJSON_Parse(reply->str);
			int  temInterval_v = cJSON_GetObjectItem(root, "VOL")->valueint;
			int  total_v = cJSON_GetObjectItem(root, "total_VOL")->valueint;

			int fenshiInterval_v = v - total_v;
			//printf("最小time=%d--最大time=%d=dds-%d=实时V=%d==最大V=%d==间隔V=%d--\n", min_time, max_time, dds,v, total_v, fenshiInterval_v);
			freeReplyObject(reply);

			if (fenshiInterval_v != 0 && max_time != dds)
			{
				char value_d[200] = "";
				const char* command1 = NULL;
				const char* command2 = NULL;

				if (len > 19)
				{
					//删除最小值
					//记录值默认 都是LastPrice v默认总成交量 endTime默认行情去秒数时间
					//reply = (redisReply *)redisCommand(rc, "HDEL %s %d", detail_myhash, min_time);
					////printf("删除最小%d---%s\n", min_time, reply->str);
					//freeReplyObject(reply);

					sprintf_s(value_d, "HDEL %s %d", detail_myhash, min_time);
					command1 = value_d;
					redisAppendCommand(rc, command1);
				}

				//插入
				//记录值默认 都是LastPrice v默认总成交量 endTime默认行情去秒数时间
				sprintf_s(value, "{\"time\":%d,\"price\":%lf,\"VOL\":%d,\"total_VOL\":%d}", dds, el, fenshiInterval_v, v);
				//reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", detail_myhash, dds, value);
				////printf("插入实时%d---%s\n", dds, reply->str);
				//freeReplyObject(reply);

				sprintf_s(value_d, "HMSET %s %d %s", detail_myhash, dds, value);
				command2 = value_d;
				redisAppendCommand(rc, command2);


				if (command1 != NULL)
				{
					if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
						printf("Failed to execute command[%s] with Pipeline.\n", command1);
						freeReplyObject(reply);
					}
					freeReplyObject(reply);
				}
				if (command2 != NULL)
				{
					if (REDIS_OK != redisGetReply(rc, (void**)&reply)) {
						printf("Failed to execute command[%s] with Pipeline.\n", command2);
						freeReplyObject(reply);
					}
					freeReplyObject(reply);
				}

			}
		}
		else {
			if (reply == NULL)
			{
				//释放HKEYS 的reply 
				freeReplyObject(reply);

				//记录值默认LastPrice v默认总成交量 endTime默认行情去秒数时间
				sprintf_s(value, "{\"time\":%d,\"price\":%lf,\"VOL\":%d,\"total_VOL\":%d}", dds, el, v, v);
				reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", detail_myhash, dds, value);
				freeReplyObject(reply);
			}
		}

		//_endthreadex(0);

		char str_el[20] = "";
		sprintf_s(str_el, "%lf", pMarketData->LastPrice);

		//判断
		if (strlen(str_el) != 0)
		{
			char value[2000] = "";

			sprintf_s(value, "{\"TradingDay\":\"%s\",\"SettlementGroupID\":\"%s\",\"SettlementID\":%d,\"InstrumentID\":\"%s\",\"UpdateTime\":\"%s\",\"UpdateMillisec\":%d,\"ExchangeID\":\"%s\",\"PreSettlementPrice\":%lf,\"PreClosePrice\":%lf,\"PreOpenInterest\":%lf,\"PreDelta\":%lf,\"OpenPrice\":%lf,\"HighestPrice\":%lf,\"LowestPrice\":%lf,\"ClosePrice\":%lf,\"UpperLimitPrice\":%lf,\"LowerLimitPrice\":%lf,\"SettlementPrice\":%lf,\"CurrDelta\":%lf,\"LastPrice\":%lf,\"Volume\":%d,\"Turnover\":%lf,\"OpenInterest\":%lf,\"BidPrice1\":%lf,\"BidVolume1\":%d,\"AskPrice1\":%lf,\"AskVolume1\":%d,\"BidPrice2\":%lf,\"BidVolume2\":%d,\"AskPrice2\":%lf,\"AskVolume2\":%d,\"BidPrice3\":%lf,\"BidVolume3\":%d,\"AskPrice3\":%lf,\"AskVolume3\":%d,\"BidPrice4\":%lf,\"BidVolume4\":%d,\"AskPrice4\":%lf,\"AskVolume4\":%d,\"BidPrice5\":%lf,\"BidVolume5\":%d,\"AskPrice5\":%lf,\"AskVolume5\":%d}",
				pMarketData->TradingDay,
				pMarketData->SettlementGroupID,
				pMarketData->SettlementID,

				pMarketData->InstrumentID,
				pMarketData->UpdateTime,
				pMarketData->UpdateMillisec,
				pMarketData->ExchangeID,

				//昨
				pMarketData->PreSettlementPrice,
				pMarketData->PreClosePrice,
				pMarketData->PreOpenInterest,
				pMarketData->PreDelta,

				//今
				pMarketData->OpenPrice,
				pMarketData->HighestPrice,
				pMarketData->LowestPrice,
				pMarketData->ClosePrice,

				pMarketData->UpperLimitPrice,
				pMarketData->LowerLimitPrice,
				pMarketData->SettlementPrice,
				pMarketData->CurrDelta,

				//其他
				pMarketData->LastPrice,
				pMarketData->Volume,
				pMarketData->Turnover,
				pMarketData->OpenInterest,

				//申
				//一
				pMarketData->BidPrice1,
				pMarketData->BidVolume1,
				pMarketData->AskPrice1,
				pMarketData->AskVolume1,

				//二
				pMarketData->BidPrice2,
				pMarketData->BidVolume2,
				pMarketData->AskPrice2,
				pMarketData->AskVolume2,

				//三
				pMarketData->BidPrice3,
				pMarketData->BidVolume3,
				pMarketData->AskPrice3,
				pMarketData->AskVolume3,

				//四
				pMarketData->BidPrice4,
				pMarketData->BidVolume4,
				pMarketData->AskPrice4,
				pMarketData->AskVolume4,

				//五
				pMarketData->BidPrice5,
				pMarketData->BidVolume5,
				pMarketData->AskPrice5,
				pMarketData->AskVolume5
			);

			reply = (redisReply *)redisCommand(rc, "HMSET ALL_InstrumentID %s %s", InstrumentID, value);
			//printf("哈希表插入信息：HMSET: %s\n\n", reply->str);
			if (reply == NULL) {

				LOG4CPLUS_FATAL(myLoger->logger, "HMSET ALL_InstrumentID Failed to execute command");
			}
			freeReplyObject(reply);
		}
		else
		{
			printf("品种：==%s==暂无行情数据，不能插入redis\n\n\n", InstrumentID);
		}
	}
	catch (exception &e) {  //exception类位于<exception>头文件中
		cout << "k线 异常\n\n\n" << endl;
	}


	LeaveCriticalSection(&g_cs);
	return   0;
}

class CSimpleHandler : public CQdpFtdcMduserSpi
{
public:
	// 构造函数，需要一个有效的指向CQdpFtdcMduserApi实例的指针
	CSimpleHandler(CQdpFtdcMduserApi *pUserApi) : m_pUserApi(pUserApi) {}

	~CSimpleHandler() {}

	// 当客户端与行情发布服务器建立起通信连接，客户端需要进行登录
	void OnFrontConnected()
	{
		CQdpFtdcReqUserLoginField reqUserLogin;
		strcpy(reqUserLogin.TradingDay, m_pUserApi->GetTradingDay());
		strcpy(reqUserLogin.BrokerID, "jy");
		strcpy(reqUserLogin.UserID, "88760");
		strcpy(reqUserLogin.Password, "88760");
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}

	// 当客户端与行情发布服务器通信连接断开时，该方法被调用
	void OnFrontDisconnected()
	{
		// 当发生这个情况后，API会自动重新连接，客户端可不做处理
		printf("OnFrontDisconnected.\n");

	}

	// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
	void OnRspUserLogin(CQdpFtdcRspUserLoginField *pRspUserLogin, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUserLogin:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);

		try {
			//登录日志
			char nowtDate[30] = "";
			struct tm *ptr;
			time_t lt = time(NULL);
			ptr = localtime(&lt);
			strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d %H:%M:%S ", ptr);
			int now = (int)time(NULL);
			char value[800] = "";
			sprintf_s(value, "{\"%sdate\":%s,\"ErrorCode\":%d,\"ErrorMsg\":%s,\"RequestID\":%d,\"Chain\":%d}", prefix, nowtDate, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			//char name[30];
			//sprintf_s(name, "%sk_tcp_login_Log", prefix);
			///*reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", name, now, value);
			//freeReplyObject(reply);*/

			sprintf_s(value, "当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功{\ntime:%d\"date\":%s,\"ErrorCode\":%d,\"ErrorMsg\":%s,\"RequestID\":%d,\"Chain\":%d}", now, nowtDate, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);

			LOG4CPLUS_WARN(myLoger->logger, value);

			if (pRspInfo->ErrorID != 0)
			{
				// 端登失败，客户端需进行错误处理
				printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
				return;
			}
		}
		catch (exception &e) {  //exception类位于<exception>头文件中
			cout << "登录日志异常-\n\n\n" << endl;
		}
		
		/*char * contracts[19] = { "","","" ,"" ,"","","","","","","","","","","","","","","" };
		contracts[0] = "g(T+D)_deil";
		contracts[1] = "Au(T+D)";
		contracts[2] = "Ag(T+D)";
		contracts[3] = "u(T+D)_deil";
		contracts[4] = "au(T+N1)";
		contracts[5] = "(T+N1)_Deli";
		contracts[6] = "au(T+N2)";
		contracts[7] = "(T+N2)_Deli";
		contracts[8] = "Au100g";
		contracts[9] = "Au50g";
		contracts[10] = "Au99.5";
		contracts[11] = "Au99.95";
		contracts[12] = "Au99.99";
		contracts[13] = "Pt99.95";
		contracts[14] = "iAu100g";
		contracts[15] = "iAu99.5";
		contracts[16] = "iAu99.99";
		contracts[17] = "mAu(T+D)";
		contracts[18] = "u(T+D)_Deli";
		m_pUserApi->SubMarketData(contracts, 19);*/

		char * contracts[4] = { "","","" ,"" };
		//contracts[0] = "g(T+D)_deil";
		contracts[0] = "Au(T+D)";
		contracts[1] = "Ag(T+D)";
		//contracts[3] = "u(T+D)_deil";
		//contracts[4] = "au(T+N1)";
		//contracts[5] = "(T+N1)_Deli";
		//contracts[6] = "au(T+N2)";
		//contracts[7] = "(T+N2)_Deli";
		contracts[2] = "Au100g";
		//contracts[9] = "Au50g";
		//contracts[10] = "Au99.5";
		//contracts[11] = "Au99.95";
		//contracts[12] = "Au99.99";
		//contracts[13] = "Pt99.95";
		//contracts[14] = "iAu100g";
		//contracts[15] = "iAu99.5";
		//contracts[16] = "iAu99.99";
		contracts[3] = "mAu(T+D)";
		//contracts[18] = "u(T+D)_Deli";
		m_pUserApi->SubMarketData(contracts, 4);
	}


	// 深度行情通知，行情服务器会主动通知客户端
	void OnRtnDepthMarketData(CQdpFtdcDepthMarketDataField *pMarketData)
	{
		HANDLE handle[1];
		InitializeCriticalSection(&g_cs);   //  
		handle[0] = (HANDLE)_beginthreadex(NULL, 0, &ThreadFunc, pMarketData, 0, NULL);
		WaitForMultipleObjects(1, handle, TRUE, INFINITE);
		CloseHandle(handle[0]);
		DeleteCriticalSection(&g_cs);
	}

	// 针对用户请求的出错通知
	void OnRspError(CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspError:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		// 客户端需进行错误处理
		char value[800] = "";
		sprintf_s(value, "针对用户请求的出错通知 ErrorCode = [%d], ErrorMsg = [%s]\nRequestID=[%d], Chain=[%d]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
		LOG4CPLUS_ERROR(myLoger->logger, value);

		// 释放rc资源
		redisFree(rc);
		freeReplyObject(reply);

		ConnrectionRedis();
	}

	///订阅合约的相关信息
	void OnRspSubMarketData(CQdpFtdcSpecificInstrumentField *pSpecificInstrument, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("Sub 返回订阅合约：%s \n", pSpecificInstrument->InstrumentID);
	}

	///订阅合约的相关信息
	void OnRspUnSubMarketData(CQdpFtdcSpecificInstrumentField *pSpecificInstrument, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("UnSub 返回订阅合约：%s \n", pSpecificInstrument->InstrumentID);
	}

private:
	// 指向CQdpFtdcMduserApi实例的指针
	CQdpFtdcMduserApi *m_pUserApi;
};



bool RunOnce()
{
	hMutex = CreateMutex(NULL, TRUE, "tickets_q");  //创建互斥
	if (NULL == hMutex)
	{
		return false;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		OutputDebugString("互斥返回");
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
		return false;
	}
	return true;
}

int main()
{

	//创建互斥对象
	//if(!RunOnce())
	//{
	//	exit(0);
	//	//return 0;
	//}
	//log 日志
	myLoger = NULL;
	myLoger = MyLogger::getInstance();

	//market
	std::string c_marketAddress;

	//oracle
	std::string c_oracle_DomainName;
	int c_oracle_Port;
	std::string c_oracle_UserID;
	std::string c_oracle_Password;

	//redis
	std::string c_redis_DomainName;
	int c_redis_Port;

	std::string c_holidays;

	const char ConfigFile[] = "config.txt";
	Config configSettings(ConfigFile);

	c_marketAddress = configSettings.Read("marketAddress", c_marketAddress);

	c_oracle_DomainName = configSettings.Read("oracle_DomainName", c_oracle_DomainName);
	c_oracle_Port = configSettings.Read("oracle_Port", 0);
	c_oracle_UserID = configSettings.Read("oracle_UserID", c_oracle_UserID);
	c_oracle_Password = configSettings.Read("oracle_Password", c_oracle_Password);

	c_redis_DomainName = configSettings.Read("redis_DomainName", c_redis_DomainName);
	c_redis_Port = configSettings.Read("redis_Port", 0);

	c_holidays = configSettings.Read("holidays", c_holidays);

	std::cout << "marketAddress:" << c_marketAddress << "\n" << std::endl;
	std::cout << "oracle_DomainName:" << c_oracle_DomainName << std::endl;
	std::cout << "oracle_Port:" << c_oracle_Port << std::endl;
	std::cout << "oracle_UserID:" << c_oracle_UserID << std::endl;
	std::cout << "oracle_Password:" << c_oracle_Password << "\n" << std::endl;
	std::cout << "redis_DomainName:" << c_redis_DomainName << std::endl;
	std::cout << "redis_Port:" << c_redis_Port << "\n" << std::endl;


	strcpy(marketAddress, c_marketAddress.c_str());

	strcpy(oracle_DomainName, c_oracle_DomainName.c_str());
	oracle_Port = c_oracle_Port;
	strcpy(oracle_UserID, c_oracle_UserID.c_str());
	strcpy(oracle_Password, c_oracle_Password.c_str());

	strcpy(redis_DomainName, c_redis_DomainName.c_str());
	redis_Port = c_redis_Port;

	char holidays[50];
	strcpy(holidays, c_holidays.c_str());
	char seg[] = ","; /*分隔符这里为逗号comma，分隔符可以为你指定的，如分号，空格等*/
	holidaysNumber = 0;
	char *substr = strtok(holidays, seg);/*利用现成的分割函数,substr为分割出来的子字符串*/
	while (substr != NULL) {
		strcpy(charlist[holidaysNumber], substr);/*把新分割出来的子字符串substr拷贝到要存储的charlsit中*/
		holidaysNumber++;
		substr = strtok(NULL, seg);/*在第一次调用时，strtok()必需给予参数str字符串，
								   往后的调用则将参数str设置成NULL。每次调用成功则返回被分割出片段的指针。*/
	}

	//连接redis
	//ConnrectionRedis();

	// 产生一个CQdpFtdcMduserApi实例
	CQdpFtdcMduserApi *pUserApi = CQdpFtdcMduserApi::CreateFtdcMduserApi();
	// 产生一个事件处理的实例
	CSimpleHandler sh(pUserApi);
	// 注册一事件处理的实例
	pUserApi->RegisterSpi(&sh);
	// 注册需要的深度行情主题
	///        TERT_RESTART:从本交易日开始重传
	///        TERT_RESUME:从上次收到的续传
	///        TERT_QUICK:先传送当前行情快照,再传送登录后市场行情的内容	//pUserApi-> SubscribeMarketDataTopic (101, TERT_RESUME);
	//pUserApi-> SubscribeMarketDataTopic (110, QDP_TERT_RESTART);
	// 设置行情发布服务器的地址
	pUserApi->RegisterFront(marketAddress);
	//pUserApi->RegisterFront("tcp://hqsource.onehgold.com:30007");

	// 使客户端开始与行情发布服务器建立连接
	pUserApi->Init();
	// 释放useapi实例
	pUserApi->Release();

	_CrtDumpMemoryLeaks();

	return 0;
}


