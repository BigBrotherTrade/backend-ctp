#pragma once

#include <cstring>
#include <redox.hpp>
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcMdApi.h>
#include "json.h"

static const std::string CHANNEL_REQ_PATTERN = "MSG:CTP:REQ:*"; // 监听req命令
static const std::string CHANNEL_TRADE_DATA = "MSG:CTP:RSP:TRADE:";  // trade回调通知
static const std::string CHANNEL_MARKET_DATA = "MSG:CTP:RSP:MARKET:";// md回调数据

extern Json::FastWriter writer;
extern redox::Redox publisher;
extern redox::Subscriber subscriber;

extern CThostFtdcTraderApi* pTraderApi;
extern CThostFtdcMdApi* pMdApi;

extern TThostFtdcBrokerIDType	BROKER_ID;		    // 经纪公司代码
extern TThostFtdcInvestorIDType INVESTOR_ID;		// 投资者代码
extern TThostFtdcPasswordType  PASSWORD ;			// 用户密码
extern TThostFtdcFrontIDType FRONT_ID;             // 前置编号
extern TThostFtdcSessionIDType SESSION_ID;         // 会话编号

extern std::atomic<int> iMarketRequestID;
extern std::atomic<int> iTradeRequestID;
extern std::atomic<bool> query_finished;
extern std::atomic<bool> keep_running;
extern std::atomic<bool> trade_login;
extern std::atomic<bool> market_login;
extern std::condition_variable check_cmd;

extern char MAC_ADDRESS[64];
extern char IP_ADDRESS[32];

void handle_req_request(const std::string& topic, const std::string& msg);

int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen);

const std::string ntos(const int n);

void handle_command();