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
    shared_ptr<CThostFtdcReqUserLoginField> req(new CThostFtdcReqUserLoginField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->UserID, INVESTOR_ID.c_str());
    strcpy(req->Password, PASSWORD.c_str());
    pTraderApi->ReqUserLogin(req.get(), 1);
    publisher.publish(CHANNEL_TRADE_DATA + "OnFrontConnected", "OnFrontConnected");
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pStruct,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspUserLogin:" + ntos(nRequestID), writer.write(root));
    publisher.set("TradingDay", root["TradingDay"].asString());
    cout << "直接确认结算单。日期=" << pStruct->TradingDay << endl;
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
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQrySettlementInfoConfirm:" + ntos(nRequestID), writer.write(root));
    // ConfirmDate = "20160803"
    long hours = 24;
    if (pStruct) {
        string confirmDate(pStruct->ConfirmDate);
        string confirmTime(pStruct->ConfirmTime);
        std::tm tm = {};
        std::stringstream ss(confirmDate + confirmTime);
        ss >> std::get_time(&tm, "%Y%m%d%H:%M:%S");
        auto confirm_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        cout << "上次结算单确认时间=" << std::put_time(&tm, "%F %T") << endl;
        auto now = std::chrono::system_clock::now();
        hours = std::chrono::duration_cast<std::chrono::hours>(now - confirm_date).count();
        root["empty"] = false;
    } else {
        root["empty"] = true;
    }
    if (hours >= 24) {
        cout << "查询结算单.." << endl;
        shared_ptr<CThostFtdcQrySettlementInfoField> req(new CThostFtdcQrySettlementInfoField);
        strcpy(req->BrokerID, BROKER_ID.c_str());
        strcpy(req->InvestorID, INVESTOR_ID.c_str());
        strcpy(req->TradingDay, "");
        pTraderApi->ReqQrySettlementInfo(req.get(), nRequestID + 1);
    }
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQrySettlementInfoConfirm:" + ntos(nRequestID), writer.write(root));
    cout << "确认结算单。" << endl;
    shared_ptr<CThostFtdcSettlementInfoConfirmField> req(new CThostFtdcSettlementInfoConfirmField);
    strcpy(req->BrokerID, BROKER_ID.c_str());
    strcpy(req->InvestorID, INVESTOR_ID.c_str());
    pTraderApi->ReqSettlementInfoConfirm(req.get(), nRequestID + 1);
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspSettlementInfoConfirm:" + ntos(nRequestID), writer.write(root));
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrument:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pStruct,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentMarginRate:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pStruct,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentCommissionRate:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryTradingAccount:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pStruct,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else if (pStruct) {
        cout << "进来了!" << endl;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPosition:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pStruct,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPositionDetail:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspOrderInsert:" + ntos(nRequestID), writer.write(root));
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspOrderAction:" + ntos(nRequestID), writer.write(root));
}

void CTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryOrder:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

void CTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryTrade:" + ntos(nRequestID), writer.write(root));
    if (bIsLast and nRequestID == iTradeRequestID) {
        query_finished = true;
        check_cmd.notify_all();
    }
}

///报单通知
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pStruct) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRtnOrder:" + root["OrderRef"].asString(), writer.write(root));
}

///成交通知
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pStruct) {
    Json::Value root;
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
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRtnTrade:" + root["OrderRef"].asString(),
                      writer.write(root));
}

void CTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pStruct) {
    /*
     * {"EnterReason":"1","EnterTime":"15:00:00","ExchangeID":"CFFEX","ExchangeInstID":"IF",
     * "InstrumentID":"IF","InstrumentStatus":"6","SettlementGroupID":"00000001","TradingSegmentSN":99}
     */
    Json::Value root;
    root["ExchangeID"] = pStruct->ExchangeID;
    root["ExchangeInstID"] = pStruct->ExchangeInstID;
    root["SettlementGroupID"] = pStruct->SettlementGroupID;
    root["InstrumentID"] = pStruct->InstrumentID;
    root["InstrumentStatus"] = pStruct->InstrumentStatus;
    root["TradingSegmentSN"] = pStruct->TradingSegmentSN;
    root["EnterTime"] = pStruct->EnterTime;
    root["EnterReason"] = pStruct->EnterReason;
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRtnInstrumentStatus:" + root["InstrumentID"].asString(),
                      writer.write(root));
}

void CTraderSpi::OnFrontDisconnected(int nReason) {
    publisher.publish(CHANNEL_TRADE_DATA + "OnFrontDisconnected", ntos(nReason));
    trade_login = false;
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher.publish(CHANNEL_TRADE_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    Json::FastWriter writer;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspError:" + ntos(nRequestID), writer.write(root));
    query_finished = true;
    check_cmd.notify_all();
}
