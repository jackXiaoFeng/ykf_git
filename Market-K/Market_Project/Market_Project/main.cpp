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


HANDLE hMutex;
CDBOperation dbOper;

redisContext *rc;
redisReply *reply;

char   dateTime[100];

//��ǰ����ʱ���
int nowTimestamp_Zero;
//��ǰʱ�������������
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

int MomdayTime;
int nowDayOfWeek;

int startMonTime;
int nowDayOfMonth;


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
	sprintf_s(str, "Provider = OraOLEDB.Oracle.1; User ID = %s; Password = %s; Data Source = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = %s)(PORT = %d))(CONNECT_DATA = (SERVICE_NAME = ORCL))); Persist Security Info = False", oracle_UserID, oracle_Password, oracle_DomainName, oracle_Port);
	bool bConn = dbOper.ConnToDB(str, oracle_UserID, oracle_Password);
	if (false == bConn)
	{
		printf("�������ݿ���ִ���\n");
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

	//printf("ʱ���%d: ����ʱ�֣�%s��%s\n", (int)tick, s, w);
	strcpy(a, s);
	strcpy(week, w);
}

int compare_time_60(int time, int time_interval_local, char *marketDate)
{
	int endTime_60;
	char s[100];
	if (nowTimestamp_Surplus >= 7200 && nowTimestamp_Surplus < 9060)
	{
		sprintf_s(s, "%s %s", marketDate, "09:30:00");
		endTime_60 = (int)StringToDatetime(s);
	}
	else
	{
		if ((nowTimestamp_Surplus >= 9000 && nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >= 41400 && nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >= 55800 && nowTimestamp_Surplus < 55860))
		{
			//��������ʱ��k�߲��Ӽ��
			endTime_60 = time;
		}
		else
		{
			endTime_60 = time + time_interval_local;
		}

		//ȥ��������
		endTime_60 = endTime_60 - int(endTime_60) % int(time_interval_local);

		if (nowTimestamp_Surplus >= 7200 && nowTimestamp_Surplus < 55860)
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
	char s[100];

	//00:00 10:30 15:30 00:00
	if (nowTimestamp_Surplus >= 0 && nowTimestamp_Surplus < 37800)
	{
		sprintf_s(s, "%s %s", marketDate, "10:30:00");
		endTime_240 = (int)StringToDatetime(s);
	}
	else if (nowTimestamp_Surplus >= 37800 && nowTimestamp_Surplus < 55860)
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
	int endTime_week;
	//00:00 10:30 15:30 00:00
	nowDayOfWeek = 0;
	MomdayTime = 0;

	int wday = ptr->tm_wday;
	nowDayOfWeek = (wday == 0) ? 7 : wday - 1; //�����Ǳ��ܵĵڼ��졣��һ=0������=6
	int nowTime = (int)lt;
	/*time_t now;
	int nowTime = (int)time(&now);
	nowTime = 1489687830;*/
	//��һ
	MomdayTime = nowTime - nowDayOfWeek * 24 * 60 * 60;

	//���� 
	int SaturdayTime = MomdayTime + 4 * 24 * 60 * 60;

	//�������������Ϊ����
	char week[20] = "";
	strftime(week, sizeof(week), "%A", ptr);
	if (strcmp(week, "Saturday") == 0)
	{
		SaturdayTime = SaturdayTime + 7 * 24 * 60 * 60;
	}
	endTime_week = SaturdayTime - (int(SaturdayTime) + 28800) % int(86400);

	MomdayTime = MomdayTime - (int(MomdayTime) + 28800) % int(86400);

	return endTime_week;
}

int get_time_month(time_t lt, tm *ptr)
{
	int endTime_month;
	nowDayOfMonth = 0;
	startMonTime = 0;
	//��k
	char jointDate[50] = "";
	int mday = ptr->tm_mday;
	nowDayOfMonth = mday - 1; //�����Ǳ��µĵڼ��졣1-31
							  //�³�
	sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1, 1, 0, 0, 0);
	startMonTime = StringToDatetime(jointDate);

	//��ĩ
	sprintf_s(jointDate, "%d-%d-%d %d:%d:%d", ptr->tm_year + 1900, ptr->tm_mon + 1 + 1, 1, 0, 0, 0);
	int endMon = StringToDatetime(jointDate);
	endMon = endMon - 24 * 60 * 60;
	endTime_month = endMon - (int(endMon) + 28800) % int(86400);

	return endTime_month;
}

// ��ȡʱ��εĳɽ���
int get_sum_v(int num, char *InstrumentID, int time)
{
	int sum = 0;
	char tempName[100] = "";
	sprintf_s(tempName, "%s_%s", InstrumentID, "f1_day");

	for (int x = 0; x < num; x++)
	{
		int timeV = time + x * 24 * 60 * 60;
		reply = (redisReply *)redisCommand(rc, "HGET  %s %d", tempName, timeV);
		if (reply->str != NULL)
		{
			cJSON *root = cJSON_Parse(reply->str);
			int  total_v = cJSON_GetObjectItem(root, "v")->valueint;
			sum += total_v;
			//printf("��Сtime=%d--���time=%d=dds-%d=ʵʱV=%d==���V=%d==���V=%d--\n", min_time, max_time, dds,v, total_v, fenshiInterval_v);
		}
		freeReplyObject(reply);
	}
	return sum;
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

//ɾ�������ַ�  || �滻���������ַ�
void delAndReplace(char *str, char *d, char oldChar, char newChar)
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
		char nowtDate[30] = "";
		struct tm *ptr;
		time_t lt = time(NULL);
		ptr = localtime(&lt);
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d %H:%M:%S ", ptr);
		int now = time(NULL);
		char value[200] = "";
		sprintf(value, "{\"date\":%s,\"ErrorCode\":%d,\"ErrorMsg\":%s,\"RequestID\":%d,\"Chain\":%d}", nowtDate, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
		reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", "k_tcp_login_Log", now, value);
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
	//���������Ϣ
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
	}

	// �������֪ͨ�����������������֪ͨ�ͻ���
	void OnRtnDepthMarketData(CQdpFtdcDepthMarketDataField *pMarketData)
	{

		//�������
		//printf_Market(pMarketData);

		//�ݶ�ֻ�洢4��Ʒ��K��
		if (!(strcmp(pMarketData->InstrumentID, "Ag(T+D)") == 0 ||
			strcmp(pMarketData->InstrumentID, "Au(T+D)") == 0 ||
			strcmp(pMarketData->InstrumentID, "Au100g") == 0 ||
			strcmp(pMarketData->InstrumentID, "mAu(T+D)") == 0))
		{
			return;
		}

		//�������ʱ��Ϊ  �������������� ƴ������ʱ����
		char nowtDate[30] = "";
		char marketDate[12] = "";
		char market_time_str[30] = "";
		char r_Market_time_str[30] = "";
		char time_hms[9] = "";

		struct tm *ptr;
		time_t lt;
		lt = time(NULL);
		ptr = localtime(&lt);
		//fix 1:�������ñ���������ƴ������ʱ���� ��Ϊ����ʱ����˵�֮�����ڻ��Ϊ��һ��
		//��������+����ʱ���� ���ڼ��� ����k
		strftime(nowtDate, sizeof(nowtDate), "%Y-%m-%d", ptr);
		strcpy(market_time_str, nowtDate);
		strcpy(time_hms, pMarketData->UpdateTime);
		strcat(market_time_str, " ");
		strcat(market_time_str, time_hms);
		int market_Updatetimes = (int)StringToDatetime(market_time_str);

		//������ʱ�����ӳ� ����23:59:59�� ƴ�ӵ�ʱ�����ֱ��������Ѿ���һ�� ����ʱ�仹����һ���bug �����ж��������ʱ������ڱ���ʱ����ʹ��ڲ�ֻһ������ �ͼ�ȥһ��������
		int now = (int)lt;
		if (market_Updatetimes > now && (market_Updatetimes - now) > 60)
		{
			market_Updatetimes -= 24 * 60 * 60;
		}


		//��������+����ʱ���� ���ڼ�����/��/��k
		int r_Market_Updatetimes;
		strcpy(marketDate, pMarketData->TradingDay);
		sprintf_s(marketDate, "%s-%s-%s", substring(marketDate, 0, 4), substring(marketDate, 4, 2), substring(marketDate, 6, 2));

		if (strcmp(marketDate, market_time_str) != 0)
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
		15:30 = 55800 15:30 = 55860
		*/
		//�����������ʱ����ȷ���Ƿ����ڿ��̼�֮�� �Ƿ��redis
		int dds = market_Updatetimes;
		nowTimestamp_Surplus = (int(dds) + 28800) % int(86400);
		nowTimestamp_Zero = dds - nowTimestamp_Surplus;

		//ͣ��ʱ���һ���� ��Ϊ�˽���ʱ30:00��ȥǰһ��ʱ��� ����ɽ���
		if ((nowTimestamp_Surplus > 72000 || nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >32400 & nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >48600 & nowTimestamp_Surplus < 55860))
		{
			//printf("����ʱ��");
		}
		else
		{
			//printf("����ʱ��\n");
			//����ʱ�� ֱ�������ص�����
			return;
		}

		//�Ƿ���ͣ�����һ����
		bool isLastMinutes = false;
		if ((nowTimestamp_Surplus >= 9000 && nowTimestamp_Surplus < 9060) ||
			(nowTimestamp_Surplus >= 41400 && nowTimestamp_Surplus < 41460) ||
			(nowTimestamp_Surplus >= 55800 && nowTimestamp_Surplus < 55860))
		{
			//��������ʱ��k�߲��Ӽ��
			isLastMinutes = true;
		}

		//��Լ��ʾȥ�������ַ� ��ת�ɴ�д�ַ�
		char origin_InstrumentID[31] = "";
		strcpy(origin_InstrumentID, pMarketData->InstrumentID);
		char InstrumentID[30] = "";
		char d[5] = { '(', ')', '+' };
		delAndReplace(origin_InstrumentID, d, '.', '_');
		strcpy(InstrumentID, origin_InstrumentID);

		printf("K�����ࣺ%sƴ��ʱ�䣺%sʱ�����%d==\n", InstrumentID, time_hms, market_Updatetimes);

		//���岢��ֵ���洢redis������Ϣ
		double maxl = pMarketData->HighestPrice;
		double minl = pMarketData->LowestPrice;
		double sl = pMarketData->OpenPrice;
		double el = pMarketData->LastPrice;
		int v = pMarketData->Volume;
		double turnover = pMarketData->Turnover;

		//�洢��Ϣ
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

		//ȥ��ʱ�������
		int ss = int(market_Updatetimes) % int(60);
		market_Updatetimes = market_Updatetimes - ss;

		int ss1 = int(r_Market_Updatetimes) % int(60);
		r_Market_Updatetimes = r_Market_Updatetimes - ss1;

		int r_TempTime = r_Market_Updatetimes - (int(r_Market_Updatetimes) + 28800) % int(86400);

		int endTime = 0;
		int fenshi_i = 6;//�жϷ�ʱ
		int len = sizeof(nameArray) / sizeof(char*);
		for (int i = 0; i < len; ++i)
		{
			//�趨k�����ֺ�ʱ����
			strcpy(name, nameArray[i]);
			sprintf_s(myhash, "%s_%s", InstrumentID, name);
			sprintf_s(myhash_v, "%s_%s_%s", InstrumentID, name, "CurrentV");

			if (i == 4)
			{
				//60���� K
				endTime = compare_time_60(market_Updatetimes, time_interval_local_array[i], marketDate);
			}
			else if (i == 5)
			{
				//240���� K
				endTime = compare_time_240(marketDate);
			}
			else if (i == 7)
			{
				//�� K
				endTime = r_TempTime;
			}
			else if (i == 8)
			{
				//�� k
				int endTime_week = get_time_week(r_lt, r_ptr);
				//�����ճɽ��� �������֮ǰ�ܳɽ���
				week_v = get_sum_v(nowDayOfWeek, InstrumentID, MomdayTime);

				//�Ե�ǰ����ʱ����Ϊkey
				endTime = r_TempTime;
			}
			else if (i == 9)
			{
				//��k
				int endTime_month = get_time_month(r_lt, r_ptr);
				//�����ճɽ��� �������֮ǰ�³ɽ���
				month_v = get_sum_v(nowDayOfMonth, InstrumentID, startMonTime);

				//�Ե�ǰ����ʱ����Ϊkey
				endTime = r_TempTime;
			}
			else
			{
				// 1 5 15 30
				time_interval_local = time_interval_local_array[i];
				
				int local = isLastMinutes ? 0 : time_interval_local;
				endTime = market_Updatetimes + local;
				endTime = endTime - int(endTime) % int(time_interval_local);
			}


			char str_el[20] = "";
			sprintf_s(str_el, "%lf", el);
			//�ж�
			if (strlen(str_el) != 0)
			{
				reply = (redisReply *)redisCommand(rc, "HKEYS %s", myhash);
				if (reply &&  reply->elements>0) {
					//�ͷ�HKEYS ��reply 
					freeReplyObject(reply);

					//��ȡredis����¼��ϣ�������� �ɽ���v  �õ�ǰ����ɽ���v ��ȥ��¼v �������ʱ���v��ֵ
					reply = (redisReply *)redisCommand(rc, "GET %s", myhash_v);
					if (reply->str == NULL)
					{
						printf("redis-��ȡ%sʧ��", myhash_v);
						return;
					}
					cJSON *root = cJSON_Parse(reply->str);
					double current_maxl;
					double current_minl;
					double current_sl;

					double origin_maxl;
					double origin_minl;
					double origin_sl;
					double origin_el;
					double origin_turnover;


					//�ж��Ƿ����¸��� ��k �����ȷ�� �ж�
					bool isNext = false;

					int current_v = cJSON_GetObjectItem(root, "v")->valueint;
					int max_time = cJSON_GetObjectItem(root, "dated")->valueint;
					int interval_v = cJSON_GetObjectItem(root, "interval_v")->valueint;
					origin_el = cJSON_GetObjectItem(root, "el")->valuedouble;

					//���̼�Ϊ�� �Ͱ���һ�����̼۸�ֵ�����̼� 
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

					if (i == 9)
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

					//�²������� �Ƿ�Ҫ���¼�¼����
					int upDateCurrent = true;

					if (endTime - max_time >= time_interval_local_array[i] || isNext)
					{
						int now_time_v = v - interval_v - current_v;
						//��8:00 ���̳ɽ���==0��
						if (now_time_v < 0)
						{
							now_time_v = v;
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

						//printf("<<v=%d<<<<<interval_v=%d<<<<<current_v=%d<<<<<<<<��%d��>>>>>>>>>>>>>>>>\n", v, interval_v, current_v, now_time_v);
						if (i == fenshi_i)
						{
							float el_meanline = turnover / v;
							sprintf(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%lf,\"dated\":%d}", el, now_time_v, el_meanline, endTime);
						}
						else
						{
							sprintf(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, el, el, now_time_v, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, endTime, value);
						freeReplyObject(reply);

						//����v
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, now_time_v, v, turnover, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, el, el, now_time_v, v, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						freeReplyObject(reply);

						//������һ������ֵ
						endTime = max_time;
						//�����¼�¼ֵ
						upDateCurrent = false;
					}
					/*else
					{*/
					//ʱ������ update_v = v - interval_v;

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
							//��һ�������� ��Ϊ�ϸ������¼ֵ
							update_v = current_v;
						}
					}

					//printf("С��ʱ�����ڵ�����: %d==������%d=v_changeNum=%d--sum_el-%f\n\n", v, update_v, v_changeNum, sum_el);

					//����k�����ݣ��ɽ��������� ���£�
					//���¼������ ����˳ɽ��� ����ֵȫ����ԭ��¼ֵ
					if (upDateCurrent)
					{
						if (i == fenshi_i)
						{
							float el_meanline = turnover / v;
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%f,\"dated\":%d}", el, update_v, el_meanline, endTime);
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
							float el_meanline = origin_turnover / (current_v + interval_v);
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%f,\"dated\":%d}", origin_el, update_v, el_meanline, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", origin_maxl, origin_minl, origin_sl, origin_el, update_v, endTime);
						}
					}

					reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, max_time, value);
					freeReplyObject(reply);

					//���¼�¼���ݣ����̼ۣ��ɽ��� �����£��൱��k���������һ��
					if (upDateCurrent)
					{
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, update_v, interval_v, turnover, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", current_maxl, current_minl, current_sl, el, update_v, interval_v, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						freeReplyObject(reply);

						//�� ��k���ʱ��� ��Ҫ��ʾ����ʱ��� ��������/�� ����Ϊ����/�� ��Ҫɾ��ǰһ����ʱ���
						if (i == 8 || i == 9)
						{
							if (endTime != max_time)
							{
								reply = (redisReply *)redisCommand(rc, "HDEL %s %d", myhash, max_time);
								freeReplyObject(reply);
							}
						}
					}
					//}
					free(root);
				}
				else {
					printf("redisCommand [lrange mylist 0 -1] error:%d. %s\n", reply->type, reply->str);
					if (reply->str == NULL)
					{
						//printf("û�� ����¼��ϣ�� �� ��k�߹�ϣ��\n");
						//�ͷ�HKEYS ��reply 
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

						//û�� ��k�߹�ϣ�� �򴴽��� �����ø�����Ĭ��ֵ
						//��¼ֵĬ�� ����LastPrice vĬ���ܳɽ��� endTimeĬ������ȥ����ʱ��
						if (i == fenshi_i)
						{
							float el_meanline = turnover / v;
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"el_meanline\":%lf,\"dated\":%d}", el, tempV, el_meanline, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"dated\":%d}", el, el, el, el, tempV, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", myhash, endTime, value);
						//printf("��һ����ϣ�������Ϣ��HMSET: %s\n\n\n", reply->str);
						freeReplyObject(reply);

						//��¼ֵĬ�� ����LastPrice v �� interval_v Ĭ���ܳɽ��� endTimeĬ������ȥ����ʱ��
						if (i == fenshi_i)
						{
							sprintf_s(value, "{\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"turnover\":%lf,\"dated\":%d}", el, v, v, turnover, endTime);
						}
						else
						{
							sprintf_s(value, "{\"maxl\":%lf,\"minl\":%lf,\"sl\":%lf,\"el\":%lf,\"v\":%d,\"interval_v\":%d,\"dated\":%d}", el, el, el, el, v, v, endTime);
						}
						reply = (redisReply *)redisCommand(rc, "SET %s %s", myhash_v, value);
						//printf("��һ��SET: %s\n", reply->str);
						freeReplyObject(reply);
					}
				}
			}
			else
			{
				printf("Ʒ�֣�==%s==�����������ݣ����ܲ���redis\n\n\n", InstrumentID);
			}
		}

		//��ϸ
		char detailName[30] = "fenshi_1_minutes_detail";

		char detail_myhash[50];
		sprintf_s(detail_myhash, "%s_%s", InstrumentID, detailName);

		reply = (redisReply *)redisCommand(rc, "HKEYS %s", detail_myhash);
		//printf("=========HKEYS: %d\n", reply->elements);

		if (reply &&  reply->elements>0) {

			int max_time;
			int min_time;

			//����
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
			//�ͷ�HKEYS ��reply 
			freeReplyObject(reply);

			reply = (redisReply *)redisCommand(rc, "HGET  %s %d", detail_myhash, max_time);
			if (reply->str == NULL)
			{
				return;
			}
			cJSON *root = cJSON_Parse(reply->str);
			int  temInterval_v = cJSON_GetObjectItem(root, "VOL")->valueint;
			int  total_v = cJSON_GetObjectItem(root, "total_VOL")->valueint;

			int fenshiInterval_v = v - total_v;
			//printf("��Сtime=%d--���time=%d=dds-%d=ʵʱV=%d==���V=%d==���V=%d--\n", min_time, max_time, dds,v, total_v, fenshiInterval_v);
			freeReplyObject(reply);

			if (fenshiInterval_v != 0 && max_time != dds)
			{
				if (len > 19)
				{
					//ɾ����Сֵ
					//��¼ֵĬ�� ����LastPrice vĬ���ܳɽ��� endTimeĬ������ȥ����ʱ��
					reply = (redisReply *)redisCommand(rc, "HDEL %s %d %s", detail_myhash, min_time);
					//printf("ɾ����С%d---%s\n", min_time, reply->str);
					freeReplyObject(reply);
				}

				//����
				//��¼ֵĬ�� ����LastPrice vĬ���ܳɽ��� endTimeĬ������ȥ����ʱ��
				sprintf_s(value, "{\"time\":%d,\"price\":%lf,\"VOL\":%d,\"total_VOL\":%d}", dds, el, fenshiInterval_v, v);
				reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", detail_myhash, dds, value);
				//printf("����ʵʱ%d---%s\n", dds, reply->str);
				freeReplyObject(reply);

			}
		}
		else {
			printf("redisCommand [lrange mylist 0 -1] error:%d. %s\n", reply->type, reply->str);
			if (reply->str == NULL)
			{
				//�ͷ�HKEYS ��reply 
				freeReplyObject(reply);

				//û�� ��k�߹�ϣ�� �򴴽��� �����ø�����Ĭ��ֵ
				//��¼ֵĬ�� ����LastPrice vĬ���ܳɽ��� endTimeĬ������ȥ����ʱ��
				sprintf_s(value, "{\"time\":%d,\"price\":%lf,\"VOL\":%d,\"total_VOL\":%d}", dds, el, v, v);
				reply = (redisReply *)redisCommand(rc, "HMSET %s %d %s", detail_myhash, dds, value);
				freeReplyObject(reply);
			}
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
}

int main()
{
	//�����������
	//if(!RunOnce())
	//{
	//	exit(0);
	//	//return 0;
	//}

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

	const char ConfigFile[] = "config.txt";
	Config configSettings(ConfigFile);

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

	//����redis
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


