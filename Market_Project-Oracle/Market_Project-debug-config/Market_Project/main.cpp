#pragma once
// main.cpp : 
//һ���򵥵����ӣ�����QdpFtdcMduserApi��QdpFtdcMduserSpi�ӿڵ�ʹ�á�

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

#include "Config.h"

#define NO_QFORKIMPL //��һ�б���Ӳ�������ʹ��

#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"ociliba.lib")
#pragma comment(lib,"ocilibm.lib")
#pragma comment(lib,"ocilibw.lib")
using namespace std;

//#define environment 1  //d ��������
//#define environment 2  //t ���Ի���
//#define environment 3  //  ��ʽ����

//#define environment 3
//#define MarketIP "tcp://101.226.241.234:30007"

//#if environment == 1
//#define redisDomainName  "redisd.onehgold.com"
//#define oracleDomainName "dbd1.onehgold.com"
//
//#elif environment == 2
//#define redisDomainName  "redist.onehgold.com"
//#define oracleDomainName "dbt1.onehgold.com"
//
//#else
//#define redisDomainName  "redis.onehgold.com"
//#define oracleDomainName "db1.onehgold.com"
//
//#endif 


//k�߼��

HANDLE hMutex;

int specialTime;
int endTime;

CDBOperation dbOper;

redisContext *rc;
redisReply *reply;

char   dateTime[100];

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

//����redis
void ConnrectionRedis()
{
	// ����Redis
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
		printf("����redis�ɹ�!\n");
	}
	//freeReplyObject(reply);

	// �ͷ�rc��Դ
	//redisFree(rc);
}

//����Oracle
void ConnrectionOracle()
{
	char str[500];
	sprintf_s(str, "Provider = OraOLEDB.Oracle.1; User ID = %s; Password = %s; Data Source = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = %s)(PORT = %d))(CONNECT_DATA = (SERVICE_NAME = ORCL))); Persist Security Info = False", oracle_UserID,oracle_Password, oracle_DomainName,oracle_Port);
	bool bConn = dbOper.ConnToDB(str, oracle_UserID, oracle_Password);
	if (false == bConn)
	{
		printf("�������ݿ���ִ���\n");
		//system("PAUSE");
		//return;
	}
	else
	{
		printf("����oracle�ɹ�!\n");

	}
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

//�ַ���ת��Ϊʱ���
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

	time_t t_ = mktime(&tm_); //�Ѿ�����8��ʱ��  
	return t_; //��ʱ��  
}

//ʱ���ת��Ϊ�ַ���
void DatetimeToString(time_t time)
{
	time_t t = time + 28800;//8h ʱ����
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

	//printf("ʱ���%d: ����ʱ�֣�%s��%s\n", (int)tick, s, w);
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
		//�������2:30��
		if (b == 9000)
		{
			specialTime = specialTime - 60 * 60;
		}
	}
	else
	{
		specialTime = 60 * 60;
	}

	//�ж�ʱ����Ƿ��ǰ�Сʱ���� �������ȥ��Сʱ
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
	//����һ���ַ�ָ�룬ָ�򴫵ݽ�����ch��ַ��  
	char* subch = (char*)calloc(sizeof(char), length + 1);
	//ͨ��calloc������һ��length���ȵ��ַ����飬���ص����ַ�ָ�롣  
	int i;
	//ֻ����C99��forѭ���вſ�����������������д�����棬��߼����ԡ�  
	pch = pch + pos;
	//��pchָ��ָ��posλ�á�  
	for (i = 0; i<length; i++)
	{
		subch[i] = *(pch++);
		//ѭ��������ֵ���顣  
	}
	subch[length] = '\0';//�����ַ�����������  
	return subch;       //���ط�����ַ������ַ��  
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

//ɾ�������ַ�  || �滻���������ַ�
void delAndReplace(char str[], char d[], char oldChar, char newChar)
{
	int i = 0, j = 0;
	while (str[i] != '\0')
	{
		//��ĸСд���д
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
	// ���캯������Ҫһ����Ч��ָ��CQdpFtdcMduserApiʵ����ָ��
	CSimpleHandler(CQdpFtdcMduserApi *pUserApi) : m_pUserApi(pUserApi) {}

	~CSimpleHandler() {}

	// ���ͻ��������鷢��������������ͨ�����ӣ��ͻ�����Ҫ���е�¼
	void OnFrontConnected()
	{
		CQdpFtdcReqUserLoginField reqUserLogin;
		strcpy(reqUserLogin.TradingDay, m_pUserApi->GetTradingDay());
		strcpy(reqUserLogin.BrokerID, "jy");
		strcpy(reqUserLogin.UserID, "88760");
		strcpy(reqUserLogin.Password, "88760");
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}


	// ���ͻ��������鷢��������ͨ�����ӶϿ�ʱ���÷���������
	void OnFrontDisconnected()
	{
		// ��������������API���Զ��������ӣ��ͻ��˿ɲ�������
		printf("OnFrontDisconnected.\n");
	}

	// ���ͻ��˷�����¼����֮�󣬸÷����ᱻ���ã�֪ͨ�ͻ��˵�¼�Ƿ�ɹ�
	void OnRspUserLogin(CQdpFtdcRspUserLoginField *pRspUserLogin, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUserLogin:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);

		//��¼��־
		char nowtDate[100] = "";
		struct tm *ptr;
		time_t lt = time(NULL);
		ptr = localtime(&lt);
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d %H:%M:%S ", ptr);
		int now = time(NULL);
		char value[200] = "";
		sprintf(value, "{\"date\":%s,\"ErrorCode\":%d,\"ErrorMsg\":%s,\"RequestID\":%d,\"Chain\":%d}", nowtDate, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
		reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", "hq_tcp_login_Log", now, value);
		freeReplyObject(reply);

		if (pRspInfo->ErrorID != 0)
		{
			// �˵�ʧ�ܣ��ͻ�������д�����
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
		// �ͻ��˰��账���ص�����
		printf(
			"\n<<<<<< \n������:%s,  ���������:%s,  ������:%d,\
            \n��Լ����:%s,  ����޸�ʱ��:%s,  ����޸ĺ���:%d,  ����������:%s��\
            \n�����:%f,  ������:%f,  ��ֲ���:%f,  ����ʵ��:%f,\
            \n����:%f,  ��߼�:%f,  ��ͼ�:%f,  ������:%f,\
            \n��ͣ���:%f,  ��ͣ���:%f,  �����:%f,  ����ʵ��:%f,\
            \n���¼�:%f,  ����:%d,  �ɽ����:%f,  �ֲ���:%f,\
            \n�����һ:%f,  ������һ:%d,  ������һ:%f,  ������һ:%d,\
            \n����۶�:%f,  ��������:%d,  �����۶�:%f,  ��������:%d,\
            \n�������:%f,  ��������:%d,  ��������:%f,  ��������:%d,\
            \n�������:%f,  ��������:%d,  ��������:%f,  ��������:%d,\
            \n�������:%f,  ��������:%d,  ��������:%f,  ��������:%d,\n>>>>>\
            \n\n",

			//����
			pMarketData->TradingDay,
			pMarketData->SettlementGroupID,
			pMarketData->SettlementID,

			pMarketData->InstrumentID,
			pMarketData->UpdateTime,
			pMarketData->UpdateMillisec,
			pMarketData->ExchangeID,

			//��
			pMarketData->PreSettlementPrice,
			pMarketData->PreClosePrice,
			pMarketData->PreOpenInterest,
			pMarketData->PreDelta,

			//��
			pMarketData->OpenPrice,
			pMarketData->HighestPrice,
			pMarketData->LowestPrice,
			pMarketData->ClosePrice,

			pMarketData->UpperLimitPrice,
			pMarketData->LowerLimitPrice,
			pMarketData->SettlementPrice,
			pMarketData->CurrDelta,

			//����
			pMarketData->LastPrice,
			pMarketData->Volume,
			pMarketData->Turnover,
			pMarketData->OpenInterest,

			//��һ
			pMarketData->BidPrice1,
			pMarketData->BidVolume1,
			pMarketData->AskPrice1,
			pMarketData->AskVolume1,

			//��
			pMarketData->BidPrice2,
			pMarketData->BidVolume2,
			pMarketData->AskPrice2,
			pMarketData->AskVolume2,

			//��
			pMarketData->BidPrice3,
			pMarketData->BidVolume3,
			pMarketData->AskPrice3,
			pMarketData->AskVolume3,

			//��
			pMarketData->BidPrice4,
			pMarketData->BidVolume4,
			pMarketData->AskPrice4,
			pMarketData->AskVolume4,

			//��
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

	// �������֪ͨ�����������������֪ͨ�ͻ���
	void OnRtnDepthMarketData(CQdpFtdcDepthMarketDataField *pMarketData)
	{

		//�������
		//printf_Market(pMarketData);
		
		_RecordsetPtr pRst;
		char sql[1500] = { 0 };

		//�������ʱ��Ϊ  �������������� ƴ������ʱ����
		char nowtDate[100] = "";
		char nowtTime[100] = "";
		char market_time_str[100] = "";
		char time_hms[30] = "";

		struct tm *ptr;
		time_t lt;
		lt = time(NULL);
		ptr = localtime(&lt);
		strftime(nowtTime, sizeof(nowtTime), "%Y%m%d%H%M%S", localtime(&lt));

		//fix 1:�������ñ���������ƴ������ʱ���� ��Ϊ����ʱ����˵�֮�����ڻ��Ϊ��һ��
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d", ptr);
		strcpy(market_time_str, nowtDate);
		strcpy(time_hms, pMarketData->UpdateTime);
		strcat(market_time_str, " ");
		strcat(market_time_str, time_hms);
		int market_Updatetimes = (int)StringToDatetime(market_time_str);

		//��Լ��ʾȥ�������ַ� ��ת�ɴ�д�ַ�
		char origin_InstrumentID[31] = "";
		strcpy(origin_InstrumentID, pMarketData->InstrumentID);
		char InstrumentID[30] = "";
		char d[5] = { '(', ')', '+' };
		delAndReplace(origin_InstrumentID, d, '.', '_');
		strcpy(InstrumentID, origin_InstrumentID);

		printf("oracle����%sƴ��ʱ�䣺%s--ʱ�����%d=\n", InstrumentID, time_hms, market_Updatetimes);

		//oracleʱ������� ��ʽҪ��ѭ yyyy - mm - dd hh : 24mi : ss.ff

		//ִ�и��� �� ���
		//sprintf_s(sql, "UPDATE QUOTATION SET TRADINGDAY  = '%s',SETTLEMENTGROUPID = '%s',SETTLEMENTID = %d,UPDATETIME = '%s',UPDATEMILLISEC = %d,EXCHANGEID = '%s',PRESETTLEMENTPRICE = %f,PRECLOSEPRICE = %f,PREOPENINTEREST = %f,PREDELTA = %f,OPENPRICE = %f,HIGHESTPRICE = %f,LOWESTPRICE = %f,CLOSEPRICE = %f, UPPERLIMITPRICE = %f,LOWERLIMITPRICE = %f,SETTLEMENTPRICE = %f,CURRDELTA = %f,LASTPRICE = %f,VOLUME = %d,TURNOVER = %f,OPENINTEREST = %f,BIDPRICE1 = %f,BIDVOLUME1 = %d,ASKPRICE1 = %f,ASKVOLUME1 = %d,BIDPRICE2 = %f,BIDVOLUME2 = %d,ASKPRICE2 = %f,ASKVOLUME2 = %d,BIDPRICE3 = %f,BIDVOLUME3 = %d,ASKPRICE3 = %f,ASKVOLUME3 = %d,BIDPRICE4 = %f,BIDVOLUME4 = %d,ASKPRICE4 = %f,ASKVOLUME4 = %d,BIDPRICE5 = %f,BIDVOLUME5 = %d,ASKPRICE5 = %f,ASKVOLUME5 = %d,DATET = to_date('%s','yyyymmddhh24miss'),DATED = %d WHERE INSTRUMENTID = '%s'",
		//	pMarketData->TradingDay,
		//	pMarketData->SettlementGroupID,
		//	pMarketData->SettlementID,

		//	pMarketData->UpdateTime,
		//	pMarketData->UpdateMillisec,
		//	pMarketData->ExchangeID,

		//	//��
		//	pMarketData->PreSettlementPrice,
		//	pMarketData->PreClosePrice,
		//	pMarketData->PreOpenInterest,
		//	pMarketData->PreDelta,

		//	//��
		//	pMarketData->OpenPrice,
		//	pMarketData->HighestPrice,
		//	pMarketData->LowestPrice,
		//	pMarketData->ClosePrice,

		//	pMarketData->UpperLimitPrice,
		//	pMarketData->LowerLimitPrice,
		//	pMarketData->SettlementPrice,
		//	pMarketData->CurrDelta,

		//	//����
		//	pMarketData->LastPrice,
		//	pMarketData->Volume,
		//	pMarketData->Turnover,
		//	pMarketData->OpenInterest,

		//	//��
		//	//һ
		//	pMarketData->BidPrice1,
		//	pMarketData->BidVolume1,
		//	pMarketData->AskPrice1,
		//	pMarketData->AskVolume1,

		//	//��
		//	pMarketData->BidPrice2,
		//	pMarketData->BidVolume2,
		//	pMarketData->AskPrice2,
		//	pMarketData->AskVolume2,

		//	//��
		//	pMarketData->BidPrice3,
		//	pMarketData->BidVolume3,
		//	pMarketData->AskPrice3,
		//	pMarketData->AskVolume3,

		//	//��
		//	pMarketData->BidPrice4,
		//	pMarketData->BidVolume4,
		//	pMarketData->AskPrice4,
		//	pMarketData->AskVolume4,

		//	//��
		//	pMarketData->BidPrice5,
		//	pMarketData->BidVolume5,
		//	pMarketData->AskPrice5,
		//	pMarketData->AskVolume5,
		//	nowtTime,
		//	market_Updatetimes,

		//	pMarketData->InstrumentID
		//);
		//pRst = dbOper.ExecuteWithResSQL(sql);
		//if (NULL != pRst)
		//{
		//	//printf("������������-%s---�ɹ�\n", pMarketData->InstrumentID);
		//}
		//else
		//{
		//	printf("������������---ʧ��\n");
		//}

		//ִ�в�����ʷ�����  
		sprintf_s(sql, "INSERT INTO %s (TRADINGDAY,SETTLEMENTGROUPID,SETTLEMENTID,INSTRUMENTID,UPDATETIME,UPDATEMILLISEC,EXCHANGEID,PRESETTLEMENTPRICE,PRECLOSEPRICE,PREOPENINTEREST,PREDELTA,OPENPRICE,HIGHESTPRICE,LOWESTPRICE,CLOSEPRICE,UPPERLIMITPRICE,LOWERLIMITPRICE,SETTLEMENTPRICE,CURRDELTA,LASTPRICE,VOLUME,TURNOVER,OPENINTEREST,BIDPRICE1,BIDVOLUME1,ASKPRICE1,ASKVOLUME1,BIDPRICE2,BIDVOLUME2,ASKPRICE2,ASKVOLUME2,BIDPRICE3,BIDVOLUME3,ASKPRICE3,ASKVOLUME3,BIDPRICE4,BIDVOLUME4,ASKPRICE4,ASKVOLUME4,BIDPRICE5,BIDVOLUME5,ASKPRICE5,ASKVOLUME5,DATET,DATED) VALUES ('%s','%s',%d,'%s','%s',%d,'%s',%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,to_date('%s','yyyymmddhh24miss'),%d)",
			InstrumentID,
			//����
			pMarketData->TradingDay,
			pMarketData->SettlementGroupID,
			pMarketData->SettlementID,

			pMarketData->InstrumentID,
			pMarketData->UpdateTime,
			pMarketData->UpdateMillisec,
			pMarketData->ExchangeID,

			//��
			pMarketData->PreSettlementPrice,
			pMarketData->PreClosePrice,
			pMarketData->PreOpenInterest,
			pMarketData->PreDelta,

			//��
			pMarketData->OpenPrice,
			pMarketData->HighestPrice,
			pMarketData->LowestPrice,
			pMarketData->ClosePrice,

			pMarketData->UpperLimitPrice,
			pMarketData->LowerLimitPrice,
			pMarketData->SettlementPrice,
			pMarketData->CurrDelta,

			//����
			pMarketData->LastPrice,
			pMarketData->Volume,
			pMarketData->Turnover,
			pMarketData->OpenInterest,

			//��
			//һ
			pMarketData->BidPrice1,
			pMarketData->BidVolume1,
			pMarketData->AskPrice1,
			pMarketData->AskVolume1,

			//��
			pMarketData->BidPrice2,
			pMarketData->BidVolume2,
			pMarketData->AskPrice2,
			pMarketData->AskVolume2,

			//��
			pMarketData->BidPrice3,
			pMarketData->BidVolume3,
			pMarketData->AskPrice3,
			pMarketData->AskVolume3,

			//��
			pMarketData->BidPrice4,
			pMarketData->BidVolume4,
			pMarketData->AskPrice4,
			pMarketData->AskVolume4,

			//��
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
			//printf("������ʷ����--%s-�ɹ�\n", pMarketData->InstrumentID);
		}
		else
		{
			printf("������ʷ����-%s--ʧ��\n",pMarketData->InstrumentID);
		}

		char str_el[20] = "";
		sprintf_s(str_el, "%lf", pMarketData->LastPrice);

		//�ж�
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

				//��
				pMarketData->PreSettlementPrice,
				pMarketData->PreClosePrice,
				pMarketData->PreOpenInterest,
				pMarketData->PreDelta, 

				//��
				pMarketData->OpenPrice,
				pMarketData->HighestPrice,
				pMarketData->LowestPrice,
				pMarketData->ClosePrice,

				pMarketData->UpperLimitPrice,
				pMarketData->LowerLimitPrice,
				pMarketData->SettlementPrice,
				pMarketData->CurrDelta,

				//����
				pMarketData->LastPrice,
				pMarketData->Volume,
				pMarketData->Turnover,
				pMarketData->OpenInterest,

				//��
				//һ
				pMarketData->BidPrice1,
				pMarketData->BidVolume1,
				pMarketData->AskPrice1,
				pMarketData->AskVolume1,

				//��
				pMarketData->BidPrice2,
				pMarketData->BidVolume2,
				pMarketData->AskPrice2,
				pMarketData->AskVolume2,

				//��
				pMarketData->BidPrice3,
				pMarketData->BidVolume3,
				pMarketData->AskPrice3,
				pMarketData->AskVolume3,

				//��
				pMarketData->BidPrice4,
				pMarketData->BidVolume4,
				pMarketData->AskPrice4,
				pMarketData->AskVolume4,

				//��
				pMarketData->BidPrice5,
				pMarketData->BidVolume5,
				pMarketData->AskPrice5,
				pMarketData->AskVolume5
			);

			reply = (redisReply *)redisCommand(rc, "HMSET ALL_InstrumentID %s %s", InstrumentID, value);
			//printf("��ϣ�������Ϣ��HMSET: %s\n\n", reply->str);
			freeReplyObject(reply);
		}
		else
		{
			printf("Ʒ�֣�==%s==�����������ݣ����ܲ���redis\n\n\n", InstrumentID);
		}	

	}

	// ����û�����ĳ���֪ͨ
	void OnRspError(CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspError:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		// �ͻ�������д�����


		// �ͷ�rc��Դ
		redisFree(rc);
		freeReplyObject(reply);

		ConnrectionRedis();
	}

	///���ĺ�Լ�������Ϣ
	void OnRspSubMarketData(CQdpFtdcSpecificInstrumentField *pSpecificInstrument, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("Sub ���ض��ĺ�Լ��%s \n", pSpecificInstrument->InstrumentID);
	}

	///���ĺ�Լ�������Ϣ
	void OnRspUnSubMarketData(CQdpFtdcSpecificInstrumentField *pSpecificInstrument, CQdpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("UnSub ���ض��ĺ�Լ��%s \n", pSpecificInstrument->InstrumentID);
	}

private:
	// ָ��CQdpFtdcMduserApiʵ����ָ��
	CQdpFtdcMduserApi *m_pUserApi;
};



bool RunOnce()
{
	hMutex = CreateMutex(NULL, TRUE, "tickets_q");  //��������
	if (NULL == hMutex)
	{
		return false;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		OutputDebugString("���ⷵ��");
		ReleaseMutex(hMutex);
		CloseHandle(hMutex);
		return false;
	}
	return true;

	/*
	//����exe ƴ�Ӵ������ ������Ŀ���� -���� -�����в���
	//Ĭ�ϲ��Ի���
	strcpy(redisDomainName, "redisd.onehgold.com");
	strcpy(oracleDomainName, "dbd1.onehgold.com");

	//���������в��� �ж����л���
	for (int i = 0; i < argc; i++)
	{
		char argv_s[40];
		if (i == 1)
		{
			int par = atoi(argv[i]);

			if (par == 1)
			{
				strcpy(redisDomainName, "redisd.onehgold.com");
				strcpy(oracleDomainName, "dbd1.onehgold.com");
			}
			else if (par == 2)
			{
				strcpy(redisDomainName, "redist.onehgold.com");
				strcpy(oracleDomainName, "dbt1.onehgold.com");
			}
			else
			{
				strcpy(redisDomainName, "redis.onehgold.com");
				strcpy(oracleDomainName, "db1.onehgold.com");
			}
			printf("���л�����1,���� 2,���� 3,��ʽ\n��ǰ������%d == %s==%s\n", par, redisDomainName, oracleDomainName);
		}

	}
	*/
}

int main(int   argc, char*   argv[])
{
	//�����������
	//if(!RunOnce())
	//{
	//	exit(0);
	//	//return 0;
	//}

	char buf[1000];
	GetCurrentDirectory(1000, buf);  //�õ���ǰ����·��

									 //ƴ��·��
	char tmpc1[1000];
	char tmpc2[] = "/config.txt";
	strcpy(tmpc1, buf);
	strcat(tmpc1, tmpc2);

	string line;
	ifstream inf(tmpc1);
	if (inf.is_open())
	{
		//while (getline(inf, line))
		//{
		//	if (!line.empty())
		//	{
		//		istringstream iss(line);
		//		vector<string> tokens;
		//		copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string>>(tokens));
		//		for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
		//		{
		//			if ((*it).find(".txt") != string::npos)
		//				filenamesv.push_back(*it);
		//				cout << *it << endl; //the file names
		//		}
		//	}
		//}
		inf.close();
	}
	else
	{
		cout << "Uanble to open the file\n";
		return 0;
	}




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

	Config configSettings(tmpc1);

	c_marketAddress = configSettings.Read("marketAddress", c_marketAddress);

	c_oracle_DomainName = configSettings.Read("oracle_DomainName", c_oracle_DomainName);
	c_oracle_Port = configSettings.Read("oracle_Port", 0);
	c_oracle_UserID = configSettings.Read("oracle_UserID", c_oracle_UserID);
	c_oracle_Password = configSettings.Read("oracle_Password", c_oracle_Password);

	c_redis_DomainName = configSettings.Read("redis_DomainName", c_redis_DomainName);
	c_redis_Port = configSettings.Read("redis_Port", 0);

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

	ConnrectionOracle();

	ConnrectionRedis();

	// ����һ��CQdpFtdcMduserApiʵ��
	CQdpFtdcMduserApi *pUserApi = CQdpFtdcMduserApi::CreateFtdcMduserApi();
	// ����һ���¼������ʵ��
	CSimpleHandler sh(pUserApi);
	// ע��һ�¼������ʵ��
	pUserApi->RegisterSpi(&sh);
	// ע����Ҫ�������������
	///        TERT_RESTART:�ӱ������տ�ʼ�ش�
	///        TERT_RESUME:���ϴ��յ�������
	///        TERT_QUICK:�ȴ��͵�ǰ�������,�ٴ��͵�¼���г����������	//pUserApi-> SubscribeMarketDataTopic (101, TERT_RESUME);
	//pUserApi-> SubscribeMarketDataTopic (110, QDP_TERT_RESTART);
	// �������鷢���������ĵ�ַ
	pUserApi->RegisterFront(marketAddress);
	//pUserApi->RegisterFront("tcp://hqsource.onehgold.com:30007");

	// ʹ�ͻ��˿�ʼ�����鷢����������������
	pUserApi->Init();
	// �ͷ�useapiʵ��
	pUserApi->Release();

	_CrtDumpMemoryLeaks();

	return 0;
}


