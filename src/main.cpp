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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include "TraderSpi.h"
#include "MdSpi.h"
#include "global.h"

using namespace std;
using namespace redox;

int main(int argc, char *argv[]) {
    string config_file;
    char exe_path[512] = {0};
    readlink("/proc/self/exe", exe_path, 512);
    string path(exe_path);
    path = path.substr(0, path.find_last_of("/")+1);
    if (argc > 1) {
        config_file = argv[1];
    } else {
        config_file = path + "config.ini";
    }
    cout << "config-file are: " << config_file << endl;
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(config_file, pt);
    if (!publisher.connect(pt.get<std::string>("redis.host"), pt.get<int>("redis.port"))) return 1;
    if (!subscriber.connect(pt.get<std::string>("redis.host"), pt.get<int>("redis.port"))) return 1;

    BROKER_ID = pt.get<std::string>("sim.broker");
    INVESTOR_ID = pt.get<std::string>("sim.investor");
    PASSWORD = pt.get<std::string>("sim.passwd");
    IP_ADDRESS = pt.get<std::string>("host.ip");
    MAC_ADDRESS = pt.get<std::string>("host.mac");

    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi((path+"trade/").c_str());    // 创建TradeApi
    CTraderSpi *pTraderSpi = new CTraderSpi();
    pTraderApi->RegisterSpi(pTraderSpi);                        // 注册事件类
    pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);        // 注册公有流
    pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);        // 注册私有流
    pTraderApi->RegisterFront((char *) pt.get<std::string>("sim.trade").c_str());    // connect

    pMdApi = CThostFtdcMdApi::CreateFtdcMdApi((path+"md/").c_str());                // 创建MdApi
    CThostFtdcMdSpi *pMdSpi = new CMdSpi();
    pMdApi->RegisterSpi(pMdSpi);                                // 注册事件类
    pMdApi->RegisterFront((char *) pt.get<std::string>("sim.market").c_str());    // connect

    std::thread command_handler(handle_command);
    subscriber.psubscribe(CHANNEL_REQ_PATTERN, handle_req_request);

    pTraderApi->Init();
    pMdApi->Init();

    pMdApi->Join();
    pTraderApi->Join();

    publisher.disconnect();
    subscriber.disconnect();
    keep_running = false;
    command_handler.join();

    return 0;
}
