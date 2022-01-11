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
#include "MdSpi.h"
#include "global.h"

using namespace std;

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_MARKET_DATA + "OnRspError:" + ntos(nRequestID), sb.GetString());
}

void CMdSpi::OnFrontDisconnected(int nReason) {
    publisher->publish(CHANNEL_MARKET_DATA + "OnFrontDisconnected", ntos(nReason));
    market_login = false;
    el::Helpers::setThreadName("market");
}

void CMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher->publish(CHANNEL_MARKET_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CMdSpi::OnFrontConnected() {
    shared_ptr<CThostFtdcReqUserLoginField> req(new CThostFtdcReqUserLoginField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->UserID, INVESTOR_ID.c_str());
    strcpy(req->Password, PASSWORD.c_str());
    pMdApi->ReqUserLogin(req.get(), 1);
    publisher->publish(CHANNEL_MARKET_DATA + "OnFrontConnected", "OnFrontConnected");
    el::Helpers::setThreadName("market");
    logger->info("行情前置已连接！发送行情登录请求..");
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pStruct,
                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["LoginTime"].SetString(pStruct->LoginTime, sizeof (pStruct->LoginTime));
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["SystemName"].SetString(pStruct->SystemName, sizeof (pStruct->SystemName));
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["MaxOrderRef"].SetString(pStruct->MaxOrderRef, sizeof (pStruct->MaxOrderRef));
        root["SHFETime"].SetString(pStruct->SHFETime, sizeof (pStruct->SHFETime));
        root["DCETime"].SetString(pStruct->DCETime, sizeof (pStruct->DCETime));
        root["CZCETime"].SetString(pStruct->CZCETime, sizeof (pStruct->CZCETime));
        root["FFEXTime"].SetString(pStruct->FFEXTime, sizeof (pStruct->FFEXTime));
        root["INETime"].SetString(pStruct->INETime, sizeof (pStruct->INETime));
        market_login = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_MARKET_DATA + "OnRspUserLogin:" + ntos(nRequestID), sb.GetString());
    logger->info("行情登录成功！交易日：%v", pStruct->TradingDay);
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pStruct,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_MARKET_DATA + "OnRspSubMarketData:" + ntos(nRequestID), sb.GetString());
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pStruct,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_MARKET_DATA + "OnRspUnSubMarketData:" + ntos(nRequestID), sb.GetString());
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pStruct) {
    rapidjson::Document root;
    root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
    root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
//    root["ExchangeID"] = pStruct->ExchangeID;
//    root["ExchangeInstID"] = pStruct->ExchangeInstID;
    root["LastPrice"] = pStruct->LastPrice;
    root["PreSettlementPrice"] = pStruct->PreSettlementPrice;
    root["PreClosePrice"] = pStruct->PreClosePrice;
    root["PreOpenInterest"] = pStruct->PreOpenInterest;
    root["OpenPrice"] = pStruct->OpenPrice;
    root["HighestPrice"] = pStruct->HighestPrice;
    root["LowestPrice"] = pStruct->LowestPrice;
    root["Volume"] = pStruct->Volume;
    root["Turnover"] = pStruct->Turnover;
    root["OpenInterest"] = pStruct->OpenInterest;
    root["ClosePrice"] = pStruct->ClosePrice;
    root["SettlementPrice"] = pStruct->SettlementPrice;
    root["UpperLimitPrice"] = pStruct->UpperLimitPrice;
    root["LowerLimitPrice"] = pStruct->LowerLimitPrice;
//    root["PreDelta"] = pStruct->PreDelta;
//    root["CurrDelta"] = pStruct->CurrDelta;
    root["UpdateMillisec"] = pStruct->UpdateMillisec;
    root["BidPrice1"] = pStruct->BidPrice1;
    root["BidVolume1"] = pStruct->BidVolume1;
    root["AskPrice1"] = pStruct->AskPrice1;
    root["AskVolume1"] = pStruct->AskVolume1;
    root["AveragePrice"] = pStruct->AveragePrice;
    root["ActionDay"].SetString(pStruct->ActionDay, sizeof (pStruct->ActionDay));
    char time_str[32] = {0};
    sprintf(time_str, "%s %s:%d", pStruct->ActionDay, pStruct->UpdateTime, pStruct->UpdateMillisec * 1000);
    root["UpdateTime"].SetString(time_str, sizeof (time_str));
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    auto data_str = sb.GetString();
    publisher->publish(CHANNEL_MARKET_DATA + "OnRtnDepthMarketData:" + root["InstrumentID"].GetString(), data_str);
    publisher->set(HASHSET_TICK + root["InstrumentID"].GetString(), data_str);
}
