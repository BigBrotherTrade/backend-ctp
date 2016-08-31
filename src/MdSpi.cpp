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
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    Json::FastWriter writer;
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspError:" + ntos(nRequestID), writer.write(root));
}

void CMdSpi::OnFrontDisconnected(int nReason) {
    publisher.publish(CHANNEL_MARKET_DATA + "OnFrontDisconnected", ntos(nReason));
    market_login = false;
}

void CMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher.publish(CHANNEL_MARKET_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CMdSpi::OnFrontConnected() {
    shared_ptr<CThostFtdcReqUserLoginField> req(new CThostFtdcReqUserLoginField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->UserID, INVESTOR_ID.c_str());
    strcpy(req->Password, PASSWORD.c_str());
    pMdApi->ReqUserLogin(req.get(), 1);
    publisher.publish(CHANNEL_MARKET_DATA + "OnFrontConnected", "OnFrontConnected");
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["TradingDay"] = pRspUserLogin->TradingDay;
        root["LoginTime"] = pRspUserLogin->LoginTime;
        root["BrokerID"] = pRspUserLogin->BrokerID;
        root["UserID"] = pRspUserLogin->UserID;
        root["SystemName"] = pRspUserLogin->SystemName;
        root["FrontID"] = pRspUserLogin->FrontID;
        root["SessionID"] = pRspUserLogin->SessionID;
        root["MaxOrderRef"] = pRspUserLogin->MaxOrderRef;
        root["SHFETime"] = pRspUserLogin->SHFETime;
        root["DCETime"] = pRspUserLogin->DCETime;
        root["CZCETime"] = pRspUserLogin->CZCETime;
        root["FFEXTime"] = pRspUserLogin->FFEXTime;
        root["INETime"] = pRspUserLogin->INETime;
        market_login = true;
    }
    Json::FastWriter writer;
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspUserLogin:" + ntos(nRequestID), writer.write(root));
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["InstrumentID"] = pSpecificInstrument->InstrumentID;
    }
    Json::FastWriter writer;
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspSubMarketData:" + ntos(nRequestID), writer.write(root));
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["InstrumentID"] = pSpecificInstrument->InstrumentID;
    }
    Json::FastWriter writer;
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspUnSubMarketData:" + ntos(nRequestID), writer.write(root));
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pData) {
    Json::Value root;
    root["TradingDay"] = pData->TradingDay;
    root["InstrumentID"] = pData->InstrumentID;
//    root["ExchangeID"] = pData->ExchangeID;
//    root["ExchangeInstID"] = pData->ExchangeInstID;
    root["LastPrice"] = pData->LastPrice;
    root["PreSettlementPrice"] = pData->PreSettlementPrice;
    root["PreClosePrice"] = pData->PreClosePrice;
    root["PreOpenInterest"] = pData->PreOpenInterest;
    root["OpenPrice"] = pData->OpenPrice;
    root["HighestPrice"] = pData->HighestPrice;
    root["LowestPrice"] = pData->LowestPrice;
    root["Volume"] = pData->Volume;
    root["Turnover"] = pData->Turnover;
    root["OpenInterest"] = pData->OpenInterest;
    root["ClosePrice"] = pData->ClosePrice;
    root["SettlementPrice"] = pData->SettlementPrice;
    root["UpperLimitPrice"] = pData->UpperLimitPrice;
    root["LowerLimitPrice"] = pData->LowerLimitPrice;
//    root["PreDelta"] = pData->PreDelta;
//    root["CurrDelta"] = pData->CurrDelta;
    root["UpdateMillisec"] = pData->UpdateMillisec;
    root["BidPrice1"] = pData->BidPrice1;
    root["BidVolume1"] = pData->BidVolume1;
    root["AskPrice1"] = pData->AskPrice1;
    root["AskVolume1"] = pData->AskVolume1;
    root["AveragePrice"] = pData->AveragePrice;
    root["ActionDay"] = pData->ActionDay;
    char time_str[32] = {0};
    sprintf(time_str, "%s %s:%d", pData->ActionDay, pData->UpdateTime, pData->UpdateMillisec * 1000);
    root["UpdateTime"] = string(time_str);
    Json::FastWriter writer;
    auto data_str = writer.write(root);
    publisher.publish(CHANNEL_MARKET_DATA + "OnRtnDepthMarketData:" + root["InstrumentID"].asString(), data_str);
    publisher.set(root["InstrumentID"].asString(), data_str);
}
