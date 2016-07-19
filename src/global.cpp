#include "global.h"
#include <iconv.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace redox;

int gb2312toutf8(char *sourcebuf, size_t sourcelen, char *destbuf, size_t destlen) {

    iconv_t cd;
    if ((cd = iconv_open("utf-8", "gb2312")) == 0)

        return -1;
    memset(destbuf, 0, destlen);

    char **source = &sourcebuf;

    char **dest = &destbuf;
    if (-1 == iconv(cd, source, &sourcelen, dest, &destlen))

        return -1;

    iconv_close(cd);

    return 0;

}

const std::string &ntos(const int n) {
    std::stringstream newstr;
    newstr << n;
    return newstr.str();
}

void handle_market_request(const string &topic, const string &msg) {
    std::vector<std::string> strs;
    boost::split(strs, topic, boost::is_any_of(":"));
    auto request_type = strs.back();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg, root);
    if (request_type == "ReqUserLogin") {
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, root["BrokerID"].asCString());
        strcpy(req.UserID, root["UserID"].asCString());
        strcpy(req.Password, root["Password"].asCString());
        int iResult = pMdApi->ReqUserLogin(&req, ++iRequestID);
    }
    else if (request_type == "SubscribeMarketData") {
        char **charArray = new char *[root.size()];
        for (int i = 0; i < root.size(); ++i) {
            charArray[i] = new char[32];
            strcpy(charArray[i], root[i].asCString());
        }
        int iResult = pMdApi->SubscribeMarketData(charArray, root.size());
        for (int i = 0; i < root.size(); ++i)
            delete charArray[i];
        delete[](charArray);
    }
    else if (request_type == "UnSubscribeMarketData") {
        char **charArray = new char *[root.size()];
        for (int i = 0; i < root.size(); ++i) {
            charArray[i] = new char[32];
            strcpy(charArray[i], root[i].asCString());
        }
        int iResult = pMdApi->UnSubscribeMarketData(charArray, root.size());
        for (int i = 0; i < root.size(); ++i)
            delete charArray[i];
        delete[](charArray);
    }
}

void handle_trade_request(const string &topic, const string &msg) {
    std::vector<std::string> strs;
    boost::split(strs, topic, boost::is_any_of(":"));
    auto request_type = strs.back();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg, root);
    if (request_type == "ReqUserLogin") {
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(BROKER_ID, root["BrokerID"].asCString());
        strcpy(INVESTOR_ID, root["InvestorID"].asCString());
        strcpy(PASSWORD, root["Password"].asCString());
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.UserID, INVESTOR_ID);
        strcpy(req.Password, PASSWORD);
        int iResult = pMdApi->ReqUserLogin(&req, ++iRequestID);
    }
    else if (request_type == "ReqSettlementInfoConfirm") {
        CThostFtdcSettlementInfoConfirmField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        int iResult = pTraderApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
    }
    else if (request_type == "ReqQrySettlementInfo") {
        CThostFtdcQrySettlementInfoField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        int iResult = pTraderApi->ReqQrySettlementInfo(&req, ++iRequestID);
    }
    else if (request_type == "ReqSettlementInfoConfirm") {
        CThostFtdcSettlementInfoConfirmField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        int iResult = pTraderApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInstrument") {
        CThostFtdcQryInstrumentField req;
        memset(&req, 0, sizeof(req));
        if (!root.isNull()) {
            strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        }
        int iResult = pTraderApi->ReqQryInstrument(&req, ++iRequestID);
    }
    else if (request_type == "QryInstrumentCommissionRate") {
        CThostFtdcQryInstrumentCommissionRateField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        if (!root.isNull()) {
            strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        }
        int iResult = pTraderApi->ReqQryInstrumentCommissionRate(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInstrumentMarginRate") {
        CThostFtdcQryInstrumentMarginRateField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        if (!root.isNull()) {
            strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        }
        int iResult = pTraderApi->ReqQryInstrumentMarginRate(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryTradingAccount") {
        CThostFtdcQryTradingAccountField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        int iResult = pTraderApi->ReqQryTradingAccount(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInvestorPosition") {
        CThostFtdcQryInvestorPositionField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        if (!root.isNull()) {
            strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        }
        int iResult = pTraderApi->ReqQryInvestorPosition(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInvestorPositionDetail") {
        CThostFtdcQryInvestorPositionDetailField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        if (!root.isNull()) {
            strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        }
        int iResult = pTraderApi->ReqQryInvestorPositionDetail(&req, ++iRequestID);
    }
    else if (request_type == "ReqOrderInsert") {
        CThostFtdcInputOrderField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, root["InstrumentID"].asCString());
        ///投资者代码
        strcpy(req.InvestorID, root["InstrumentID"].asCString());
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
        int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryOrder") {
        CThostFtdcQryOrderField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        int iResult = pTraderApi->ReqQryOrder(&req, ++iRequestID);
    }
    else if (request_type == "ReqOrderAction") {
        CThostFtdcInputOrderActionField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        strcpy(req.OrderRef, root["OrderRef"].asCString());
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);
        req.FrontID = FRONT_ID;
        req.SessionID = SESSION_ID;
        req.ActionFlag = THOST_FTDC_AF_Delete;
        int iResult = pTraderApi->ReqOrderAction(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInstrument") {
        CThostFtdcQryInstrumentField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        int iResult = pTraderApi->ReqQryInstrument(&req, ++iRequestID);
    }
    else if (request_type == "ReqQryInstrument") {
        CThostFtdcQryInstrumentField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.InstrumentID, root["InstrumentID"].asCString());
        int iResult = pTraderApi->ReqQryInstrument(&req, ++iRequestID);
    }
}
