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
#include <iconv.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace redox;

string MAC_ADDRESS;
string IP_ADDRESS;
string BROKER_ID;		// 经纪公司代码
string INVESTOR_ID;		// 投资者代码
string PASSWORD ;		// 用户密码
TThostFtdcFrontIDType FRONT_ID;             // 前置编号
TThostFtdcSessionIDType SESSION_ID;         // 会话编号
CThostFtdcTraderApi* pTraderApi = 0;
CThostFtdcMdApi* pMdApi = 0;

std::atomic<int> iMarketRequestID(0);
std::atomic<int> iTradeRequestID(0);
std::atomic<bool> query_finished(true);
std::atomic<bool> keep_running(true);
std::atomic<bool> trade_login(false);
std::atomic<bool> market_login(false);
std::condition_variable check_cmd;
redox::Redox publisher;
redox::Subscriber subscriber;

struct RequestCommand {
    string cmd;
    Json::Value arg;
};

queue<RequestCommand> cmd_queue;
std::mutex mut;

map<string, boost::function<int(const Json::Value &)> > req_func;

int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen) {
    iconv_t cd;
    if ((cd = iconv_open("utf-8", "gb2312")) == 0)
        return -1;
    memset(destbuf, 0, destlen);
    char **source = &sourcebuf;
    char **dest = &destbuf;
    iconv(cd, source, &sourcelen, dest, &destlen);
    iconv_close(cd);
    return 0;
}

const std::string ntos(const int n) {
    std::stringstream newstr;
    newstr << n;
    return newstr.str();
}

int IsMarketLogin(const Json::Value &root) {
    publisher.publish(CHANNEL_MARKET_DATA + "IsMarketLogin", ntos(market_login));
    return 1;
}

int IsTradeLogin(const Json::Value &root) {
    publisher.publish(CHANNEL_TRADE_DATA + "IsTradeLogin", ntos(trade_login));
    return 1;
}

int MarketReqUserLogin(const Json::Value &root) {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    BROKER_ID = root["BrokerID"].asString();
    INVESTOR_ID = root["UserID"].asString();
    PASSWORD = root["Password"].asString();
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.UserID, INVESTOR_ID.c_str());
    strcpy(req.Password, PASSWORD.c_str());
    iMarketRequestID = root["RequestID"].asInt();
    return pMdApi->ReqUserLogin(&req, iMarketRequestID);
}

int SubscribeMarketData(const Json::Value &root) {
    char **charArray = new char *[root.size()];
    for (unsigned int i = 0; i < root.size(); ++i) {
        charArray[i] = new char[32];
        strcpy(charArray[i], root[i].asCString());
    }
    cout << "root.size=" << root.size() << endl;
    int iResult = pMdApi->SubscribeMarketData(charArray, root.size());
    for (unsigned int i = 0; i < root.size(); ++i)
        delete charArray[i];
    delete[](charArray);
    return iResult;
}

int UnSubscribeMarketData(const Json::Value &root) {
    char **charArray = new char *[root.size()];
    for (unsigned int i = 0; i < root.size(); ++i) {
        charArray[i] = new char[32];
        strcpy(charArray[i], root[i].asCString());
    }
    int iResult = pMdApi->UnSubscribeMarketData(charArray, root.size());
    for (unsigned int i = 0; i < root.size(); ++i)
        delete charArray[i];
    delete[](charArray);
    return iResult;
}

int ReqSettlementInfoConfirm(const Json::Value &root) {
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqSettlementInfoConfirm(&req, iTradeRequestID);
}

int TradeReqUserLogin(const Json::Value &root) {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    BROKER_ID = root["BrokerID"].asString();
    INVESTOR_ID = root["UserID"].asString();
    PASSWORD = root["Password"].asString();
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.UserID, INVESTOR_ID.c_str());
    strcpy(req.Password, PASSWORD.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqUserLogin(&req, iTradeRequestID);
}

int ReqOrderInsert(const Json::Value &root) {
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    req.Direction = root["Direction"].asString()[0];
    strcpy(req.OrderRef, root["OrderRef"].asCString());
    if (root["LimitPrice"].isNull()) {
        req.LimitPrice = 0;
    } else {
        req.LimitPrice = root["LimitPrice"].asDouble();
    }
    if (root["StopPrice"].isNull()) {
        req.StopPrice = 0;
    } else {
        req.StopPrice = root["StopPrice"].asDouble();
    }
    req.VolumeTotalOriginal = root["VolumeTotalOriginal"].asInt();
    req.OrderPriceType = root["OrderPriceType"].asString()[0];
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    req.CombOffsetFlag[0] = root["CombOffsetFlag"].asString()[0];
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    req.VolumeCondition = THOST_FTDC_VC_CV;
    req.MinVolume = 1;
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    req.ContingentCondition = root["ContingentCondition"].asString()[0];
    req.IsAutoSuspend = 1;
    req.UserForceClose = 0;
    req.TimeCondition = root["TimeCondition"].asString()[0];
    strcpy(req.IPAddress, IP_ADDRESS.c_str());
    strcpy(req.MacAddress, MAC_ADDRESS.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqOrderInsert(&req, iTradeRequestID);
}

int ReqOrderAction(const Json::Value &root) {
    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    strcpy(req.OrderRef, root["OrderRef"].asCString());
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    req.FrontID = FRONT_ID;
    req.SessionID = SESSION_ID;
    req.ActionFlag = THOST_FTDC_AF_Delete;
    strcpy(req.IPAddress, IP_ADDRESS.c_str());
    strcpy(req.MacAddress, MAC_ADDRESS.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqOrderAction(&req, iTradeRequestID);
}

int ReqQrySettlementInfo(const Json::Value &root) {
    CThostFtdcQrySettlementInfoField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQrySettlementInfo(&req, iTradeRequestID);
}

int ReqQryInstrument(const Json::Value &root) {
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    if (!root["InstrumentID"].isNull()) {
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    }
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryInstrument(&req, iTradeRequestID);
}

int ReqQryInstrumentCommissionRate(const Json::Value &root) {
    CThostFtdcQryInstrumentCommissionRateField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if (!root["InstrumentID"].isNull()) {
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    }
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryInstrumentCommissionRate(&req, iTradeRequestID);
}

int ReqQryInstrumentMarginRate(const Json::Value &root) {
    CThostFtdcQryInstrumentMarginRateField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if (!root["InstrumentID"].isNull()) {
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    }
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryInstrumentMarginRate(&req, iTradeRequestID);
}

int ReqQryTradingAccount(const Json::Value &root) {
    CThostFtdcQryTradingAccountField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryTradingAccount(&req, iTradeRequestID);
}

int ReqQryInvestorPosition(const Json::Value &root) {
    CThostFtdcQryInvestorPositionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if (!root["InstrumentID"].isNull()) {
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    }
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryInvestorPosition(&req, iTradeRequestID);
}

int ReqQryInvestorPositionDetail(const Json::Value &root) {
    CThostFtdcQryInvestorPositionDetailField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    if (!root["InstrumentID"].isNull()) {
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    }
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryInvestorPositionDetail(&req, iTradeRequestID);
}

int ReqQryOrder(const Json::Value &root) {
    CThostFtdcQryOrderField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryOrder(&req, iTradeRequestID);
}

int ReqQryTrade(const Json::Value &root) {
    CThostFtdcQryTradeField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    strcpy(req.InstrumentID, root["InstrumentID"].asCString());
    strcpy(req.TradeID, root["TradeID"].asCString());
    iTradeRequestID = root["RequestID"].asInt();
    return pTraderApi->ReqQryTrade(&req, iTradeRequestID);
}

void handle_req_request(const string &topic, const string &msg) {
    std::vector<std::string> strs;
    boost::split(strs, topic, boost::is_any_of(":"));
    auto request_type = strs.back();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg, root);
    std::lock_guard<std::mutex> lk(mut);
    RequestCommand rc;
    rc.cmd = request_type;
    rc.arg = root;
    cmd_queue.push(rc);
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

    auto start = std::chrono::high_resolution_clock::now();
    while (keep_running) {
        std::unique_lock<std::mutex> lk(mut);
        bool wait_rst = check_cmd.wait_for(lk, std::chrono::seconds(5), [start] {
            if (cmd_queue.empty())
                return false;
            if (query_finished)
                return true;
            return std::chrono::high_resolution_clock::now() - start > std::chrono::seconds(30);
        });
        if (!wait_rst) continue;
        RequestCommand cmd = cmd_queue.front();
        cmd_queue.pop();
        auto func = req_func.find(cmd.cmd);
        if (func == req_func.end()) {
            cout << "can't find req_func=" << cmd.cmd << endl;
            continue;
        }
        // 查询类接口调用频率限制为1秒一次
        if (boost::find_first(cmd.cmd, "ReqQry")) {
            std::this_thread::sleep_until(start + std::chrono::milliseconds(1000));
            query_finished = false;
            start = std::chrono::high_resolution_clock::now();
        }
        cout << "calling " << cmd.cmd << endl;
        int iResult = (func->second)(cmd.arg);
        cout << "rst=" << iResult << endl;
        if (iResult != 0) {
            Json::Value err = "failed";
            if (iResult == -2)
                err = "未处理请求超过许可数";
            else if (iResult == -3)
                err = "每秒发送请求数超过许可数";
            Json::FastWriter writer;
            publisher.publish(CHANNEL_MARKET_DATA + "OnRspError:" + cmd.arg["RequestID"].asString(),
                              writer.write(err));
        }
    }
}
