#pragma once
// mddemo.cpp : 
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


#include <hiredis.h>
#include <ocilib.h>
#include "cJSON.h"

#define NO_QFORKIMPL //这一行必须加才能正常使用


#pragma comment(lib,"hiredis.lib")

#pragma comment(lib,"ociliba.lib")
#pragma comment(lib,"ocilibm.lib")
#pragma comment(lib,"ocilibw.lib")

using namespace std;

//k线间隔

HANDLE hMutex;

int specialTime;
int endTime;

CDBOperation dbOper;

redisContext *rc;
redisReply *reply;

char   dateTime[100];

//连接redis
void ConnrectionRedis()
{
	char ip[] = "redisd.onehgold.com";
	int port = 6379;
	// 连接Redis
	rc = redisConnect(ip, port);
	if (rc == NULL || rc->err) {
		if (rc) {
			printf("Connection error: %s\n", rc->errstr);
			redisFree(rc);
		}
		else {
			printf("Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
	//freeReplyObject(reply);

	// 释放rc资源
	//redisFree(rc);
}

//连接Oracle
void ConnrectionOracle()
{
	//bool bConn = dbOper.ConnToDB("Provider = OraOLEDB.Oracle.1; User ID = test; Password = Abcd1234; Data Source = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = 222.73.85.6)(PORT = 15212))(CONNECT_DATA = (SERVICE_NAME = ORCL))); Persist Security Info = False", "HQ", "Abcd1234");

	bool bConn = dbOper.ConnToDB("Provider = OraOLEDB.Oracle.1; User ID = test; Password = Abcd1234; Data Source = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = dbd1.onehgold.com)(PORT = 1521))(CONNECT_DATA = (SERVICE_NAME = ORCL))); Persist Security Info = False", "HQ", "Abcd1234");
	if (false == bConn)
	{
		printf("连接数据库出现错误\n");
		system("PAUSE");
		return;
	}
	printf("连接数据库成功!\n");
}

void err_handler(OCI_Error *err)
{
	printf("Error ORA-%05i - msg : %s\n", OCI_ErrorGetOCICode(err), OCI_ErrorGetString(err));
}

typedef struct times
{
	int Year;
	int Mon;
	int Day;
	int Hour;
	int Min;
	int Second;
}Times;

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

void stamp_to_standard(int stampTime, char a[100], char week[100])
{
	//printf("a=%s=\n ", a);

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

bool compare_time_60(int time, char week[30])
{

	int	b = (int(time) + 28800) % int(86400);

	if (b >= 7200 && b <= 9000)
	{
		if (strcmp(week, "Saturday") == 0)
		{
			specialTime = (8 + 48) * 60 * 60;
		}
		else
		{
			specialTime = 8 * 60 * 60;
		}
		//特殊情况2:30分
		if (b == 9000)
		{
			specialTime = specialTime - 60 * 60;
		}
	}
	else
	{
		specialTime = 60 * 60;
	}

	//判断时间戳是否是半小时区间 最后结果减去半小时
	if (b >= 7200 && b <= 55800)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int compare_time_240(int time, char week[30])
{
	int endTime_240;
	//00:00 10:30 15:30 00:00
	int a = time - (int(time) + 28800) % int(86400);
	int	b = (int(time) + 28800) % int(86400);

	if (b >= 0 && b < 37800)
	{
		if (strcmp(week, "Saturday") == 0)
		{
			endTime_240 = a + 48 * 60 * 60 + 37800;
		}
		else
		{
			endTime_240 = a + 37800;
		}
	}
	else if (b >= 37800 && b < 55800)
	{
		endTime_240 = a + 55800;
	}
	else
	{
		endTime_240 = a + 86400;
	}
	return endTime_240;
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

void del_char(char str[], char d[])
{
	int i = 0, j = 0;
	while (str[i] != '\0')
	{

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

char * replaceAll(char * src, char oldChar, char newChar) {
	char * head = src;
	while (*src != '\0') {
		if (*src == oldChar) 
			*src = newChar;
		src++;
	}
	return head;
}

//删除特殊字符  || 替换其他特殊字符
void delAndReplace(char str[], char d[], char oldChar, char newChar)
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

		if (pRspInfo->ErrorID != 0)
		{
			// 端登失败，客户端需进行错误处理
			printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			return;
		}
		char * contracts[19] = { "","","" ,"" ,"","","","","","","","","","","","","","","" };
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
		m_pUserApi->SubMarketData(contracts, 19);
	}

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


		/*
		if (pMarketData->AskPrice1 == DBL_MAX)
		printf("%s,", "");
		else
		printf("%f,", pMarketData->AskPrice1);

		if (pMarketData->BidPrice1 == DBL_MAX)
		printf("%s \n", "");
		else
		printf("%f \n", pMarketData->BidPrice1);
		*/
	}

	// 深度行情通知，行情服务器会主动通知客户端
	void OnRtnDepthMarketData(CQdpFtdcDepthMarketDataField *pMarketData)
	{

		//输出行情
		printf_Market(pMarketData);
		
		_RecordsetPtr pRst;
		char sql[1500] = { 0 };

		//行情更新时间为  本地日期年月日 拼接行情时分秒
		char nowtDate[100] = "";
		char nowtTime[100] = "";
		char market_time_str[100] = "";
		char time_hms[30] = "";

		struct tm *ptr;
		time_t lt;
		lt = time(NULL);
		ptr = localtime(&lt);
		strftime(nowtTime, sizeof(nowtTime), "%Y%m%d%H%M%S", localtime(&lt));

		//fix 1:年月日用本地年月日拼接行情时分秒 因为行情时间晚八点之后日期会变为下一天
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d", ptr);
		strcpy(market_time_str, nowtDate);
		strcpy(time_hms, pMarketData->UpdateTime);
		strcat(market_time_str, " ");
		strcat(market_time_str, time_hms);
		int market_Updatetimes = (int)StringToDatetime(market_time_str);

		//合约标示去除多余字符 并转成大写字符
		char origin_InstrumentID[31] = "";
		strcpy(origin_InstrumentID, pMarketData->InstrumentID);
		char InstrumentID[30] = "";
		char d[5] = { '(', ')', '+' };
		delAndReplace(origin_InstrumentID, d, '.', '_');
		strcpy(InstrumentID, origin_InstrumentID);

		printf("行情%s更新拼接时间：%s=时间戳===%d==\n", InstrumentID, time_hms, market_Updatetimes);


		//oracle时间戳类型 格式要遵循 yyyy - mm - dd hh : 24mi : ss.ff

		//执行更新 表 语句
		sprintf_s(sql, "UPDATE QUOTATION SET TRADINGDAY  = '%s',SETTLEMENTGROUPID = '%s',SETTLEMENTID = %d,UPDATETIME = '%s',UPDATEMILLISEC = %d,EXCHANGEID = '%s',PRESETTLEMENTPRICE = %f,PRECLOSEPRICE = %f,PREOPENINTEREST = %f,PREDELTA = %f,OPENPRICE = %f,HIGHESTPRICE = %f,LOWESTPRICE = %f,CLOSEPRICE = %f, UPPERLIMITPRICE = %f,LOWERLIMITPRICE = %f,SETTLEMENTPRICE = %f,CURRDELTA = %f,LASTPRICE = %f,VOLUME = %d,TURNOVER = %f,OPENINTEREST = %f,BIDPRICE1 = %f,BIDVOLUME1 = %d,ASKPRICE1 = %f,ASKVOLUME1 = %d,BIDPRICE2 = %f,BIDVOLUME2 = %d,ASKPRICE2 = %f,ASKVOLUME2 = %d,BIDPRICE3 = %f,BIDVOLUME3 = %d,ASKPRICE3 = %f,ASKVOLUME3 = %d,BIDPRICE4 = %f,BIDVOLUME4 = %d,ASKPRICE4 = %f,ASKVOLUME4 = %d,BIDPRICE5 = %f,BIDVOLUME5 = %d,ASKPRICE5 = %f,ASKVOLUME5 = %d,DATET = to_date('%s','yyyymmddhh24miss'),DATED = %d WHERE INSTRUMENTID = '%s'",
			pMarketData->TradingDay,
			pMarketData->SettlementGroupID,
			pMarketData->SettlementID,

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
			pMarketData->AskVolume5,
			nowtTime,
			market_Updatetimes,

			pMarketData->InstrumentID
		);
		pRst = dbOper.ExecuteWithResSQL(sql);
		if (NULL != pRst)
		{
			printf("更新行情种类-%s---成功\n", pMarketData->InstrumentID);
		}
		else
		{
			printf("更新行情种类---失败\n");
		}

		//执行插入历史表语句  
		sprintf_s(sql, "INSERT INTO %s (TRADINGDAY,SETTLEMENTGROUPID,SETTLEMENTID,INSTRUMENTID,UPDATETIME,UPDATEMILLISEC,EXCHANGEID,PRESETTLEMENTPRICE,PRECLOSEPRICE,PREOPENINTEREST,PREDELTA,OPENPRICE,HIGHESTPRICE,LOWESTPRICE,CLOSEPRICE,UPPERLIMITPRICE,LOWERLIMITPRICE,SETTLEMENTPRICE,CURRDELTA,LASTPRICE,VOLUME,TURNOVER,OPENINTEREST,BIDPRICE1,BIDVOLUME1,ASKPRICE1,ASKVOLUME1,BIDPRICE2,BIDVOLUME2,ASKPRICE2,ASKVOLUME2,BIDPRICE3,BIDVOLUME3,ASKPRICE3,ASKVOLUME3,BIDPRICE4,BIDVOLUME4,ASKPRICE4,ASKVOLUME4,BIDPRICE5,BIDVOLUME5,ASKPRICE5,ASKVOLUME5,DATET,DATED) VALUES ('%s','%s',%d,'%s','%s',%d,'%s',%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,to_date('%s','yyyymmddhh24miss'),%d)",
			InstrumentID,
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
			pMarketData->AskVolume5,
			nowtTime,
			market_Updatetimes
		);
		pRst = dbOper.ExecuteWithResSQL(sql);
		if (NULL != pRst)
		{
			printf("插入历史数据--%s-成功\n", pMarketData->InstrumentID);
		}
		else
		{
			printf("插入历史数据--失败\n");
		}

		char str_el[20] = "";
		sprintf_s(str_el, "%lf", pMarketData->LastPrice);

		//判断
		if (strlen(str_el) != 0)
		{
			char value[2000] = "";

			sprintf(value, "{\"TradingDay\":\"%s\",\"SettlementGroupID\":\"%s\",\"SettlementID\":%d,\"InstrumentID\":\"%s\",\"UpdateTime\":\"%s\",\"UpdateMillisec\":%d,\"ExchangeID\":\"%s\",\"PreSettlementPrice\":%lf,\"PreClosePrice\":%lf,\"PreOpenInterest\":%lf,\"PreDelta\":%lf,\"OpenPrice\":%lf,\"HighestPrice\":%lf,\"LowestPrice\":%lf,\"ClosePrice\":%lf,\"UpperLimitPrice\":%lf,\"LowerLimitPrice\":%lf,\"SettlementPrice\":%lf,\"CurrDelta\":%lf,\"LastPrice\":%lf,\"Volume\":%d,\"Turnover\":%lf,\"OpenInterest\":%lf,\"BidPrice1\":%lf,\"BidVolume1\":%d,\"AskPrice1\":%lf,\"AskVolume1\":%d,\"BidPrice2\":%lf,\"BidVolume2\":%d,\"AskPrice2\":%lf,\"AskVolume2\":%d,\"BidPrice3\":%lf,\"BidVolume3\":%d,\"AskPrice3\":%lf,\"AskVolume3\":%d,\"BidPrice4\":%lf,\"BidVolume4\":%d,\"AskPrice4\":%lf,\"AskVolume4\":%d,\"BidPrice5\":%lf,\"BidVolume5\":%d,\"AskPrice5\":%lf,\"AskVolume5\":%d}",
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
			printf("哈希表插入信息：HMSET: %s\n\n", reply->str);
			freeReplyObject(reply);
		}
		else
		{
			printf("品种：==%s==暂无行情数据，不能插入redis\n\n\n", InstrumentID);
		}	

	}

	// 针对用户请求的出错通知
	void OnRspError(CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspError:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		// 客户端需进行错误处理


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

	//int tm_mday; /* 一个月中的日期 - 取值区间为[1,31] */
	//int tm_mon; /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
	//int tm_year; /* 年份，其值等于实际年份减去1900 */
	//int tm_wday; /* 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 atoi*/

	/*
	struct tm *ptr;
	time_t lt;
	lt = time(NULL);
	//lt = time_t(1487959830);
	ptr = localtime(&lt);
	

	printf("second:%d\n", ptr->tm_sec);
	printf("minute:%d\n", ptr->tm_min);
	printf("hour:%d\n", ptr->tm_hour);
	printf("wday:%d\n", ptr->tm_wday);
	printf("mday:%d\n", ptr->tm_mday);
	printf("month:%d\n", ptr->tm_mon + 1);
	printf("year:%d\n", ptr->tm_year + 1900);

	int wday = ptr->tm_wday;

	int nowDayOfWeek = (wday == 0) ? 7 : wday - 1; //今天是本周的第几天。周一=0，周日=6

	int nowTime = (int)lt;


	/*time_t now;
	int nowTime = (int)time(&now);
	nowTime = 1489687830;*/
	
	/*
	//周一
	int MomdayTime = nowTime - nowDayOfWeek * 24 * 60 * 60;

	//周五 
	int SaturdayTime = MomdayTime + 4 * 24 * 60 * 60;

	char week[20] = "";
	strftime(week, sizeof(week), "%A", ptr);
	if (strcmp(week, "Saturday") == 0)
	{
		SaturdayTime = SaturdayTime + 7 * 24 * 60 * 60;
	}

	//strftime(s, sizeof(s), "%H:%M", &tm);


	char jointDate[50] = "";
	//月初
	sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1, 1, 0, 0, 0);
	int startMon = StringToDatetime(jointDate);

	//月末
	sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1 + 1, 1, 0, 0, 0);
	int endMon = StringToDatetime(jointDate);
	endMon = endMon - 24 * 60 * 60;


	int end_Month = ptr->tm_mon + 1;

	lt = time_t(1488211200);
	ptr = localtime(&lt);
	int max_Month = ptr->tm_mon + 1;

	if (max_Month != end_Month)
	{
		printf("---end_time_month=%d--max_time_month=%d--\n", end_Month, max_Month);
	}

	char nowtTime[100] = "2014-02-14 20:47:00";
	int a =  StringToDatetime(nowtTime);
	*/


	ConnrectionOracle();

	ConnrectionRedis();

	
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
	pUserApi->RegisterFront("tcp://101.226.241.234:30007");
	//pUserApi->RegisterFront("tcp://hqsource.onehgold.com:30007");

	// 使客户端开始与行情发布服务器建立连接
	pUserApi->Init();
	// 释放useapi实例
	pUserApi->Release();

	_CrtDumpMemoryLeaks();


	return 0;
}


