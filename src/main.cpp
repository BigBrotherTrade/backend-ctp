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
#include <thread>
#include "TraderSpi.h"
#include "MdSpi.h"
#include "global.h"
#if defined(__linux__)
#include <unistd.h>
#include <fmt/core.h>
#elif defined(_WIN32)
#include <filesystem>
#include <format>
#endif
#include <chrono>
INITIALIZE_EASYLOGGINGPP // NOLINT
using namespace std;
#if defined(__linux__)
using namespace fmt;
#endif
using namespace chrono;
using namespace sw::redis;

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
#ifdef _WIN32
    filesystem::path config_file = filesystem::current_path() / "config.ini";
    string config_path(filesystem::current_path().string());
    cout << format("config-file: {}\\config.ini", config_path) << endl;
    string& log_path = config_path;
    cout << format("log-file: {}\\backend-ctp.log", log_path) << endl;
    string& md_path = config_path;
    string& trade_path = config_path;
    ifstream ifs(config_file.string());
#else
    string home_str(getenv("HOME"));
    string config_path = home_str + "/.config/backend-ctp/config.ini";
    string log_path = home_str + "/.cache/backend-ctp/log";
    string md_path = home_str + "/.cache/backend-ctp/md/";
    string trade_path = home_str + "/.cache/backend-ctp/trade/";
    mkdir( log_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    mkdir( md_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    mkdir( trade_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    cout << format("config-file: {}", config_path) << endl;
    cout << format("log-file: {}/backend-ctp.log", log_path) << endl;
    ifstream ifs(config_path);
#endif
    string line, key, split, val;
    map<string, string> config;
    while ( getline(ifs, line) )
        if (stringstream(line) >> key >> split >> val && key[0] != ';' && split == "=")
            config[key] = val;

    #if defined(__linux__)
    if ( daemon(0, 0) ) return 1;
    #endif

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::Format, "%datetime{%Y-%M-%d %H:%m:%s.%g} (%thread) [%level] %msg" );
    defaultConf.setGlobally( el::ConfigurationType::Filename, log_path + "/backend-ctp.log" );
    defaultConf.setGlobally( el::ConfigurationType::MaxLogFileSize, "2097152" );
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "0" );
    el::Loggers::reconfigureLogger("default", defaultConf);
    logger = el::Loggers::getLogger("default");
    el::Helpers::setThreadName("main");
    logger->info("服务重新启动,连接 redis %v:%v", config["host"], stoi( config["port"] ));
    ConnectionOptions connection_options;
    connection_options.host = config["host"];
    connection_options.port = stoi( config["port"] );
    connection_options.db = stoi( config["db"] );
    cout << "connect to redis...";
    Redis new_pub = Redis(connection_options);
    publisher = &new_pub;
    Subscriber subscriber = publisher->subscriber();
    cout << "done!" << endl;
    BROKER_ID = config["broker"];
    INVESTOR_ID = config["investor"];
    PASSWORD = config["passwd"];
    APPID = config["appid"];
    AUTHCODE = config["authcode"];
    USERINFO = config["userinfo"];
    IP_ADDRESS = config["ip"];
    MAC_ADDRESS = config["mac"];
    logger->info("连接交易服务器..");
    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi( trade_path.c_str() );   // 创建TradeApi
    auto *pTraderSpi = new CTraderSpi();
    pTraderApi->RegisterSpi(pTraderSpi);                                           // 注册事件类
    pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);                // 注册公有流
    pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);               // 注册私有流
    pTraderApi->RegisterFront( (char *) config["trade"].c_str() );     // connect
    logger->info("连接行情服务器..");
    pMdApi = CThostFtdcMdApi::CreateFtdcMdApi( md_path.c_str() );      // 创建MdApi
    CThostFtdcMdSpi *pMdSpi = new CMdSpi();
    pMdApi->RegisterSpi(pMdSpi);                                       // 注册事件类
    pMdApi->RegisterFront( (char *) config["market"].c_str() );        // connect

    logger->info("开启命令处理线程..");
    jthread command_handler(handle_command);
    subscriber.psubscribe(CHANNEL_REQ_PATTERN);
    subscriber.on_pmessage(handle_req_request);

    pTraderApi->Init();
    pMdApi->Init();

    jthread heart_beat([connection_options] (std::stop_token stop_token) {
        Redis beater = Redis(connection_options);
        while ( ! stop_token.stop_requested() ) {
            beater.setex("HEARTBEAT:BACKEND_CTP", 61, "1");
            this_thread::sleep_for(seconds(60));
        }
    });
    jthread comsume([&subscriber] (std::stop_token stop_token) {
        el::Helpers::setThreadName("redis");
        while ( ! stop_token.stop_requested() ) subscriber.consume();
    });
    logger->info("服务已启动.");
    pTraderApi->Join();
    pMdApi->Join();
    command_handler.join();
    logger->info("服务已退出.");
    return 0;
}
