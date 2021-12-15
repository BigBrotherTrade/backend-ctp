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
#include <iostream>
#include <unistd.h>
#include "TraderSpi.h"
#include "MdSpi.h"
#include "global.h"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace redox;

int main(int argc, char **argv) {
    std::string home_str(getenv("HOME"));
    std::string config_path = home_str + "/.config/backend-ctp/config.ini";
    std::string log_path = home_str + "/.cache/backend-ctp/log";
    std::string md_path = home_str + "/.cache/backend-ctp/md/";
    std::string trade_path = home_str + "/.cache/backend-ctp/trade/";
    mkdir( log_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    mkdir( md_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    mkdir( trade_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    cout << "config-file: " << config_path << endl;
    std::string 	line, key, split, val;
    std::ifstream 	ifs(config_path);
    std::map<std::string, std::string> config;
    while ( std::getline(ifs, line) )
        if (std::stringstream(line) >> key >> split >> val && key[0] != ';' && split == "=")
            config[key] = val;

    if ( daemon(0, 0) ) return 1;

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::Format,
                             "%datetime{%Y-%M-%d %H:%m:%s.%g} (%thread) [%level] %msg" );
    defaultConf.setGlobally( el::ConfigurationType::Filename, log_path + "/backend-ctp.log" );
    defaultConf.setGlobally( el::ConfigurationType::MaxLogFileSize, "2097152" );
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "0" );
    el::Loggers::reconfigureLogger("default", defaultConf);
    logger = el::Loggers::getLogger("default");
    logger->info("服务重新启动，连接 redis %v:%v", config["host"], std::stoi( config["port"] ));
    if ( ! publisher.connect( config["host"], std::stoi( config["port"] ) ) ) return 1;
    if ( ! subscriber.connect( config["host"], std::stoi( config["port"] ) ) ) return 1;

    BROKER_ID = config["broker"];
    INVESTOR_ID = config["investor"];
    PASSWORD = config["passwd"];
    APPID = config["appid"];
    AUTHCODE = config["authcode"];
    USERINFO = config["userinfo"];
    IP_ADDRESS = config["ip"];
    MAC_ADDRESS = config["mac"];
    logger->info("连接交易服务器..");
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto tm = *localtime(&tt);
    auto now = tm.tm_hour * 100 + tm.tm_min;
    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi( trade_path.c_str() );   // 创建TradeApi
    auto *pTraderSpi = new CTraderSpi();
    pTraderApi->RegisterSpi(pTraderSpi);                               // 注册事件类
    pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);                // 注册公有流
    pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);               // 注册私有流
    if ( ( now >= 845 and now <= 1520 ) or ( now >= 2045 and now <= 2359 ) ) {
        pTraderApi->RegisterFront( (char *) config["trade"].c_str() );     // connect
        logger->info("当前时间：%v-%v-%v %v:%v:%v 连接正常交易网关",
                     tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    else {
        pTraderApi->RegisterFront( (char *) config["trade_off"].c_str() ); // connect
        logger->info("当前时间：%v-%v-%v %v:%v:%v 连接离线查询网关",
                     tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    logger->info("连接行情服务器..");
    pMdApi = CThostFtdcMdApi::CreateFtdcMdApi( md_path.c_str() );      // 创建MdApi
    CThostFtdcMdSpi *pMdSpi = new CMdSpi();
    pMdApi->RegisterSpi(pMdSpi);                                       // 注册事件类
    if ( ( now >= 845 and now <= 1520 ) or ( now >= 2045 and now <= 2359 ) )
        pMdApi->RegisterFront( (char *) config["market"].c_str() );        // connect
    else
        pMdApi->RegisterFront( (char *) config["market_off"].c_str() );    // connect

    logger->info("开启命令处理线程..");
    std::thread command_handler(handle_command);
    subscriber.psubscribe(CHANNEL_REQ_PATTERN, handle_req_request);

    pTraderApi->Init();
    pMdApi->Init();

    auto host = config["host"];
    auto port = std::stoi( config["port"] );
    std::thread([host, port] {
        redox::Redox beater;
        beater.connect( host, port);
        while ( keep_running ) {
            beater.commandSync({"SET", "HEARTBEAT:BACKEND_CTP", "1", "EX", "61"});
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }).detach();
    logger->info("服务已启动.");

    pMdApi->Join();
    pTraderApi->Join();

    publisher.disconnect();
    subscriber.disconnect();
    keep_running = false;
    command_handler.join();

    logger->info("服务已退出.");
    return 0;
}
