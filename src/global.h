/*
 * Copyright 2016 timercrack
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstring>
#include <sw/redis++/redis++.h>
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcMdApi.h>
#include "json.h"
#include "easylogging++.h"

static const std::string CHANNEL_REQ_PATTERN = "MSG:CTP:REQ:*"; // 监听req命令
static const std::string CHANNEL_TRADE_DATA = "MSG:CTP:RSP:TRADE:";  // trade回调通知
static const std::string CHANNEL_MARKET_DATA = "MSG:CTP:RSP:MARKET:";// md回调数据

extern el::Logger* logger;
extern sw::redis::Redis* publisher;

extern CThostFtdcTraderApi* pTraderApi;
extern CThostFtdcMdApi* pMdApi;

extern std::string MAC_ADDRESS;
extern std::string IP_ADDRESS;
extern std::string BROKER_ID;		    // 经纪公司代码
extern std::string INVESTOR_ID;		    // 投资者代码
extern std::string PASSWORD;			// 用户密码
extern std::string APPID;	            // appid
extern std::string AUTHCODE;			// 认证码
extern std::string USERINFO;	        // 产品信息
extern TThostFtdcFrontIDType FRONT_ID;             // 前置编号
extern TThostFtdcSessionIDType SESSION_ID;         // 会话编号

extern int iMarketRequestID;
extern int iTradeRequestID;
extern bool query_finished;
extern bool keep_running;
extern bool trade_login;
extern bool market_login;
extern std::condition_variable check_cmd;

extern void handle_req_request(std::string pattern, std::string channel, std::string msg);

int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen);

std::string ntos(int n);

void handle_command();