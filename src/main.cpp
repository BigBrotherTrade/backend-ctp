#include <iostream>
#include "TraderSpi.h"
#include "MdSpi.h"
#include "global.h"

using namespace std;
using namespace redox;

int main(int argc, char* argv[]) {
    boost::property_tree::ini_parser::read_ini("config.ini", pt);
    if(!publisher.connect(pt.get<std::string>("redis.host"), pt.get<int>("redis.port"))) return 1;
    if(!subscriber.connect(pt.get<std::string>("redis.host"), pt.get<int>("redis.port"))) return 1;

    std::strcpy(BROKER_ID, pt.get<std::string>("sim.broker").c_str());
    std::strcpy(INVESTOR_ID, pt.get<std::string>("sim.investor").c_str());
    std::strcpy(PASSWORD, pt.get<std::string>("sim.passwd").c_str());
    std::strcpy(IP_ADDRESS, pt.get<std::string>("host.ip").c_str());
    std::strcpy(MAC_ADDRESS, pt.get<std::string>("host.mac").c_str());

    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();	// 创建TradeApi
    CTraderSpi* pTraderSpi = new CTraderSpi();
    pTraderApi->RegisterSpi(pTraderSpi);			            // 注册事件类
    pTraderApi->SubscribePublicTopic(THOST_TERT_RESTART);		// 注册公有流
    pTraderApi->SubscribePrivateTopic(THOST_TERT_RESTART);		// 注册私有流
    pTraderApi->RegisterFront((char*)pt.get<std::string>("sim.trade").c_str());	// connect
    pTraderApi->Init();

    pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();		        // 创建MdApi
    CThostFtdcMdSpi* pMdSpi = new CMdSpi();
    pMdApi->RegisterSpi(pMdSpi);						        // 注册事件类
    pMdApi->RegisterFront((char*)pt.get<std::string>("sim.market").c_str());	// connect
    pMdApi->Init();

    subscriber.psubscribe(CHANNEL_TRADE_PATTERN, handle_trade_request);
    subscriber.psubscribe(CHANNEL_MARKET_PATTERN, handle_market_request);
    pTraderApi->Join();
    pMdApi->Join();
    publisher.disconnect();
    subscriber.disconnect();

    return 0;
}
