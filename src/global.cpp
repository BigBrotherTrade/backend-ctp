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
#include "global.h"

#if defined(__linux__)
#include <fmt/core.h>
#include <iconv.h>
#elif defined(_WIN32)

#include <windows.h>
#include <format>

#endif

#include <chrono>
#include <queue>
#include <thread>

#define ERROR_RESULT -999

using namespace std;
#if defined(__linux__)
using namespace fmt;
#endif
using namespace chrono;
using json = nlohmann::json;

string MAC_ADDRESS;
string IP_ADDRESS;
string BROKER_ID;        // 经纪公司代码
string INVESTOR_ID;        // 投资者代码
string PASSWORD;        // 用户密码
string APPID;                // appid
string AUTHCODE;            // 认证码
string USERINFO;            // 产品信息
TThostFtdcFrontIDType FRONT_ID;             // 前置编号
TThostFtdcSessionIDType SESSION_ID;         // 会话编号
CThostFtdcTraderApi *pTraderApi = nullptr;
CThostFtdcMdApi *pMdApi = nullptr;
el::Logger *logger = nullptr;
sw::redis::Redis *publisher = nullptr;

int iMarketRequestID(0);
int iTradeRequestID(0);
bool query_finished(true);
bool trade_login(false);
bool market_login(false);

mutex mut;
typedef queue<pair<string, json> > CmdQueue;

CmdQueue &getQueue() {
    static CmdQueue cmd_queue;
    return cmd_queue;
}

condition_variable &getCond() {
    static condition_variable check_cmd;
    return check_cmd;
}

#if defined(__linux__)
int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen) {
    iconv_t cd;
    if ((cd = iconv_open("utf-8", "gb2312")) == nullptr)
        return -1;
    memset(destbuf, 0, destlen);
    char **source = &sourcebuf;
    char **dest = &destbuf;
    iconv(cd, source, &sourcelen, dest, &destlen);
    iconv_close(cd);
    return 0;
}
#elif defined(_WIN32)
int gb2312toutf8(char *sourcebuf, [[maybe_unused]] size_t sourcelen, char *destbuf, [[maybe_unused]] size_t destlen) {
    int len = MultiByteToWideChar(CP_ACP, 0, sourcebuf, -1, nullptr, 0);
    auto *wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, sourcebuf, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, destbuf, len, nullptr, nullptr);
    delete[] wstr;
    return 0;
}
#endif

int IsMarketLogin(const json &root) {
    publisher->publish(format("{}IsMarketLogin", CHANNEL_MARKET_DATA), format("{}", market_login));
    return 1;
}

int IsTradeLogin(const json &root) {
    publisher->publish(format("{}IsTradeLogin", CHANNEL_MARKET_DATA), format("{}", trade_login));
    return 1;
}

int MarketReqUserLogin(const json &root) {
    try {
        CThostFtdcReqUserLoginField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.UserID, INVESTOR_ID.c_str());
        strcpy(req.Password, PASSWORD.c_str());
        iMarketRequestID = root["RequestID"].get<int>();
        return pMdApi->ReqUserLogin(&req, iMarketRequestID);
    } catch (json::exception &e) {
        logger->error("MarketReqUserLogin failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int SubscribeMarketData(const json &root) {
    try {
        auto **charArray = new char *[root.size()];
        for (unsigned int i = 0; i < root.size(); ++i) {
            charArray[i] = new char[32];
            strcpy(charArray[i], root[i].get<string>().c_str());
        }
        logger->info("订阅合约数量: %v", root.size());
        int iResult = pMdApi->SubscribeMarketData(charArray, (int) root.size());
        for (unsigned int i = 0; i < root.size(); ++i)
            delete charArray[i];
        delete[](charArray);
        return iResult;
    } catch (json::exception &e) {
        logger->error("SubscribeMarketData failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int UnSubscribeMarketData(const json &root) {
    try {
        auto **charArray = new char *[root.size()];
        for (unsigned int i = 0; i < root.size(); ++i) {
            charArray[i] = new char[32];
            strcpy(charArray[i], root[i].get<string>().c_str());
        }
        int iResult = pMdApi->UnSubscribeMarketData(charArray, (int) root.size());
        for (unsigned int i = 0; i < root.size(); ++i)
            delete charArray[i];
        delete[](charArray);
        return iResult;
    } catch (json::exception &e) {
        logger->error("UnSubscribeMarketData failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqSettlementInfoConfirm(const json &root) {
    try {
        CThostFtdcSettlementInfoConfirmField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        return pTraderApi->ReqSettlementInfoConfirm(&req, root["RequestID"].get<int>());
    } catch (json::exception &e) {
        logger->error("ReqSettlementInfoConfirm failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int TradeReqUserLogin(const json &root) {
    try {
        CThostFtdcReqUserLoginField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.UserID, INVESTOR_ID.c_str());
        strcpy(req.Password, PASSWORD.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqUserLogin(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("TradeReqUserLogin failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqOrderInsert(const json &root) {
    try {
        CThostFtdcInputOrderField req{};
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        req.Direction = root["Direction"].get<string>()[0];
        strncpy(req.OrderRef, root["OrderRef"].get<string>().c_str(), sizeof(req.OrderRef));
        req.LimitPrice = root["LimitPrice"].get<double>();
        req.VolumeTotalOriginal = root["VolumeTotalOriginal"].get<int>();
        req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        req.CombOffsetFlag[0] = root["CombOffsetFlag"].get<string>()[0];
        req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
        req.VolumeCondition = THOST_FTDC_VC_AV;
        req.MinVolume = 1;
        req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        req.ContingentCondition = THOST_FTDC_CC_Immediately;
        req.IsAutoSuspend = 1;
        req.UserForceClose = 0;
        req.TimeCondition = THOST_FTDC_TC_GFD;
        strcpy(req.IPAddress, IP_ADDRESS.c_str());
        strcpy(req.MacAddress, MAC_ADDRESS.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqOrderInsert(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqOrderInsert failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqOrderAction(const json &root) {
    try {
        CThostFtdcInputOrderActionField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        strcpy(req.ExchangeID, root["ExchangeID"].get<string>().c_str());
        strcpy(req.UserID, root["UserID"].get<string>().c_str());
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        req.OrderActionRef = root["RequestID"].get<int>() + 1;
        req.ActionFlag = THOST_FTDC_AF_Delete;
        strcpy(req.IPAddress, IP_ADDRESS.c_str());
        strcpy(req.MacAddress, MAC_ADDRESS.c_str());
        strcpy(req.OrderSysID, root["OrderSysID"].get<string>().c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqOrderAction(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqOrderAction failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQrySettlementInfo(const json &root) {
    try {
        CThostFtdcQrySettlementInfoField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQrySettlementInfo(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQrySettlementInfo failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryInstrument(const json &root) {
    try {
        CThostFtdcQryInstrumentField req{};
        if (root.contains("InstrumentID")) {
            strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        }
        if (root.contains("ExchangeID")) {
            strcpy(req.ExchangeID, root["ExchangeID"].get<string>().c_str());
        }
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryInstrument(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryInstrument failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryInstrumentCommissionRate(const json &root) {
    try {
        CThostFtdcQryInstrumentCommissionRateField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        if (root.contains("InstrumentID")) {
            strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        }
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryInstrumentCommissionRate(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryInstrumentCommissionRate failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryInstrumentMarginRate(const json &root) {
    try {
        CThostFtdcQryInstrumentMarginRateField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        if (root.contains("InstrumentID")) {
            strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        }
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        req.HedgeFlag = THOST_FTDC_HF_Speculation;
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryInstrumentMarginRate(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryInstrumentMarginRate failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryTradingAccount(const json &root) {
    try {
        CThostFtdcQryTradingAccountField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryTradingAccount(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryTradingAccount failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryInvestorPosition(const json &root) {
    try {
        CThostFtdcQryInvestorPositionField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        if (root.contains("InstrumentID")) {
            strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        }
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryInvestorPosition(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryInvestorPosition failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryInvestorPositionDetail(const json &root) {
    try {
        CThostFtdcQryInvestorPositionDetailField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        if (root.contains("InstrumentID")) {
            strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        }
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryInvestorPositionDetail(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryInvestorPositionDetail failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryOrder(const json &root) {
    try {
        CThostFtdcQryOrderField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryOrder(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryOrder failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryTrade(const json &root) {
    try {
        CThostFtdcQryTradeField req{};
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        strcpy(req.TradeID, root["TradeID"].get<string>().c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryTrade(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryTrade failed: %v", e.what());
        return ERROR_RESULT;
    }
}

int ReqQryDepthMarketData(const json &root) {
    try {
        CThostFtdcQryDepthMarketDataField req{};
        strcpy(req.ExchangeID, root["ExchangeID"].get<string>().c_str());
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqQryDepthMarketData(&req, iTradeRequestID);
    } catch (json::exception &e) {
        logger->error("ReqQryDepthMarketData failed: %v", e.what());
        return ERROR_RESULT;
    }
}

void handle_req_request([[maybe_unused]] string pattern, string channel, string msg) {
    auto request_type = channel.substr(channel.find_last_of(':') + 1);
    json json_msg;
    try {
        json_msg = json::parse(msg);
        cout << request_type << "->" << json_msg << endl;
    } catch (json::exception &e) {
        logger->error("handle_req_request failed: %v", e.what());
        return;
    }
    lock_guard<mutex> lk(mut);
    CmdQueue &cmd_queue = getQueue();
    cmd_queue.emplace(request_type, json_msg);
    if (request_type.starts_with("Subscribe"))
        logger->info("queue_size: %v, cmd: %v", cmd_queue.size(), request_type);
    else
        logger->info("queue_size: %v, cmd: %v, msg: %v", cmd_queue.size(), request_type, msg);
    getCond().notify_all();
}

void handle_command(std::stop_token stop_token) {
    map<string, function<int(const json &)> > req_func;
    req_func["IsMarketLogin"] = &IsMarketLogin;
    req_func["IsTradeLogin"] = &IsTradeLogin;
    req_func["MarketReqUserLogin"] = &MarketReqUserLogin;
    req_func["SubscribeMarketData"] = &SubscribeMarketData;
    req_func["UnSubscribeMarketData"] = &UnSubscribeMarketData;
    req_func["ReqSettlementInfoConfirm"] = &ReqSettlementInfoConfirm;
    req_func["TradeReqUserLogin"] = &TradeReqUserLogin;
    req_func["ReqOrderInsert"] = &ReqOrderInsert;
    req_func["ReqOrderAction"] = &ReqOrderAction;
    req_func["ReqQrySettlementInfo"] = &ReqQrySettlementInfo;
    req_func["ReqQryInstrument"] = &ReqQryInstrument;
    req_func["ReqQryInstrumentCommissionRate"] = &ReqQryInstrumentCommissionRate;
    req_func["ReqQryInstrumentMarginRate"] = &ReqQryInstrumentMarginRate;
    req_func["ReqQryTradingAccount"] = &ReqQryTradingAccount;
    req_func["ReqQryInvestorPosition"] = &ReqQryInvestorPosition;
    req_func["ReqQryInvestorPositionDetail"] = &ReqQryInvestorPositionDetail;
    req_func["ReqQryOrder"] = &ReqQryOrder;
    req_func["ReqQryTrade"] = &ReqQryTrade;
    req_func["ReqQryDepthMarketData"] = &ReqQryDepthMarketData;

    query_finished = true;
    auto start = high_resolution_clock::now();
    el::Helpers::setThreadName("command");
    logger->info("命令处理线程已启动.");
    CmdQueue &cmd_queue = getQueue();
    while (!stop_token.stop_requested()) {
        if (cmd_queue.empty()) {
            unique_lock<mutex> lk(mut);
            bool wait_rst = getCond().wait_for(lk, seconds(1), [start, &cmd_queue] {
                // 没有新命令
                if (cmd_queue.empty())
                    return false;
                // 新命令是交易类命令， 直接执行
                if (cmd_queue.front().first.starts_with("ReqQry"))
                    return true;
                // 上一次的查询命令执行完毕，可以执行新查询
                if (query_finished)
                    return true;
                // 上一次的查询命令执行超时，可以执行新查询
                return high_resolution_clock::now() - start > seconds(30);
            });
            if (!wait_rst) continue;
        }
        auto cmd_pair = cmd_queue.front();
        cmd_queue.pop();
        auto func = req_func.find(cmd_pair.first);
        if (func == req_func.end()) {
            logger->error("can't find req_func=%v", cmd_pair.second);
            continue;
        }
        // 查询类接口调用频率限制为1秒一次
        if (cmd_pair.first.starts_with("ReqQry")) {
            this_thread::sleep_until(start + milliseconds(1000));
            start = high_resolution_clock::now();
            query_finished = false;
        } else {
            query_finished = true;
        }
        if (cmd_pair.first.starts_with("Subscribe"))
            logger->info("发送命令 %v", cmd_pair.first);
        else
            logger->info("发送命令 %v : %v", cmd_pair.first, cmd_pair.second);
        int iResult = (func->second)(cmd_pair.second);
        json err = "发送成功";
        if (iResult == -1)
            err = "因网络原因发送失败";
        else if (iResult == -2)
            err = "未处理请求队列总数量超限";
        else if (iResult == -3)
            err = "每秒发送请求数量超限";
        else if (iResult == ERROR_RESULT)
            err = "发送失败";
        logger->info("结果: %v", err);
        if (iResult != ERROR_RESULT && iResult < 0) {
            publisher->publish(format("{}OnRspError:{}", CHANNEL_MARKET_DATA, cmd_pair.second["RequestID"].get<int>()), cmd_pair.second.dump());
        }
    }
    logger->info("监听线程已退出.");
}
