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
#pragma once
#include "ThostFtdcTraderApi.h"

class CTraderSpi : public CThostFtdcTraderSpi {
public:
    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    void OnFrontConnected() override;

    ///客户端认证响应
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pStruct,
                           CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///登录请求响应
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) override;

    ///投资者结算结果确认响应
    void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询结算信息确认响应
    void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pStruct,
                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询投资者结算结果响应
    void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pStruct,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询合约响应
    void OnRspQryInstrument(CThostFtdcInstrumentField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) override;

    ///请求查询合约保证金率响应
    void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pStruct,
                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///合约交易状态通知
    void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pStruct) override;

    ///请求查询合约手续费率响应
    void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pStruct,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询资金账户响应
    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pStruct,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询投资者持仓响应
    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pStruct,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///请求查询投资者持仓明细响应
    void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pStruct,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///报单录入请求响应
    void OnRspOrderInsert(CThostFtdcInputOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                          int nRequestID, bool bIsLast) override;

    ///报单操作请求响应
    void OnRspOrderAction(CThostFtdcInputOrderActionField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                          int nRequestID, bool bIsLast) override;

    ///请求查询报单响应
    void OnRspQryOrder(CThostFtdcOrderField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) override;

    ///请求查询成交响应
    void OnRspQryTrade(CThostFtdcTradeField *pStruct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) override;

    ///错误应答
    void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    void OnFrontDisconnected(int nReason) override;

    ///心跳超时警告。当长时间未收到报文时，该方法被调用。
    void OnHeartBeatWarning(int nTimeLapse) override;

    ///报单通知
    void OnRtnOrder(CThostFtdcOrderField *pStruct) override;

    ///成交通知
    void OnRtnTrade(CThostFtdcTradeField *pStruct) override;

    ///请求查询行情响应
    void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pStruct, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;
};
