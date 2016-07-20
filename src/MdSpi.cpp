#include "MdSpi.h"
#include "global.h"

using namespace std;

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    publisher.publish(CHANNEL_MARKET_DATA+"OnRspError:" + ntos(nRequestID), writer.write(root));
}

void CMdSpi::OnFrontDisconnected(int nReason) {
    publisher.publish(CHANNEL_MARKET_DATA + "OnFrontDisconnected", ntos(nReason));
    market_login = false;
}

void CMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher.publish(CHANNEL_MARKET_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CMdSpi::OnFrontConnected() {
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
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspUserLogin", writer.write(root));
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
        root["InstrumentID"] = "InstrumentID", pSpecificInstrument->InstrumentID;
    }
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspSubMarketData", writer.write(root));
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
        root["InstrumentID"] = "InstrumentID", pSpecificInstrument->InstrumentID;
    }
    publisher.publish(CHANNEL_MARKET_DATA + "OnRspUnSubMarketData", writer.write(root));
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pData) {
    Json::Value root;
    root["TradingDay"] = pData->TradingDay;
    root["InstrumentID"] = pData->InstrumentID;
    root["ExchangeID"] = pData->ExchangeID;
    root["ExchangeInstID"] = pData->ExchangeInstID;
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
    root["PreDelta"] = pData->PreDelta;
    root["CurrDelta"] = pData->CurrDelta;
    root["UpdateMillisec"] = pData->UpdateMillisec;
    root["BidPrice1"] = pData->BidPrice1;
    root["BidVolume1"] = pData->BidVolume1;
    root["AskPrice1"] = pData->AskPrice1;
    root["AskVolume1"] = pData->AskVolume1;
    root["BidPrice2"] = pData->BidPrice2;
    root["BidVolume2"] = pData->BidVolume2;
    root["AskPrice2"] = pData->AskPrice2;
    root["AskVolume2"] = pData->AskVolume2;
    root["BidPrice3"] = pData->BidPrice3;
    root["BidVolume3"] = pData->BidVolume3;
    root["AskPrice3"] = pData->AskPrice3;
    root["AskVolume3"] = pData->AskVolume3;
    root["BidPrice4"] = pData->BidPrice4;
    root["BidVolume4"] = pData->BidVolume4;
    root["AskPrice4"] = pData->AskPrice4;
    root["AskVolume4"] = pData->AskVolume4;
    root["BidPrice5"] = pData->BidPrice5;
    root["BidVolume5"] = pData->BidVolume5;
    root["AskPrice5"] = pData->AskPrice5;
    root["AskVolume5"] = pData->AskVolume5;
    root["AveragePrice"] = pData->AveragePrice;
    root["ActionDay"] = pData->ActionDay;
    publisher.publish(CHANNEL_MARKET_DATA + "OnRtnDepthMarketData", writer.write(root));
}
