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
#include <iconv.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#include <queue>
#include <thread>

using namespace std;
using json = nlohmann::json;

string MAC_ADDRESS;
string IP_ADDRESS;
string BROKER_ID;		// 经纪公司代码
string INVESTOR_ID;		// 投资者代码
string PASSWORD;		// 用户密码
string APPID;	            // appid
string AUTHCODE;			// 认证码
string USERINFO;	        // 产品信息
TThostFtdcFrontIDType FRONT_ID;             // 前置编号
TThostFtdcSessionIDType SESSION_ID;         // 会话编号
CThostFtdcTraderApi* pTraderApi = nullptr;
CThostFtdcMdApi* pMdApi = nullptr;
el::Logger* logger = nullptr;
sw::redis::Redis* publisher = nullptr;

int iMarketRequestID(0);
int iTradeRequestID(0);
bool query_finished(true);
bool keep_running(true);
bool trade_login(false);
bool market_login(false);

mutex mut;
condition_variable check_cmd;
queue<pair<string, json> > cmd_queue;
map<string, function<int(const json &)> > req_func;

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
int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen) {
    int len = MultiByteToWideChar(CP_ACP, 0, sourcebuf, -1, nullptr, 0);
    auto* wstr = new wchar_t[len+1];
    memset(wstr, 0, len+1);
    MultiByteToWideChar(CP_ACP, 0, sourcebuf, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, destbuf, len, nullptr, nullptr);
    delete[] wstr;
    return 0;
}
#endif

std::string ntos(int n) {
    std::stringstream newstr;
    newstr << n;
    return newstr.str();
}

int IsMarketLogin(const json &root) {
    publisher->publish(CHANNEL_MARKET_DATA + "IsMarketLogin", ntos(market_login));
    return 1;
}

int IsTradeLogin(const json &root) {
    publisher->publish(CHANNEL_TRADE_DATA + "IsTradeLogin", ntos(trade_login));
    return 1;
}

int MarketReqUserLogin(const json &root) {
    CThostFtdcReqUserLoginField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.UserID, INVESTOR_ID.c_str());
    strcpy(req.Password, PASSWORD.c_str());
    iMarketRequestID = root["RequestID"].get<int>();
    return pMdApi->ReqUserLogin(&req, iMarketRequestID);
}

int SubscribeMarketData(const json &root) {
    auto **charArray = new char *[root.size()];
    for (unsigned int i = 0; i < root.size(); ++i) {
        charArray[i] = new char[32];
        strcpy(charArray[i], root[i].get<string>().c_str());
    }
    logger->info("订阅合约数量: %v", root.size());
    int iResult = pMdApi->SubscribeMarketData(charArray, (int)root.size());
    for (unsigned int i = 0; i < root.size(); ++i)
        delete charArray[i];
    delete[](charArray);
    return iResult;
}

int UnSubscribeMarketData(const json &root) {
    auto **charArray = new char *[root.size()];
    for (unsigned int i = 0; i < root.size(); ++i) {
        charArray[i] = new char[32];
        strcpy(charArray[i], root[i].get<string>().c_str());
    }
    int iResult = pMdApi->UnSubscribeMarketData(charArray, (int)root.size());
    for (unsigned int i = 0; i < root.size(); ++i)
        delete charArray[i];
    delete[](charArray);
    return iResult;
}

int ReqSettlementInfoConfirm(const json &root) {
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), root["RequestID"].get<int>());
    return pTraderApi->ReqSettlementInfoConfirm(req.get(), root["RequestID"].get<int>());
}

int TradeReqUserLogin(const json &root) {
    CThostFtdcReqUserLoginField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.UserID, INVESTOR_ID.c_str());
    strcpy(req.Password, PASSWORD.c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqUserLogin(&req, iTradeRequestID);
}

int ReqOrderInsert(const json &root) {
    CThostFtdcInputOrderField req{};
    try {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
        req.Direction = root["Direction"].get<string>()[0];
        strcpy(req.OrderRef, root["OrderRef"].get<string>().c_str());
        if (root["LimitPrice"].is_null()) {
            req.LimitPrice = 0;
        } else {
            req.LimitPrice = root["LimitPrice"].get<double>();
        }
        if (root["StopPrice"].is_null()) {
            req.StopPrice = 0;
        } else {
            req.StopPrice = root["StopPrice"].get<double>();
        }
        req.VolumeTotalOriginal = root["VolumeTotalOriginal"].get<int>();
        req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        strcpy(req.BrokerID, BROKER_ID.c_str());
        strcpy(req.InvestorID, INVESTOR_ID.c_str());
        req.CombOffsetFlag[0] = root["CombOffsetFlag"].get<string>()[0];
        req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
        req.VolumeCondition = THOST_FTDC_VC_AV;
        req.MinVolume = 1;
        req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        req.ContingentCondition = root["ContingentCondition"].get<string>()[0];
        req.IsAutoSuspend = 1;
        req.UserForceClose = 0;
        req.TimeCondition = root["TimeCondition"].get<string>()[0];
        strcpy(req.IPAddress, IP_ADDRESS.c_str());
        strcpy(req.MacAddress, MAC_ADDRESS.c_str());
        iTradeRequestID = root["RequestID"].get<int>();
        return pTraderApi->ReqOrderInsert(&req, iTradeRequestID);
    } catch (json::exception &e) {
        cout << "error: " << e.what() << endl;
        logger->error("ReqOrderInsert failed: %v", e.what());
        return -999;
    }
}

int ReqOrderAction(const json &root) {
    CThostFtdcInputOrderActionField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
//    strcpy(req.OrderRef, root["OrderRef"].get<string>().c_str());
    strcpy(req.ExchangeID, root["ExchangeID"].get<string>().c_str());
    strcpy(req.UserID, root["UserID"].get<string>().c_str());
    strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    req.OrderActionRef = root["RequestID"].get<int>() + 1;
//    req.FrontID = FRONT_ID;
//    req.SessionID = SESSION_ID;
    req.ActionFlag = THOST_FTDC_AF_Delete;
    strcpy(req.IPAddress, IP_ADDRESS.c_str());
    strcpy(req.MacAddress, MAC_ADDRESS.c_str());
    strcpy(req.OrderSysID, root["OrderSysID"].get<string>().c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqOrderAction(&req, iTradeRequestID);
}

int ReqQrySettlementInfo(const json &root) {
    CThostFtdcQrySettlementInfoField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQrySettlementInfo(&req, iTradeRequestID);
}

int ReqQryInstrument(const json &root) {
    CThostFtdcQryInstrumentField req{};
    if ( root.contains("InstrumentID") ) {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    }
    if ( root.contains("ExchangeID") ) {
        strcpy(req.ExchangeID, root["ExchangeID"].get<string>().c_str());
    }
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryInstrument(&req, iTradeRequestID);
}

int ReqQryInstrumentCommissionRate(const json &root) {
    CThostFtdcQryInstrumentCommissionRateField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if ( root.contains("InstrumentID") ) {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    }
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryInstrumentCommissionRate(&req, iTradeRequestID);
}

int ReqQryInstrumentMarginRate(const json &root) {
    CThostFtdcQryInstrumentMarginRateField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if ( root.contains("InstrumentID") ) {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    }
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryInstrumentMarginRate(&req, iTradeRequestID);
}

int ReqQryTradingAccount(const json &root) {
    CThostFtdcQryTradingAccountField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryTradingAccount(&req, iTradeRequestID);
}

int ReqQryInvestorPosition(const json &root) {
    CThostFtdcQryInvestorPositionField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if ( root.contains("InstrumentID") ) {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    }
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryInvestorPosition(&req, iTradeRequestID);
}

int ReqQryInvestorPositionDetail(const json &root) {
    CThostFtdcQryInvestorPositionDetailField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if ( root.contains("InstrumentID") ) {
        strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    }
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryInvestorPositionDetail(&req, iTradeRequestID);
}

int ReqQryOrder(const json &root) {
    CThostFtdcQryOrderField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryOrder(&req, iTradeRequestID);
}

int ReqQryTrade(const json &root) {
    CThostFtdcQryTradeField req{};
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    strcpy(req.InstrumentID, root["InstrumentID"].get<string>().c_str());
    strcpy(req.TradeID, root["TradeID"].get<string>().c_str());
    iTradeRequestID = root["RequestID"].get<int>();
    return pTraderApi->ReqQryTrade(&req, iTradeRequestID);
}

void handle_req_request(std::string pattern, std::string channel, std::string msg) {
    auto request_type = channel.substr(channel.find_last_of(':') + 1);
    json json_msg = json::parse(msg);
    cout << "req_msg: " << json_msg << endl;
    std::lock_guard<std::mutex> lk(mut);
    cmd_queue.emplace(request_type, json_msg);
    if ( request_type.find_first_of("Subscribe") != std::string::npos )
        logger->info("queue_size: %v, cmd: %v", cmd_queue.size(), request_type);
    else
        logger->info("queue_size: %v, cmd: %v, msg: %v", cmd_queue.size(), request_type, msg);
    check_cmd.notify_all();
}

void handle_command() {
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

    query_finished = true;
    auto start = std::chrono::high_resolution_clock::now();
    el::Helpers::setThreadName("command");
    logger->info("命令处理线程已启动.");
    while ( keep_running ) {
        if ( cmd_queue.empty() ) {
            std::unique_lock< std::mutex > lk( mut );
            bool wait_rst = check_cmd.wait_for(lk, std::chrono::seconds(1), [ start ] {
                // 没有新命令
                if ( cmd_queue.empty() )
                    return false;
                // 新命令是交易类命令， 直接执行
                if ( cmd_queue.front().first.find_first_of("ReqQry") == std::string::npos )
                    return true;
                // 上一次的查询命令执行完毕，可以执行新查询
                if ( query_finished )
                    return true;
                // 上一次的查询命令执行超时，可以执行新查询
                return std::chrono::high_resolution_clock::now() - start > std::chrono::seconds(30);
            });
            if ( ! wait_rst ) continue;
        }
        auto cmd_pair = cmd_queue.front();
        cmd_queue.pop();
        auto func = req_func.find(cmd_pair.first);
        if ( func == req_func.end() ) {
            logger->error("can't find req_func=%v", cmd_pair.second);
            continue;
        }
        // 查询类接口调用频率限制为1秒一次
        if ( cmd_pair.first.find_first_of("ReqQry") != std::string::npos ) {
            std::this_thread::sleep_until(start + std::chrono::milliseconds(1000));
            start = std::chrono::high_resolution_clock::now();
            query_finished = false;
        } else {
            query_finished = true;
        }
        if ( cmd_pair.first.find_first_of("Subscribe") != std::string::npos )
            logger->info("发送命令 %v", cmd_pair.first);
        else
            logger->info("发送命令 %v : %v", cmd_pair.first, cmd_pair.second);
        int iResult = (func->second)(cmd_pair.second);
        json err = "发送成功";
        if ( iResult == -1 )
            err = "因网络原因发送失败";
        else if ( iResult == -2 )
            err = "未处理请求队列总数量超限";
        else if ( iResult == -3 )
            err = "每秒发送请求数量超限";
        else if ( iResult == -999 )
            err = "发送失败";
        logger->info("结果: %v", err);
        if ( iResult > -999 && iResult < 0 ) {
            publisher->publish(CHANNEL_MARKET_DATA + "OnRspError:" + cmd_pair.second["RequestID"].get<string>(),
                               cmd_pair.second.dump());
        }
    }
    logger->info("监听线程已退出.");
}
