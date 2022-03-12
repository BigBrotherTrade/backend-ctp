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
#pragma warning( push )
#pragma warning( disable : 4200 )  // 移除MSVC编译器警告
#include <sw/redis++/redis++.h>
#pragma warning( pop )
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcMdApi.h>
#define JSON_HAS_CPP_17 1
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include "json.hpp"
#include "easylogging++.h"

inline constexpr auto CHANNEL_REQ_PATTERN = "MSG:CTP:REQ:*";         // 监听req命令
inline constexpr auto CHANNEL_TRADE_DATA = "MSG:CTP:RSP:TRADE:";     // trade回调通知
inline constexpr auto CHANNEL_MARKET_DATA = "MSG:CTP:RSP:MARKET:";   // md回调数据

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
extern bool trade_login;
extern bool market_login;

extern void handle_req_request([[maybe_unused]] std::string pattern, std::string channel, std::string msg);
extern int gb2312toutf8(char *sourcebuf, [[maybe_unused]] size_t sourcelen, char *destbuf, size_t destlen);
extern void handle_command(std::stop_token);
extern std::condition_variable& getCond();