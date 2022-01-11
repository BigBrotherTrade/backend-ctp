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
#include "ThostFtdcTraderApi.h"
#include "TraderSpi.h"
#include "global.h"

using namespace std;

void CTraderSpi::OnFrontConnected() {
    publisher->publish(CHANNEL_TRADE_DATA + "OnFrontConnected", "OnFrontConnected");
    shared_ptr<CThostFtdcReqAuthenticateField> req(new CThostFtdcReqAuthenticateField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->UserID, INVESTOR_ID.c_str());
    strcpy(req->AuthCode, AUTHCODE.c_str());
    strcpy(req->AppID, APPID.c_str());
    el::Helpers::setThreadName("trade");
    logger->info("交易前置已连接！发送交易终端认证请求..");
    pTraderApi->ReqAuthenticate(req.get(), 1);
}

void CTraderSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pStruct,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
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
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["AppID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspAuthenticate:" + ntos(nRequestID), sb.GetString());
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pStruct,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
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
        // 保存会话参数
        FRONT_ID = pStruct->FrontID;
        SESSION_ID = pStruct->SessionID;
        trade_login = true;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspUserLogin:" + ntos(nRequestID), sb.GetString());
    std::string trading_day = root["TradingDay"].GetString();
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
    logger->info("交易前置登录成功！账号：%v AppID：%v sessionid：%v 交易日：%v",
                 pStruct->UserID, APPID, pStruct->SessionID, pStruct->TradingDay);
    logger->info("直接确认结算单...");
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), nRequestID + 1);
//    shared_ptr<CThostFtdcQrySettlementInfoConfirmField> req(new CThostFtdcQrySettlementInfoConfirmField);
//    strcpy(req->BrokerID, BROKER_ID.c_str());
//    strcpy(req->InvestorID, INVESTOR_ID.c_str());
//    pTraderApi->ReqQrySettlementInfoConfirm(req.get(), nRequestID+1);
}

void CTraderSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["ConfirmDate"].SetString(pStruct->ConfirmDate, sizeof (pStruct->ConfirmDate));
        root["ConfirmTime"].SetString(pStruct->ConfirmTime, sizeof (pStruct->ConfirmTime));
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQrySettlementInfoConfirm:" + ntos(nRequestID), sb.GetString());
    // ConfirmDate = "20160803"
    long hours = 24;
    if (pStruct) {
        string confirmDate(pStruct->ConfirmDate);
        string confirmTime(pStruct->ConfirmTime);
        std::tm tm = {};
        std::stringstream ss(confirmDate + confirmTime);
        ss >> std::get_time(&tm, "%Y%m%d%H:%M:%S");
        auto confirm_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        logger->info("上次结算单确认时间=%v", std::put_time(&tm, "%F %T"));
        auto now = std::chrono::system_clock::now();
        hours = std::chrono::duration_cast<std::chrono::hours>(now - confirm_date).count();
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    if (hours >= 24) {
        logger->info("查询结算单..");
        shared_ptr<CThostFtdcQrySettlementInfoField> req(new CThostFtdcQrySettlementInfoField);
        strcpy(req->BrokerID, BROKER_ID.c_str());
        strcpy(req->InvestorID, INVESTOR_ID.c_str());
        strcpy(req->TradingDay, "");
        pTraderApi->ReqQrySettlementInfo(req.get(), nRequestID + 1);
    }
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["SettlementID"] = pStruct->SettlementID;
        root["SequenceNo"] = pStruct->SequenceNo;
        char utf_str[512] = {0};
        gb2312toutf8(pStruct->Content, sizeof(pStruct->Content), utf_str, sizeof(utf_str));
        root["Content"].SetString(utf_str, 512);
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQrySettlementInfo:" + ntos(nRequestID), sb.GetString());
    logger->info("确认结算单。");
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), nRequestID + 1);
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["ConfirmDate"].SetString(pStruct->ConfirmDate, sizeof (pStruct->ConfirmDate));
        root["ConfirmTime"].SetString(pStruct->ConfirmTime, sizeof (pStruct->ConfirmTime));
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspSettlementInfoConfirm:" + ntos(nRequestID), sb.GetString());
    logger->info("结算单确认成功！");
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
        char inst_name[64] = {0};
        gb2312toutf8(pStruct->InstrumentName, sizeof(pStruct->InstrumentName), inst_name, 64);
        root["InstrumentName"].SetString(inst_name, sizeof (inst_name));
        root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
        root["ProductID"].SetString(pStruct->ProductID, sizeof (pStruct->ProductID));
        root["ProductClass"] = pStruct->ProductClass;
        root["DeliveryYear"] = pStruct->DeliveryYear;
        root["DeliveryMonth"] = pStruct->DeliveryMonth;
        root["MaxMarketOrderVolume"] = pStruct->MaxMarketOrderVolume;
        root["MinMarketOrderVolume"] = pStruct->MinMarketOrderVolume;
        root["MaxLimitOrderVolume"] = pStruct->MaxLimitOrderVolume;
        root["MinLimitOrderVolume"] = pStruct->MinLimitOrderVolume;
        root["VolumeMultiple"] = pStruct->VolumeMultiple;
        root["PriceTick"] = pStruct->PriceTick;
        root["CreateDate"].SetString(pStruct->CreateDate, sizeof (pStruct->CreateDate));
        root["OpenDate"].SetString(pStruct->OpenDate, sizeof (pStruct->OpenDate));
        root["ExpireDate"].SetString(pStruct->ExpireDate, sizeof (pStruct->ExpireDate));
        root["StartDelivDate"].SetString(pStruct->StartDelivDate, sizeof (pStruct->StartDelivDate));
        root["EndDelivDate"].SetString(pStruct->EndDelivDate, sizeof (pStruct->EndDelivDate));
        root["InstLifePhase"] = pStruct->InstLifePhase;
        root["IsTrading"] = pStruct->IsTrading;
        root["PositionType"] = pStruct->PositionType;
        root["PositionDateType"] = pStruct->PositionDateType;
        root["LongMarginRatio"] = pStruct->LongMarginRatio;
        root["ShortMarginRatio"] = pStruct->ShortMarginRatio;
        root["MaxMarginSideAlgorithm"] = pStruct->MaxMarginSideAlgorithm;
        root["UnderlyingInstrID"].SetString(pStruct->UnderlyingInstrID, sizeof (pStruct->UnderlyingInstrID));
        root["StrikePrice"] = pStruct->StrikePrice;
        root["OptionsType"] = pStruct->OptionsType;
        root["UnderlyingMultiple"] = pStruct->UnderlyingMultiple;
        root["CombinationType"] = pStruct->CombinationType;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryInstrument:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pStruct,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["InvestorRange"] = pStruct->InvestorRange;
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentMarginRate:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pStruct,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["InvestorRange"] = pStruct->InvestorRange;
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentCommissionRate:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["AccountID"].SetString(pStruct->AccountID, sizeof (pStruct->AccountID));
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
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["SettlementID"] = pStruct->SettlementID;
        root["Credit"] = pStruct->Credit;
        root["Mortgage"] = pStruct->Mortgage;
        root["ExchangeMargin"] = pStruct->ExchangeMargin;
        root["DeliveryMargin"] = pStruct->DeliveryMargin;
        root["ExchangeDeliveryMargin"] = pStruct->ExchangeDeliveryMargin;
        root["ReserveBalance"] = pStruct->ReserveBalance;
        root["CurrencyID"].SetString(pStruct->CurrencyID, sizeof (pStruct->CurrencyID));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryTradingAccount:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pStruct,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
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
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPosition:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pStruct,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["Direction"] = pStruct->Direction;
        root["OpenDate"].SetString(pStruct->OpenDate, sizeof (pStruct->OpenDate));
        root["TradeID"].SetString(pStruct->TradeID, sizeof (pStruct->TradeID));
        root["Volume"] = pStruct->Volume;
        root["OpenPrice"] = pStruct->OpenPrice;
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["SettlementID"] = pStruct->SettlementID;
        root["TradeType"] = pStruct->TradeType;
        root["CombInstrumentID"].SetString(pStruct->CombInstrumentID, sizeof (pStruct->CombInstrumentID));
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPositionDetail:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["OrderPriceType"] = pStruct->OrderPriceType;
        root["Direction"] = pStruct->Direction;
        root["CombOffsetFlag"].SetString(pStruct->CombOffsetFlag, sizeof (pStruct->CombOffsetFlag));
        root["CombHedgeFlag"].SetString(pStruct->CombHedgeFlag, sizeof (pStruct->CombHedgeFlag));
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
        root["TimeCondition"] = pStruct->TimeCondition;
        root["GTDDate"].SetString(pStruct->GTDDate, sizeof (pStruct->GTDDate));
        root["VolumeCondition"] = pStruct->VolumeCondition;
        root["MinVolume"] = pStruct->MinVolume;
        root["ContingentCondition"] = pStruct->ContingentCondition;
        root["StopPrice"] = pStruct->StopPrice;
        root["ForceCloseReason"] = pStruct->ForceCloseReason;
        root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
        root["BusinessUnit"].SetString(pStruct->BusinessUnit, sizeof (pStruct->BusinessUnit));
        root["RequestID"] = pStruct->RequestID;
        root["UserForceClose"] = pStruct->UserForceClose;
        root["IsSwapOrder"] = pStruct->IsSwapOrder;
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
        root["InvestUnitID"].SetString(pStruct->InvestUnitID, sizeof (pStruct->InvestUnitID));
        root["AccountID"].SetString(pStruct->AccountID, sizeof (pStruct->AccountID));
        root["CurrencyID"].SetString(pStruct->CurrencyID, sizeof (pStruct->CurrencyID));
        root["ClientID"].SetString(pStruct->ClientID, sizeof (pStruct->ClientID));
        root["IPAddress"].SetString(pStruct->IPAddress, sizeof (pStruct->IPAddress));
        root["MacAddress"].SetString(pStruct->MacAddress, sizeof (pStruct->MacAddress));
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspOrderInsert:" + ntos(nRequestID), sb.GetString());
    logger->info("OnRspOrderInsert：%v", sb.GetString());
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["OrderActionRef"] = pStruct->OrderActionRef;
        root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
        root["RequestID"] = pStruct->RequestID;
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
        root["OrderSysID"].SetString(pStruct->OrderSysID, sizeof (pStruct->OrderSysID));
        root["ActionFlag"] = pStruct->ActionFlag;
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeChange"] = pStruct->VolumeChange;
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["InvestUnitID"].SetString(pStruct->InvestUnitID, sizeof (pStruct->InvestUnitID));
        root["IPAddress"].SetString(pStruct->IPAddress, sizeof (pStruct->IPAddress));
        root["MacAddress"].SetString(pStruct->MacAddress, sizeof (pStruct->MacAddress));
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspOrderAction:" + ntos(nRequestID), sb.GetString());
    logger->info("OnRspOrderAction：%v", sb.GetString());
}

void CTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["OrderPriceType"] = pStruct->OrderPriceType;
        root["Direction"] = pStruct->Direction;
        root["CombOffsetFlag"].SetString(pStruct->CombOffsetFlag, sizeof (pStruct->CombOffsetFlag));
        root["CombHedgeFlag"].SetString(pStruct->CombHedgeFlag, sizeof (pStruct->CombHedgeFlag));
        root["LimitPrice"] = pStruct->LimitPrice;
        root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
        root["TimeCondition"] = pStruct->TimeCondition;
        root["GTDDate"].SetString(pStruct->GTDDate, sizeof (pStruct->GTDDate));
        root["VolumeCondition"] = pStruct->VolumeCondition;
        root["MinVolume"] = pStruct->MinVolume;
        root["ContingentCondition"] = pStruct->ContingentCondition;
        root["StopPrice"] = pStruct->StopPrice;
        root["ForceCloseReason"] = pStruct->ForceCloseReason;
        root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
        root["BusinessUnit"].SetString(pStruct->BusinessUnit, sizeof (pStruct->BusinessUnit));
        root["RequestID"] = pStruct->RequestID;
        root["OrderLocalID"].SetString(pStruct->OrderLocalID, sizeof (pStruct->OrderLocalID));
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
        root["ParticipantID"].SetString(pStruct->ParticipantID, sizeof (pStruct->ParticipantID));
        root["ClientID"].SetString(pStruct->ClientID, sizeof (pStruct->ClientID));
        root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
        root["TraderID"].SetString(pStruct->TraderID, sizeof (pStruct->TraderID));
        root["InstallID"] = pStruct->InstallID;
        root["OrderSubmitStatus"] = pStruct->OrderSubmitStatus;
        root["NotifySequence"] = pStruct->NotifySequence;
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["SettlementID"] = pStruct->SettlementID;
        root["OrderSysID"].SetString(pStruct->OrderSysID, sizeof (pStruct->OrderSysID));
        root["OrderSource"] = pStruct->OrderSource;
        root["OrderStatus"] = pStruct->OrderStatus;
        root["OrderType"] = pStruct->OrderType;
        root["VolumeTraded"] = pStruct->VolumeTraded;
        root["VolumeTotal"] = pStruct->VolumeTotal;
        root["InsertDate"].SetString(pStruct->InsertDate, sizeof (pStruct->InsertDate));
        root["InsertTime"].SetString(pStruct->InsertTime, sizeof (pStruct->InsertTime));
        root["ActiveTime"].SetString(pStruct->ActiveTime, sizeof (pStruct->ActiveTime));
        root["SuspendTime"].SetString(pStruct->SuspendTime, sizeof (pStruct->SuspendTime));
        root["UpdateTime"].SetString(pStruct->UpdateTime, sizeof (pStruct->UpdateTime));
        root["CancelTime"].SetString(pStruct->CancelTime, sizeof (pStruct->CancelTime));
        root["ActiveTraderID"].SetString(pStruct->ActiveTraderID, sizeof (pStruct->ActiveTraderID));
        root["ClearingPartID"].SetString(pStruct->ClearingPartID, sizeof (pStruct->ClearingPartID));
        root["SequenceNo"] = pStruct->SequenceNo;
        root["FrontID"] = pStruct->FrontID;
        root["SessionID"] = pStruct->SessionID;
        root["InsertDate"].SetString(pStruct->InsertDate, sizeof (pStruct->InsertDate));
        root["UserProductInfo"].SetString(pStruct->UserProductInfo, sizeof (pStruct->UserProductInfo));
        char utf_str[512] = {0};
        gb2312toutf8(pStruct->StatusMsg, sizeof(pStruct->StatusMsg), utf_str, sizeof(utf_str));
        root["StatusMsg"].SetString(utf_str, sizeof (utf_str));
        root["UserForceClose"] = pStruct->UserForceClose;
        root["ActiveUserID"].SetString(pStruct->ActiveUserID, sizeof (pStruct->ActiveUserID));
        root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
        root["RelativeOrderSysID"].SetString(pStruct->RelativeOrderSysID, sizeof (pStruct->RelativeOrderSysID));
        root["ZCETotalTradedVolume"] = pStruct->ZCETotalTradedVolume;
        root["IsSwapOrder"] = pStruct->IsSwapOrder;
        root["BranchID"].SetString(pStruct->BranchID, sizeof (pStruct->BranchID));
        root["InvestUnitID"].SetString(pStruct->InvestUnitID, sizeof (pStruct->InvestUnitID));
        root["AccountID"].SetString(pStruct->AccountID, sizeof (pStruct->AccountID));
        root["CurrencyID"].SetString(pStruct->CurrencyID, sizeof (pStruct->CurrencyID));
        root["IPAddress"].SetString(pStruct->IPAddress, sizeof (pStruct->IPAddress));
        root["MacAddress"].SetString(pStruct->MacAddress, sizeof (pStruct->MacAddress));
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryOrder:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
        root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
        root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
        root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
        root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
        root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
        root["TradeID"].SetString(pStruct->TradeID, sizeof (pStruct->TradeID));
        root["Direction"] = pStruct->Direction;
        root["OrderSysID"].SetString(pStruct->OrderSysID, sizeof (pStruct->OrderSysID));
        root["ParticipantID"].SetString(pStruct->ParticipantID, sizeof (pStruct->ParticipantID));
        root["ClientID"].SetString(pStruct->ClientID, sizeof (pStruct->ClientID));
        root["TradingRole"] = pStruct->TradingRole;
        root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
        root["OffsetFlag"] = pStruct->OffsetFlag;
        root["HedgeFlag"] = pStruct->HedgeFlag;
        root["Price"] = pStruct->Price;
        root["Volume"] = pStruct->Volume;
        root["TradeDate"].SetString(pStruct->TradeDate, sizeof (pStruct->TradeDate));
        root["TradeTime"].SetString(pStruct->TradeTime, sizeof (pStruct->TradeTime));
        root["TradeType"] = pStruct->TradeType;
        root["PriceSource"] = pStruct->PriceSource;
        root["TraderID"].SetString(pStruct->TraderID, sizeof (pStruct->TraderID));
        root["OrderLocalID"].SetString(pStruct->OrderLocalID, sizeof (pStruct->OrderLocalID));
        root["ClearingPartID"].SetString(pStruct->ClearingPartID, sizeof (pStruct->ClearingPartID));
        root["BusinessUnit"].SetString(pStruct->BusinessUnit, sizeof (pStruct->BusinessUnit));
        root["SequenceNo"] = pStruct->SequenceNo;
        root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
        root["SettlementID"] = pStruct->SettlementID;
        root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
        root["TradeSource"] = pStruct->TradeSource;
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspQryTrade:" + ntos(nRequestID), sb.GetString());
    if (bIsLast && nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

///报单通知
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pStruct) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
    root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
    root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
    root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
    root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
    root["OrderPriceType"] = pStruct->OrderPriceType;
    root["Direction"] = pStruct->Direction;
    root["CombOffsetFlag"].SetString(pStruct->CombOffsetFlag, sizeof (pStruct->CombOffsetFlag));
    root["CombHedgeFlag"].SetString(pStruct->CombHedgeFlag, sizeof (pStruct->CombHedgeFlag));
    root["LimitPrice"] = pStruct->LimitPrice;
    root["VolumeTotalOriginal"] = pStruct->VolumeTotalOriginal;
    root["TimeCondition"] = pStruct->TimeCondition;
    root["GTDDate"].SetString(pStruct->GTDDate, sizeof (pStruct->GTDDate));
    root["VolumeCondition"] = pStruct->VolumeCondition;
    root["MinVolume"] = pStruct->MinVolume;
    root["ContingentCondition"] = pStruct->ContingentCondition;
    root["StopPrice"] = pStruct->StopPrice;
    root["ForceCloseReason"] = pStruct->ForceCloseReason;
    root["IsAutoSuspend"] = pStruct->IsAutoSuspend;
    root["BusinessUnit"].SetString(pStruct->BusinessUnit, sizeof (pStruct->BusinessUnit));
    root["RequestID"] = pStruct->RequestID;
    root["OrderLocalID"].SetString(pStruct->OrderLocalID, sizeof (pStruct->OrderLocalID));
    root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
    root["ParticipantID"].SetString(pStruct->ParticipantID, sizeof (pStruct->ParticipantID));
    root["ClientID"].SetString(pStruct->ClientID, sizeof (pStruct->ClientID));
    root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
    root["TraderID"].SetString(pStruct->TraderID, sizeof (pStruct->TraderID));
    root["InstallID"] = pStruct->InstallID;
    root["OrderSubmitStatus"] = pStruct->OrderSubmitStatus;
    root["NotifySequence"] = pStruct->NotifySequence;
    root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
    root["SettlementID"] = pStruct->SettlementID;
    root["OrderSysID"].SetString(pStruct->OrderSysID, sizeof (pStruct->OrderSysID));
    root["OrderSource"] = pStruct->OrderSource;
    root["OrderStatus"] = pStruct->OrderStatus;
    root["OrderType"] = pStruct->OrderType;
    root["VolumeTraded"] = pStruct->VolumeTraded;
    root["VolumeTotal"] = pStruct->VolumeTotal;
    root["InsertDate"].SetString(pStruct->InsertDate, sizeof (pStruct->InsertDate));
    root["InsertTime"].SetString(pStruct->InsertTime, sizeof (pStruct->InsertTime));
    root["ActiveTime"].SetString(pStruct->ActiveTime, sizeof (pStruct->ActiveTime));
    root["SuspendTime"].SetString(pStruct->SuspendTime, sizeof (pStruct->SuspendTime));
    root["UpdateTime"].SetString(pStruct->UpdateTime, sizeof (pStruct->UpdateTime));
    root["CancelTime"].SetString(pStruct->CancelTime, sizeof (pStruct->CancelTime));
    root["ActiveTraderID"].SetString(pStruct->ActiveTraderID, sizeof (pStruct->ActiveTraderID));
    root["ClearingPartID"].SetString(pStruct->ClearingPartID, sizeof (pStruct->ClearingPartID));
    root["SequenceNo"] = pStruct->SequenceNo;
    root["FrontID"] = pStruct->FrontID;
    root["SessionID"] = pStruct->SessionID;
    root["UserProductInfo"].SetString(pStruct->UserProductInfo, sizeof (pStruct->UserProductInfo));
    char utf_str[512] = {0};
    gb2312toutf8(pStruct->StatusMsg, sizeof(pStruct->StatusMsg), utf_str, sizeof(utf_str));
    root["StatusMsg"].SetString(utf_str, sizeof (utf_str));
    root["UserForceClose"] = pStruct->UserForceClose;
    root["ActiveUserID"].SetString(pStruct->ActiveUserID, sizeof (pStruct->ActiveUserID));
    root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
    root["RelativeOrderSysID"].SetString(pStruct->RelativeOrderSysID, sizeof (pStruct->RelativeOrderSysID));
    root["ZCETotalTradedVolume"] = pStruct->ZCETotalTradedVolume;
    root["IsSwapOrder"] = pStruct->IsSwapOrder;
    root["BranchID"].SetString(pStruct->BranchID, sizeof (pStruct->BranchID));
    root["InvestUnitID"].SetString(pStruct->InvestUnitID, sizeof (pStruct->InvestUnitID));
    root["AccountID"].SetString(pStruct->AccountID, sizeof (pStruct->AccountID));
    root["CurrencyID"].SetString(pStruct->CurrencyID, sizeof (pStruct->CurrencyID));
    root["IPAddress"].SetString(pStruct->IPAddress, sizeof (pStruct->IPAddress));
    root["MacAddress"].SetString(pStruct->MacAddress, sizeof (pStruct->MacAddress));
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRtnOrder:" + root["OrderRef"].GetString(), sb.GetString());
    logger->info("OnRtnOrder：%v", sb.GetString());
}

///成交通知
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pStruct) {
    rapidjson::Document root;
    root["AppID"].SetString(APPID, root.GetAllocator());
    root["SessionID"] = SESSION_ID;
    root["BrokerID"].SetString(pStruct->BrokerID, sizeof (pStruct->BrokerID));
    root["InvestorID"].SetString(pStruct->InvestorID, sizeof (pStruct->InvestorID));
    root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
    root["OrderRef"].SetString(pStruct->OrderRef, sizeof (pStruct->OrderRef));
    root["UserID"].SetString(pStruct->UserID, sizeof (pStruct->UserID));
    root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
    root["TradeID"].SetString(pStruct->TradeID, sizeof (pStruct->TradeID));
    root["Direction"] = pStruct->Direction;
    root["OrderSysID"].SetString(pStruct->OrderSysID, sizeof (pStruct->OrderSysID));
    root["ParticipantID"].SetString(pStruct->ParticipantID, sizeof (pStruct->ParticipantID));
    root["ClientID"].SetString(pStruct->ClientID, sizeof (pStruct->ClientID));
    root["TradingRole"] = pStruct->TradingRole;
    root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
    root["OffsetFlag"] = pStruct->OffsetFlag;
    root["HedgeFlag"] = pStruct->HedgeFlag;
    root["Price"] = pStruct->Price;
    root["Volume"] = pStruct->Volume;
    root["TradeDate"].SetString(pStruct->TradeDate, sizeof (pStruct->TradeDate));
    root["TradeTime"].SetString(pStruct->TradeTime, sizeof (pStruct->TradeTime));
    root["TradeType"] = pStruct->TradeType;
    root["PriceSource"] = pStruct->PriceSource;
    root["TraderID"].SetString(pStruct->TraderID, sizeof (pStruct->TraderID));
    root["OrderLocalID"].SetString(pStruct->OrderLocalID, sizeof (pStruct->OrderLocalID));
    root["ClearingPartID"].SetString(pStruct->ClearingPartID, sizeof (pStruct->ClearingPartID));
    root["BusinessUnit"].SetString(pStruct->BusinessUnit, sizeof (pStruct->BusinessUnit));
    root["SequenceNo"] = pStruct->SequenceNo;
    root["TradingDay"].SetString(pStruct->TradingDay, sizeof (pStruct->TradingDay));
    root["SettlementID"] = pStruct->SettlementID;
    root["BrokerOrderSeq"] = pStruct->BrokerOrderSeq;
    root["TradeSource"] = pStruct->TradeSource;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRtnTrade:" + root["OrderRef"].GetString(), sb.GetString());
    logger->info("OnRtnTrade：%v", sb.GetString());
}

void CTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pStruct) {
    /*
     * {"EnterReason":"1","EnterTime":"15:00:00","ExchangeID":"CFFEX","ExchangeInstID":"IF",
     * "InstrumentID":"IF","InstrumentStatus":"6","SettlementGroupID":"00000001","TradingSegmentSN":99}
     */
    rapidjson::Document root;
    root["ExchangeID"].SetString(pStruct->ExchangeID, sizeof (pStruct->ExchangeID));
    root["ExchangeInstID"].SetString(pStruct->ExchangeInstID, sizeof (pStruct->ExchangeInstID));
    root["SettlementGroupID"].SetString(pStruct->SettlementGroupID, sizeof (pStruct->SettlementGroupID));
    root["InstrumentID"].SetString(pStruct->InstrumentID, sizeof (pStruct->InstrumentID));
    root["InstrumentStatus"] = pStruct->InstrumentStatus;
    root["TradingSegmentSN"] = pStruct->TradingSegmentSN;
    root["EnterTime"].SetString(pStruct->EnterTime, sizeof (pStruct->EnterTime));
    root["EnterReason"] = pStruct->EnterReason;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRtnInstrumentStatus:" + root["InstrumentID"].GetString(), 
                       sb.GetString());
}

void CTraderSpi::OnFrontDisconnected(int nReason) {
    publisher->publish(CHANNEL_TRADE_DATA + "OnFrontDisconnected", ntos(nReason));
    trade_login = false;
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher->publish(CHANNEL_TRADE_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    rapidjson::Document root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    publisher->publish(CHANNEL_TRADE_DATA + "OnRspError:" + ntos(nRequestID), sb.GetString());
    query_finished = true;
    check_cmd.notify_all();
    logger->info("OnRspError：%v", sb.GetString());
}
