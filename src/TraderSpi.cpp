#include "ThostFtdcTraderApi.h"
#include "TraderSpi.h"
#include "global.h"

using namespace std;

Json::FastWriter writer;

void CTraderSpi::OnFrontConnected() {
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
    } else {
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
        ORDER_ID = atoi(pStruct->MaxOrderRef) + 1;
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspUserLogin", writer.write(root));
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["BrokerID"] = pStruct->BrokerID;
        root["InvestorID"] = pStruct->InvestorID;
        root["ConfirmDate"] = pStruct->ConfirmDate;
        root["ConfirmTime"] = pStruct->ConfirmTime;
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspSettlementInfoConfirm", writer.write(root));
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
        root["InstrumentID"] = pStruct->InstrumentID;
        root["ExchangeID"] = pStruct->ExchangeID;
        root["InstrumentName"] = pStruct->InstrumentName;
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrument", writer.write(root));
}

void CTraderSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pStruct,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentMarginRate", writer.write(root));
}

void CTraderSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pStruct,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInstrumentCommissionRate", writer.write(root));
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryTradingAccount", writer.write(root));
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pStruct,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPosition", writer.write(root));
}

void CTraderSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pStruct,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryInvestorPositionDetail", writer.write(root));
}

void CTraderSpi::ReqOrderInsert() {
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, BROKER_ID);
    ///投资者代码
    strcpy(req.InvestorID, INVESTOR_ID);
    ///合约代码
    strcpy(req.InstrumentID, INSTRUMENT_ID);
    ///报单引用
    strcpy(req.OrderRef, ORDER_REF);
    ///用户代码
//    TThostFtdcUserIDType    UserID;
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向: 
    req.Direction = DIRECTION;
    ///组合开平标志: 开仓
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    req.LimitPrice = LIMIT_PRICE;
    ///数量: 1
    req.VolumeTotalOriginal = 1;
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///GTD日期
//    TThostFtdcDateType    GTDDate;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///止损价
//    TThostFtdcPriceType    StopPrice;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///业务单元
//    TThostFtdcBusinessUnitType    BusinessUnit;
    ///请求编号
//    TThostFtdcRequestIDType    RequestID;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    lOrderTime = time(NULL);
    int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
    cerr << "--->>> 报单录入请求: " << ((iResult == 0) ? "成功" : "失败") << endl;
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspOrderInsert", writer.write(root));
}

void CTraderSpi::ReqOrderAction(CThostFtdcOrderField *pOrder) {
    static bool ORDER_ACTION_SENT = false;        //是否发送了报单
    if (ORDER_ACTION_SENT)
        return;

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, pOrder->BrokerID);
    ///投资者代码
    strcpy(req.InvestorID, pOrder->InvestorID);
    ///报单操作引用
//    TThostFtdcOrderActionRefType    OrderActionRef;
    ///报单引用
    strcpy(req.OrderRef, pOrder->OrderRef);
    ///请求编号
//    TThostFtdcRequestIDType    RequestID;
    ///前置编号
    req.FrontID = FRONT_ID;
    ///会话编号
    req.SessionID = SESSION_ID;
    ///交易所代码
//    TThostFtdcExchangeIDType    ExchangeID;
    ///报单编号
//    TThostFtdcOrderSysIDType    OrderSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
//    TThostFtdcPriceType    LimitPrice;
    ///数量变化
//    TThostFtdcVolumeType    VolumeChange;
    ///用户代码
//    TThostFtdcUserIDType    UserID;
    ///合约代码
    strcpy(req.InstrumentID, pOrder->InstrumentID);
    lOrderTime = time(NULL);
    int iResult = pTraderApi->ReqOrderAction(&req, ++iRequestID);
    cerr << "--->>> 报单操作请求: " << ((iResult == 0) ? "成功" : "失败") << endl;
    ORDER_ACTION_SENT = true;
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspOrderAction", writer.write(root));
}

void CTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                   bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = 0;

    if (pRspInfo && pRspInfo->ErrorID != 0) {
        root["ErrorID"] = pRspInfo->ErrorID;
    } else {
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
        root["StatusMsg"] = pStruct->StatusMsg;
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
    }
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspQryOrder", writer.write(root));
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
    root["StatusMsg"] = pStruct->StatusMsg;
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
    publisher.publish(CHANNEL_TRADE_DATA + "OnRtnOrder", writer.write(root));
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
    publisher.publish(CHANNEL_TRADE_DATA + "OnRtnTrade", writer.write(root));
}

void CTraderSpi::OnFrontDisconnected(int nReason) {
    publisher.publish(CHANNEL_TRADE_DATA + "OnFrontDisconnected", ntos(nReason));
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
    publisher.publish(CHANNEL_TRADE_DATA + "OnHeartBeatWarning", ntos(nTimeLapse));
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    Json::Value root;
    root["nRequestID"] = nRequestID;
    root["bIsLast"] = bIsLast;
    root["ErrorID"] = pRspInfo->ErrorID;
    publisher.publish(CHANNEL_TRADE_DATA + "OnRspError", writer.write(root));
}

bool CTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder) {
    return ((pOrder->FrontID == FRONT_ID) &&
            (pOrder->SessionID == SESSION_ID) &&
            (strcmp(pOrder->OrderRef, ORDER_REF) == 0));
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder) {
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}
