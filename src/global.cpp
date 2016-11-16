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

using namespace std;
using namespace redox;

string MAC_ADDRESS;
string IP_ADDRESS;
string BROKER_ID;		// 经纪公司代码
string INVESTOR_ID;		// 投资者代码
string PASSWORD;		// 用户密码
string AUTHCODE;		// 认证码
string USERPRODUCTINFO;	// 产品信息
TThostFtdcFrontIDType FRONT_ID;             // 前置编号
TThostFtdcSessionIDType SESSION_ID;         // 会话编号
CThostFtdcTraderApi* pTraderApi = 0;
CThostFtdcMdApi* pMdApi = 0;
el::Logger* logger;

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

map<string, std::function<int(const Json::Value &)> > req_func;

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
    logger->info("订阅合约数量: %v", root.size());
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
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), root["RequestID"].asInt());
    return pTraderApi->ReqSettlementInfoConfirm(req.get(), root["RequestID"].asInt());
}

int TradeReqUserLogin(const Json::Value &root) {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
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
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    strcpy(req.BrokerID, BROKER_ID.c_str());
    strcpy(req.InvestorID, INVESTOR_ID.c_str());
    req.CombOffsetFlag[0] = root["CombOffsetFlag"].asString()[0];
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    req.VolumeCondition = THOST_FTDC_VC_AV;
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
    auto request_type = topic.substr(topic.find_last_of(":") + 1);
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg, root);
    std::lock_guard<std::mutex> lk(mut);
    RequestCommand rc;
    rc.cmd = request_type;
    rc.arg = root;
    cmd_queue.push(rc);
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
    logger->info("命令处理线程已启动.");
    while ( keep_running ) {
        if ( cmd_queue.empty() ) {
            std::unique_lock< std::mutex > lk( mut );
            bool wait_rst = check_cmd.wait_for(lk, std::chrono::seconds(1), [ start ] {
                // 没有新命令
                if ( cmd_queue.empty() )
                    return false;
                // 新命令是交易类命令， 直接执行
                if ( cmd_queue.front().cmd.find_first_of("ReqQry") == std::string::npos )
                    return true;
                // 上一次的查询命令执行完毕，可以执行新查询
                if ( query_finished )
                    return true;
                // 上一次的查询命令执行超时，可以执行新查询
                return std::chrono::high_resolution_clock::now() - start > std::chrono::seconds(30);
            });
            if ( ! wait_rst ) continue;
        }
        RequestCommand cmd = cmd_queue.front();
        cmd_queue.pop();
        auto func = req_func.find(cmd.cmd);
        if ( func == req_func.end() ) {
            logger->error("can't find req_func=%v", cmd.cmd);
            continue;
        }
        // 查询类接口调用频率限制为1秒一次
        if ( cmd.cmd.find_first_of("ReqQry") != std::string::npos ) {
            std::this_thread::sleep_until(start + std::chrono::milliseconds(1000));
            start = std::chrono::high_resolution_clock::now();
            query_finished = false;
        } else {
            query_finished = true;
        }
        if ( cmd.cmd.find_first_of("Subscribe") != std::string::npos )
            logger->info("发送命令 %v", cmd.cmd);
        else
            logger->info("发送命令 %v : %v", cmd.cmd, cmd.arg);
        int iResult = (func->second)(cmd.arg);
        Json::Value err = "发送成功";
        if ( iResult == -1 )
            err = "因网络原因发送失败";
        else if ( iResult == -2 )
            err = "未处理请求队列总数量超限";
        else if ( iResult == -3 )
            err = "每秒发送请求数量超限";
        logger->info("结果: %v", err);
        if ( iResult ) {
            Json::FastWriter writer;
            publisher.publish(CHANNEL_MARKET_DATA + "OnRspError:" + cmd.arg["RequestID"].asString(),
                              writer.write(err));
        }
    }
    logger->info("监听线程已退出.");
}
