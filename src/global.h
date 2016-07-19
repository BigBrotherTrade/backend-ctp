#pragma once

#include <cstring>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <redox.hpp>
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcMdApi.h>
#include "json.h"

static const std::string CHANNEL_TRADE_PATTERN = "MSG:CTP:REQ:TRADE:*";   // 监听trade命令
static const std::string CHANNEL_MARKET_PATTERN = "MSG:CTP:REQ:MARKET:*"; // 监听md命令
static const std::string CHANNEL_TRADE_DATA = "MSG:CTP:RSP:TRADE:";  // trade回调通知
static const std::string CHANNEL_MARKET_DATA = "MSG:CTP:RSP:MARKET:";// md回调数据

boost::property_tree::ptree pt;
redox::Redox publisher;
redox::Subscriber subscriber;

CThostFtdcTraderApi* pTraderApi;
CThostFtdcMdApi* pMdApi;

TThostFtdcBrokerIDType	BROKER_ID;		    // 经纪公司代码
TThostFtdcInvestorIDType INVESTOR_ID;		// 投资者代码
TThostFtdcPasswordType  PASSWORD ;			// 用户密码
TThostFtdcFrontIDType FRONT_ID;             // 前置编号
TThostFtdcSessionIDType SESSION_ID;         // 会话编号
TThostFtdcOrderRefType ORDER_REF;           // 报单引用
int ORDER_ID = 0;
time_t lOrderTime;
time_t lOrderOkTime;
// 请求编号
int iRequestID = 0;

char MAC_ADDRESS[64] = {0};
char IP_ADDRESS[32] = {0};

void handle_trade_request(const std::string& topic, const std::string& msg);
void handle_market_request(const std::string& topic, const std::string& msg);

int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen);

const std::string& ntos(const int n);
