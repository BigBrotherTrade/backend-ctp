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
#include <iomanip>
#if defined(__linux__)
#include <fmt/core.h>
#elif defined(_WIN32)
#include <format>
#endif
#include <chrono>
#include "ThostFtdcTraderApi.h"
#include "TraderSpi.h"
#include "global.h"

using namespace std;
#if defined(__linux__)
using namespace fmt;
#endif
using namespace chrono;
using json = nlohmann::json;

void CTraderSpi::OnFrontConnected() {
    publisher->publish(format("{}OnFrontConnected", CHANNEL_TRADE_DATA), "OnFrontConnected");
    shared_ptr<CThostFtdcReqAuthenticateField> req(new CThostFtdcReqAuthenticateField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->UserID, INVESTOR_ID.c_str());
    strcpy(req->UserProductInfo, USERINFO.c_str());
    strcpy(req->AuthCode, AUTHCODE.c_str());
    strcpy(req->AppID, APPID.c_str());
    el::Helpers::setThreadName("trade");
    logger->info("交易前置已连接！发送交易终端认证请求..");
    pTraderApi->ReqAuthenticate(req.get(), 1);
}

void CTraderSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        char utf_str[512] = {0};
        gb2312toutf8(pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg), utf_str, sizeof(utf_str));
        logger->info("交易终端认证失败！err: %v", utf_str);
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct){
        root["empty"] = false;
        root["BrokerID"] = pStruct->BrokerID;
        root["UserID"] = pStruct->UserID;
        char utf_str[2048] = {0};
        gb2312toutf8(pStruct->UserProductInfo, sizeof(pStruct->UserProductInfo), utf_str, sizeof(utf_str));
        root["UserProductInfo"] = utf_str;
        root["AppID"] = pStruct->AppID;
        root["AppType"] = pStruct->AppType;
        logger->info("交易终端认证成功！发送交易登录请求..");

        shared_ptr<CThostFtdcReqUserLoginField> req(new CThostFtdcReqUserLoginField);
        strcpy(req->BrokerID, BROKER_ID.c_str());
        strcpy(req->UserID, INVESTOR_ID.c_str());
        strcpy(req->Password, PASSWORD.c_str());
        pTraderApi->ReqUserLogin(req.get(), 1);
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspAuthenticate:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["TradingDay"] = pStruct->TradingDay;
        root["LoginTime"] = pStruct->LoginTime;
        root["BrokerID"] = pStruct->BrokerID;
        root["UserID"] = pStruct->UserID;
        root["SystemName"] = pStruct->SystemName;
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["MaxOrderRef"] = pStruct->MaxOrderRef;
        root["SHFETime"] = pStruct->SHFETime;
        root["DCETime"] = pStruct->DCETime;
        root["CZCETime"] = pStruct->CZCETime;
        root["FFEXTime"] = pStruct->FFEXTime;
        root["INETime"] = pStruct->INETime;
        // 保存会话参数
        FRONT_ID = pStruct->FrontID;
        SESSION_ID = pStruct->SessionID;
        trade_login = true;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspUserLogin:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    auto trading_day = root["TradingDay"].get<string>();
    if ( !trading_day.empty() ) {
        string last_day1, last_day2;
        auto c1 = publisher->get("TradingDay");
        if ( c1 )
            last_day1 = *c1;
        auto c2 = publisher->get("LastTradingDay");
        if ( c2 )
            last_day2 = *c2;
        if (trading_day != last_day1) {
            publisher->set("TradingDay", trading_day);
            if ( last_day2.empty() )
                publisher->set("LastTradingDay", trading_day);
            else
                publisher->set("LastTradingDay", last_day1);
        }
    }
    logger->info("交易前置登录成功！账号：%v AppID：%v sessionid：%v 交易日：%v", pStruct->UserID, APPID, pStruct->SessionID, pStruct->TradingDay);
    logger->info("直接确认结算单...");
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), nRequestID + 1);
}

void CTraderSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["ConfirmDate"] = pStruct->ConfirmDate;
        root["ConfirmTime"] = pStruct->ConfirmTime;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQrySettlementInfoConfirm:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    // ConfirmDate = "20160803"
    long confirm_hours = 24;
    if (pStruct) {
#if defined(__linux__)
        string confirmDate(pStruct->ConfirmDate);
        string confirmTime(pStruct->ConfirmTime);
        tm tm = {};
        stringstream ss(confirmDate + confirmTime);
        ss >> get_time(&tm, "%Y%m%d%H:%M:%S");
        auto confirm_date = system_clock::from_time_t(mktime(&tm));
        logger->info("上次结算单确认时间=%v", put_time(&tm, "%F %T"));
        auto now = system_clock::now();
        confirm_hours = duration_cast<hours>(now - confirm_date).count();
#else
        stringstream in;
        in << pStruct->ConfirmDate << pStruct->ConfirmTime << "+0800";
        sys_seconds sys_confirm_date;
        from_stream(in, "%F%T%z", sys_confirm_date);
        zoned_time confirm_date(current_zone(), sys_confirm_date);
        logger->info(format("上次结算单确认时间: {{0:%F %T}}", confirm_date));
        zoned_time now(current_zone(), floor<seconds>(system_clock::now()));
        confirm_hours = duration_cast<hours>(now.get_sys_time() - confirm_date.get_sys_time()).count();
#endif
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    if (confirm_hours >= 24) {
        logger->info("查询结算单..");
        shared_ptr<CThostFtdcQrySettlementInfoField> req(new CThostFtdcQrySettlementInfoField);
        strcpy(req->BrokerID, BROKER_ID.c_str());
        strcpy(req->InvestorID, INVESTOR_ID.c_str());
        strcpy(req->TradingDay, "");
        pTraderApi->ReqQrySettlementInfo(req.get(), nRequestID + 1);
    }
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["SequenceNo"] = pStruct->SequenceNo;
        char utf_str[512] = {0};
        gb2312toutf8(pStruct->Content, sizeof(pStruct->Content), utf_str, sizeof(utf_str));
        root["Content"] = utf_str;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQrySettlementInfo:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    logger->info("确认结算单。");
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), nRequestID + 1);
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["ConfirmDate"] = pStruct->ConfirmDate;
        root["ConfirmTime"] = pStruct->ConfirmTime;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspSettlementInfoConfirm:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    logger->info("结算单确认成功！");
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["ExchangeID"] = pStruct->ExchangeID;
        char inst_name[64] = {0};
        gb2312toutf8(pStruct->InstrumentName, sizeof(pStruct->InstrumentName), inst_name, 64);
        root["InstrumentName"] = inst_name;
        root["ExchangeInstID"] = pStruct->ExchangeInstID;
        root["ProductID"] = pStruct->ProductID;
        root["ProductClass"] = pStruct->ProductClass;
        root["DeliveryYear"] = pStruct->DeliveryYear;
        root["DeliveryMonth"] = pStruct->DeliveryMonth;
        root["MaxMarketOrderVolume"] = pStruct->MaxMarketOrderVolume;
        root["MinMarketOrderVolume"] = pStruct->MinMarketOrderVolume;
        root["MaxLimitOrderVolume"] = pStruct->MaxLimitOrderVolume;
        root["MinLimitOrderVolume"] = pStruct->MinLimitOrderVolume;
        root["VolumeMultiple"] = pStruct->VolumeMultiple;
        root["PriceTick"] = pStruct->PriceTick;
        root["CreateDate"] = pStruct->CreateDate;
        root["OpenDate"] = pStruct->OpenDate;
        root["ExpireDate"] = pStruct->ExpireDate;
        root["StartDelivDate"] = pStruct->StartDelivDate;
        root["EndDelivDate"] = pStruct->EndDelivDate;
        root["InstLifePhase"] = pStruct->InstLifePhase;
        root["IsTrading"] = pStruct->IsTrading;
        root["PositionType"] = pStruct->PositionType;
        root["PositionDateType"] = pStruct->PositionDateType;
        root["LongMarginRatio"] = pStruct->LongMarginRatio;
        root["ShortMarginRatio"] = pStruct->ShortMarginRatio;
        root["MaxMarginSideAlgorithm"] = pStruct->MaxMarginSideAlgorithm;
        root["UnderlyingInstrID"] = pStruct->UnderlyingInstrID;
        root["StrikePrice"] = pStruct->StrikePrice;
        root["OptionsType"] = pStruct->OptionsType;
        root["UnderlyingMultiple"] = pStruct->UnderlyingMultiple;
        root["CombinationType"] = pStruct->CombinationType;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryInstrument:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["InvestorRange"] = pStruct->InvestorRange;
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["LongMarginRatioByMoney"] = pStruct->LongMarginRatioByMoney;
        root["LongMarginRatioByVolume"] = pStruct->LongMarginRatioByVolume;
        root["ShortMarginRatioByMoney"] = pStruct->ShortMarginRatioByMoney;
        root["ShortMarginRatioByVolume"] = pStruct->ShortMarginRatioByVolume;
        root["IsRelative"] = pStruct->IsRelative;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryInstrumentMarginRate:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["InvestorRange"] = pStruct->InvestorRange;
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["OpenRatioByMoney"] = pStruct->OpenRatioByMoney;
        root["OpenRatioByVolume"] = pStruct->OpenRatioByVolume;
        root["CloseRatioByMoney"] = pStruct->CloseRatioByMoney;
        root["CloseRatioByVolume"] = pStruct->CloseRatioByVolume;
        root["CloseTodayRatioByMoney"] = pStruct->CloseTodayRatioByMoney;
        root["CloseTodayRatioByVolume"] = pStruct->CloseTodayRatioByVolume;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryInstrumentCommissionRate:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["TradingDay"] = pStruct->TradingDay;
        root["InstrumentID"] = pStruct->InstrumentID;
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
        root["UpdateMillisec"] = pStruct->UpdateMillisec;
        root["BidPrice1"] = pStruct->BidPrice1;
        root["BidVolume1"] = pStruct->BidVolume1;
        root["AskPrice1"] = pStruct->AskPrice1;
        root["AskVolume1"] = pStruct->AskVolume1;
        root["AveragePrice"] = pStruct->AveragePrice;
        root["ActionDay"] = pStruct->ActionDay;
        root["UpdateTime"] = pStruct->UpdateTime;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryDepthMarketData:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["AccountID"] = pStruct->AccountID;
        root["PreMortgage"] = pStruct->PreMortgage;
        root["PreCredit"] = pStruct->PreCredit;
        root["PreDeposit"] = pStruct->PreDeposit;
        root["PreBalance"] = pStruct->PreBalance;
        root["PreMargin"] = pStruct->PreMargin;
        root["InterestBase"] = pStruct->InterestBase;
        root["Interest"] = pStruct->Interest;
        root["Deposit"] = pStruct->Deposit;
        root["Withdraw"] = pStruct->Withdraw;
        root["FrozenMargin"] = pStruct->FrozenMargin;
        root["FrozenCash"] = pStruct->FrozenCash;
        root["FrozenCommission"] = pStruct->FrozenCommission;
        root["CurrMargin"] = pStruct->CurrMargin;
        root["CashIn"] = pStruct->CashIn;
        root["Commission"] = pStruct->Commission;
        root["CloseProfit"] = pStruct->CloseProfit;
        root["PositionProfit"] = pStruct->PositionProfit;
        root["Balance"] = pStruct->Balance;
        root["Available"] = pStruct->Available;
        root["WithdrawQuota"] = pStruct->WithdrawQuota;
        root["Reserve"] = pStruct->Reserve;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["Credit"] = pStruct->Credit;
        root["Mortgage"] = pStruct->Mortgage;
        root["ExchangeMargin"] = pStruct->ExchangeMargin;
        root["DeliveryMargin"] = pStruct->DeliveryMargin;
        root["ExchangeDeliveryMargin"] = pStruct->ExchangeDeliveryMargin;
        root["ReserveBalance"] = pStruct->ReserveBalance;
        root["CurrencyID"] = pStruct->CurrencyID;
        root["PreFundMortgageIn"] = pStruct->PreFundMortgageIn;
        root["PreFundMortgageOut"] = pStruct->PreFundMortgageOut;
        root["FundMortgageIn"] = pStruct->FundMortgageIn;
        root["FundMortgageOut"] = pStruct->FundMortgageOut;
        root["FundMortgageAvailable"] = pStruct->FundMortgageAvailable;
        root["MortgageableFund"] = pStruct->MortgageableFund;
        root["SpecProductMargin"] = pStruct->SpecProductMargin;
        root["SpecProductFrozenMargin"] = pStruct->SpecProductFrozenMargin;
        root["SpecProductCommission"] = pStruct->SpecProductCommission;
        root["SpecProductFrozenCommission"] = pStruct->SpecProductFrozenCommission;
        root["SpecProductPositionProfit"] = pStruct->SpecProductPositionProfit;
        root["SpecProductCloseProfit"] = pStruct->SpecProductCloseProfit;
        root["SpecProductPositionProfitByAlg"] = pStruct->SpecProductPositionProfitByAlg;
        root["SpecProductExchangeMargin"] = pStruct->SpecProductExchangeMargin;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryTradingAccount:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["PosiDirection"] = pStruct->PosiDirection;
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["PositionDate"] = pStruct->PositionDate;
        root["YdPosition"] = pStruct->YdPosition;
        root["Position"] = pStruct->Position;
        root["LongFrozen"] = pStruct->LongFrozen;
        root["ShortFrozen"] = pStruct->ShortFrozen;
        root["LongFrozenAmount"] = pStruct->LongFrozenAmount;
        root["ShortFrozenAmount"] = pStruct->ShortFrozenAmount;
        root["OpenVolume"] = pStruct->OpenVolume;
        root["CloseVolume"] = pStruct->CloseVolume;
        root["OpenAmount"] = pStruct->OpenAmount;
        root["CloseAmount"] = pStruct->CloseAmount;
        root["PositionCost"] = pStruct->PositionCost;
        root["PreMargin"] = pStruct->PreMargin;
        root["UseMargin"] = pStruct->UseMargin;
        root["FrozenMargin"] = pStruct->FrozenMargin;
        root["FrozenCash"] = pStruct->FrozenCash;
        root["FrozenCommission"] = pStruct->FrozenCommission;
        root["CashIn"] = pStruct->CashIn;
        root["Commission"] = pStruct->Commission;
        root["CloseProfit"] = pStruct->CloseProfit;
        root["PositionProfit"] = pStruct->PositionProfit;
        root["PreSettlementPrice"] = pStruct->PreSettlementPrice;
        root["SettlementPrice"] = pStruct->SettlementPrice;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["OpenCost"] = pStruct->OpenCost;
        root["ExchangeMargin"] = pStruct->ExchangeMargin;
        root["CombPosition"] = pStruct->CombPosition;
        root["CombLongFrozen"] = pStruct->CombLongFrozen;
        root["CombShortFrozen"] = pStruct->CombShortFrozen;
        root["CloseProfitByDate"] = pStruct->CloseProfitByDate;
        root["CloseProfitByTrade"] = pStruct->CloseProfitByTrade;
        root["TodayPosition"] = pStruct->TodayPosition;
        root["MarginRateByMoney"] = pStruct->MarginRateByMoney;
        root["MarginRateByVolume"] = pStruct->MarginRateByVolume;
        root["StrikeFrozen"] = pStruct->StrikeFrozen;
        root["StrikeFrozenAmount"] = pStruct->StrikeFrozenAmount;
        root["AbandonFrozen"] = pStruct->AbandonFrozen;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryInvestorPosition:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["Direction"] = pStruct->Direction;
        root["OpenDate"] = pStruct->OpenDate;
        root["TradeID"] = pStruct->TradeID;
        root["Volume"] = pStruct->Volume;
        root["OpenPrice"] = pStruct->OpenPrice;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["TradeType"] = pStruct->TradeType;
        root["CombInstrumentID"] = pStruct->CombInstrumentID;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["CloseProfitByDate"] = pStruct->CloseProfitByDate;
        root["CloseProfitByTrade"] = pStruct->CloseProfitByTrade;
        root["PositionProfitByDate"] = pStruct->PositionProfitByDate;
        root["PositionProfitByTrade"] = pStruct->PositionProfitByTrade;
        root["Margin"] = pStruct->Margin;
        root["ExchMargin"] = pStruct->ExchMargin;
        root["MarginRateByMoney"] = pStruct->MarginRateByMoney;
        root["MarginRateByVolume"] = pStruct->MarginRateByVolume;
        root["LastSettlementPrice"] = pStruct->LastSettlementPrice;
        root["SettlementPrice"] = pStruct->SettlementPrice;
        root["CloseVolume"] = pStruct->CloseVolume;
        root["CloseAmount"] = pStruct->CloseAmount;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryInvestorPositionDetail:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["InstrumentID"] = pStruct->InstrumentID;
        root["OrderRef"] = pStruct->OrderRef;
        root["UserID"] = pStruct->UserID;
        root["OrderPriceType"] = pStruct->OrderPriceType;
        root["Direction"] = pStruct->Direction;
        root["CombOffsetFlag"] = pStruct->CombOffsetFlag;
        root["CombHedgeFlag"] = pStruct->CombHedgeFlag;
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
        root["TimeCondition"] = pStruct->TimeCondition;
        root["GTDDate"] = pStruct->GTDDate;
        root["VolumeCondition"] = pStruct->VolumeCondition;
        root["MinVolume"] = pStruct->MinVolume;
        root["ContingentCondition"] = pStruct->ContingentCondition;
        root["StopPrice"] = pStruct->StopPrice;
        root["ForceCloseReason"] = pStruct->ForceCloseReason;
        root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
        root["BusinessUnit"] = pStruct->BusinessUnit;
        root["RequestID"] = pStruct->RequestID;
        root["UserForceClose"] = pStruct->UserForceClose;
        root["IsSwapOrder"] = pStruct->IsSwapOrder;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["InvestUnitID"] = pStruct->InvestUnitID;
        root["AccountID"] = pStruct->AccountID;
        root["CurrencyID"] = pStruct->CurrencyID;
        root["ClientID"] = pStruct->ClientID;
        root["IPAddress"] = pStruct->IPAddress;
        root["MacAddress"] = pStruct->MacAddress;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRspOrderInsert:{}", CHANNEL_TRADE_DATA, nRequestID), msg_str);
    logger->info("OnRspOrderInsert：%v", msg_str);
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["OrderActionRef"] = pStruct->OrderActionRef;
        root["OrderRef"] = pStruct->OrderRef;
        root["RequestID"] = pStruct->RequestID;
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["OrderSysID"] = pStruct->OrderSysID;
        root["ActionFlag"] = pStruct->ActionFlag;
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeChange"] = pStruct->VolumeChange;
        root["UserID"] = pStruct->UserID;
        root["InstrumentID"] = pStruct->InstrumentID;
        root["InvestUnitID"] = pStruct->InvestUnitID;
        root["IPAddress"] = pStruct->IPAddress;
        root["MacAddress"] = pStruct->MacAddress;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRspOrderAction:{}", CHANNEL_TRADE_DATA, nRequestID), msg_str);
    logger->info("OnRspOrderAction：%v", msg_str);
}

void CTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["OrderRef"] = pStruct->OrderRef;
        root["UserID"] = pStruct->UserID;
        root["OrderPriceType"] = pStruct->OrderPriceType;
        root["Direction"] = pStruct->Direction;
        root["CombOffsetFlag"] = pStruct->CombOffsetFlag;
        root["CombHedgeFlag"] = pStruct->CombHedgeFlag;
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
        root["TimeCondition"] = pStruct->TimeCondition;
        root["GTDDate"] = pStruct->GTDDate;
        root["VolumeCondition"] = pStruct->VolumeCondition;
        root["MinVolume"] = pStruct->MinVolume;
        root["ContingentCondition"] = pStruct->ContingentCondition;
        root["StopPrice"] = pStruct->StopPrice;
        root["ForceCloseReason"] = pStruct->ForceCloseReason;
        root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
        root["BusinessUnit"] = pStruct->BusinessUnit;
        root["RequestID"] = pStruct->RequestID;
        root["OrderLocalID"] = pStruct->OrderLocalID;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["ParticipantID"] = pStruct->ParticipantID;
        root["ClientID"] = pStruct->ClientID;
        root["ExchangeInstID"] = pStruct->ExchangeInstID;
        root["TraderID"] = pStruct->TraderID;
        root["InstallID"] = pStruct->InstallID;
        root["OrderSubmitStatus"] = pStruct->OrderSubmitStatus;
        root["NotifySequence"] = pStruct->NotifySequence;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["OrderSysID"] = pStruct->OrderSysID;
        root["OrderSource"] = pStruct->OrderSource;
        root["OrderStatus"] = pStruct->OrderStatus;
        root["OrderType"] = pStruct->OrderType;
        root["VolumeTraded"] = pStruct->VolumeTraded;
        root["VolumeTotal"] = pStruct->VolumeTotal;
        root["InsertDate"] = pStruct->InsertDate;
        root["InsertTime"] = pStruct->InsertTime;
        root["ActiveTime"] = pStruct->ActiveTime;
        root["SuspendTime"] = pStruct->SuspendTime;
        root["UpdateTime"] = pStruct->UpdateTime;
        root["CancelTime"] = pStruct->CancelTime;
        root["ActiveTraderID"] = pStruct->ActiveTraderID;
        root["ClearingPartID"] = pStruct->ClearingPartID;
        root["SequenceNo"] = pStruct->SequenceNo;
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["UserProductInfo"] = pStruct->UserProductInfo;
        char utf_str[512] = {0};
        gb2312toutf8(pStruct->StatusMsg, sizeof(pStruct->StatusMsg), utf_str, sizeof(utf_str));
        root["StatusMsg"] = utf_str;
        root["UserForceClose"] = pStruct->UserForceClose;
        root["ActiveUserID"] = pStruct->ActiveUserID;
        root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
        root["RelativeOrderSysID"] = pStruct->RelativeOrderSysID;
        root["ZCETotalTradedVolume"] = pStruct->ZCETotalTradedVolume;
        root["IsSwapOrder"] = pStruct->IsSwapOrder;
        root["BranchID"] = pStruct->BranchID;
        root["InvestUnitID"] = pStruct->InvestUnitID;
        root["AccountID"] = pStruct->AccountID;
        root["CurrencyID"] = pStruct->CurrencyID;
        root["IPAddress"] = pStruct->IPAddress;
        root["MacAddress"] = pStruct->MacAddress;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRspQryOrder:{}", CHANNEL_TRADE_DATA, nRequestID), msg_str);
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

void CTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["InstrumentID"] = pStruct->InstrumentID;
        root["OrderRef"] = pStruct->OrderRef;
        root["UserID"] = pStruct->UserID;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["TradeID"] = pStruct->TradeID;
        root["Direction"] = pStruct->Direction;
        root["OrderSysID"] = pStruct->OrderSysID;
        root["ParticipantID"] = pStruct->ParticipantID;
        root["ClientID"] = pStruct->ClientID;
        root["TradingRole"] = pStruct->TradingRole;
        root["ExchangeInstID"] = pStruct->ExchangeInstID;
        root["OffsetFlag"] = pStruct->OffsetFlag;
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["Price"] = pStruct->Price;
        root["Volume"] = pStruct->Volume;
        root["TradeDate"] = pStruct->TradeDate;
        root["TradeTime"] = pStruct->TradeTime;
        root["TradeType"] = pStruct->TradeType;
        root["PriceSource"] = pStruct->PriceSource;
        root["TraderID"] = pStruct->TraderID;
        root["OrderLocalID"] = pStruct->OrderLocalID;
        root["ClearingPartID"] = pStruct->ClearingPartID;
        root["BusinessUnit"] = pStruct->BusinessUnit;
        root["SequenceNo"] = pStruct->SequenceNo;
        root["TradingDay"] = pStruct->TradingDay;
        root["SettlementID"] = pStruct->SettlementID;
        root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
        root["TradeSource"] = pStruct->TradeSource;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    publisher->publish(format("{}OnRspQryTrade:{}", CHANNEL_TRADE_DATA, nRequestID), root.dump());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        getCond().notify_all();
    }
}

///报单通知
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pStruct) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["BrokerID"] = pStruct->BrokerID;
    root["InvestorID"] = pStruct->InvestorID;
    root["InstrumentID"] = pStruct->InstrumentID;
    root["OrderRef"] = pStruct->OrderRef;
    root["UserID"] = pStruct->UserID;
    root["OrderPriceType"] = pStruct->OrderPriceType;
    root["Direction"] = pStruct->Direction;
    root["CombOffsetFlag"] = pStruct->CombOffsetFlag;
    root["CombHedgeFlag"] = pStruct->CombHedgeFlag;
    root["LimitPrice"] = pStruct->LimitPrice;
    root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
    root["TimeCondition"] = pStruct->TimeCondition;
    root["GTDDate"] = pStruct->GTDDate;
    root["VolumeCondition"] = pStruct->VolumeCondition;
    root["MinVolume"] = pStruct->MinVolume;
    root["ContingentCondition"] = pStruct->ContingentCondition;
    root["StopPrice"] = pStruct->StopPrice;
    root["ForceCloseReason"] = pStruct->ForceCloseReason;
    root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
    root["BusinessUnit"] = pStruct->BusinessUnit;
    root["RequestID"] = pStruct->RequestID;
    root["OrderLocalID"] = pStruct->OrderLocalID;
    root["ExchangeID"] = pStruct->ExchangeID;
    root["ParticipantID"] = pStruct->ParticipantID;
    root["ClientID"] = pStruct->ClientID;
    root["ExchangeInstID"] = pStruct->ExchangeInstID;
    root["TraderID"] = pStruct->TraderID;
    root["InstallID"] = pStruct->InstallID;
    root["OrderSubmitStatus"] = pStruct->OrderSubmitStatus;
    root["NotifySequence"] = pStruct->NotifySequence;
    root["TradingDay"] = pStruct->TradingDay;
    root["SettlementID"] = pStruct->SettlementID;
    root["OrderSysID"] = pStruct->OrderSysID;
    root["OrderSource"] = pStruct->OrderSource;
    root["OrderStatus"] = pStruct->OrderStatus;
    root["OrderType"] = pStruct->OrderType;
    root["VolumeTraded"] = pStruct->VolumeTraded;
    root["VolumeTotal"] = pStruct->VolumeTotal;
    root["InsertDate"] = pStruct->InsertDate;
    root["InsertTime"] = pStruct->InsertTime;
    root["ActiveTime"] = pStruct->ActiveTime;
    root["SuspendTime"] = pStruct->SuspendTime;
    root["UpdateTime"] = pStruct->UpdateTime;
    root["CancelTime"] = pStruct->CancelTime;
    root["ActiveTraderID"] = pStruct->ActiveTraderID;
    root["ClearingPartID"] = pStruct->ClearingPartID;
    root["SequenceNo"] = pStruct->SequenceNo;
    root["FrontID"] = pStruct->FrontID;
    root["SessionID"] = pStruct->SessionID;
    root["UserProductInfo"] = pStruct->UserProductInfo;
    char utf_str[512] = {0};
    gb2312toutf8(pStruct->StatusMsg, sizeof(pStruct->StatusMsg), utf_str, sizeof(utf_str));
    root["StatusMsg"] = utf_str;
    root["UserForceClose"] = pStruct->UserForceClose;
    root["ActiveUserID"] = pStruct->ActiveUserID;
    root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
    root["RelativeOrderSysID"] = pStruct->RelativeOrderSysID;
    root["ZCETotalTradedVolume"] = pStruct->ZCETotalTradedVolume;
    root["IsSwapOrder"] = pStruct->IsSwapOrder;
    root["BranchID"] = pStruct->BranchID;
    root["InvestUnitID"] = pStruct->InvestUnitID;
    root["AccountID"] = pStruct->AccountID;
    root["CurrencyID"] = pStruct->CurrencyID;
    root["IPAddress"] = pStruct->IPAddress;
    root["MacAddress"] = pStruct->MacAddress;
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRtnOrder:{}", CHANNEL_TRADE_DATA, root["OrderRef"].get<string>()), msg_str);
    logger->info("OnRtnOrder：%v", msg_str);
}

///成交通知
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pStruct) {
    json root;
    root["AppID"] = APPID;
    root["SessionID"] = SESSION_ID;
    root["BrokerID"] = pStruct->BrokerID;
    root["InvestorID"] = pStruct->InvestorID;
    root["InstrumentID"] = pStruct->InstrumentID;
    root["OrderRef"] = pStruct->OrderRef;
    root["UserID"] = pStruct->UserID;
    root["ExchangeID"] = pStruct->ExchangeID;
    root["TradeID"] = pStruct->TradeID;
    root["Direction"] = pStruct->Direction;
    root["OrderSysID"] = pStruct->OrderSysID;
    root["ParticipantID"] = pStruct->ParticipantID;
    root["ClientID"] = pStruct->ClientID;
    root["TradingRole"] = pStruct->TradingRole;
    root["ExchangeInstID"] = pStruct->ExchangeInstID;
    root["OffsetFlag"] = pStruct->OffsetFlag;
    root["HedgeFlag"] = pStruct->HedgeFlag;
    root["Price"] = pStruct->Price;
    root["Volume"] = pStruct->Volume;
    root["TradeDate"] = pStruct->TradeDate;
    root["TradeTime"] = pStruct->TradeTime;
    root["TradeType"] = pStruct->TradeType;
    root["PriceSource"] = pStruct->PriceSource;
    root["TraderID"] = pStruct->TraderID;
    root["OrderLocalID"] = pStruct->OrderLocalID;
    root["ClearingPartID"] = pStruct->ClearingPartID;
    root["BusinessUnit"] = pStruct->BusinessUnit;
    root["SequenceNo"] = pStruct->SequenceNo;
    root["TradingDay"] = pStruct->TradingDay;
    root["SettlementID"] = pStruct->SettlementID;
    root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
    root["TradeSource"] = pStruct->TradeSource;
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRtnTrade:{}", CHANNEL_TRADE_DATA, root["OrderRef"].get<string>()), msg_str);
    logger->info("OnRtnTrade：%v", msg_str);
}

void CTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pStruct) {
    /*
     * {"EnterReason":"1","EnterTime":"15:00:00","ExchangeID":"CFFEX","ExchangeInstID":"IF",
     * "InstrumentID":"IF","InstrumentStatus":"6","SettlementGroupID":"00000001","TradingSegmentSN":99}
     */
    json root;
    root["ExchangeID"] = pStruct->ExchangeID;
    root["ExchangeInstID"] = pStruct->ExchangeInstID;
    root["SettlementGroupID"] = pStruct->SettlementGroupID;
    root["InstrumentID"] = pStruct->InstrumentID;
    root["InstrumentStatus"] = pStruct->InstrumentStatus;
    root["TradingSegmentSN"] = pStruct->TradingSegmentSN;
    root["EnterTime"] = pStruct->EnterTime;
    root["EnterReason"] = pStruct->EnterReason;
    publisher->publish(format("{}OnRtnInstrumentStatus:{}", CHANNEL_TRADE_DATA, root["InstrumentID"].get<string>()), root.dump());
}

void CTraderSpi::OnFrontDisconnected(int nReason) {
    publisher->publish(format("{}OnFrontDisconnected", CHANNEL_TRADE_DATA), format("{}", nReason));
    trade_login = false;
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher->publish(format("{}OnHeartBeatWarning", CHANNEL_TRADE_DATA), format("{}", nTimeLapse));
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    el::Helpers::setThreadName("trade");
    json root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    auto msg_str = root.dump();
    publisher->publish(format("{}OnRspError:{}", CHANNEL_TRADE_DATA, nRequestID), msg_str);
    query_finished = true;
    getCond().notify_all();
    logger->info("OnRspError：%v", msg_str);
}
