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
#include <sys/stat.h>
#include <unistd.h>
#include "TraderSpi.h"
#include "MdSpi.h"
#include "global.h"

using namespace std;
using namespace redox;

int main(int argc, char *argv[]) {
    char exe_path[512] = {0};
    if ( readlink( "/proc/self/exe", exe_path, 512 ) < 0) {
        cout << "readlink falied!" << endl;
        return 1;
    }
    string path(exe_path), config_file;
    path = path.substr(0, path.find_last_of("/") + 1);
    mkdir( ( path + "trade" ).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    mkdir( ( path + "md" ).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    if ( argc > 1 ) {
        config_file = argv[1];
    } else {
        config_file = path + "config.ini";
    }
    cout << "config-file are: " << config_file << endl;
    std::string 	line, key, split, val;
    std::ifstream 	ifs(config_file);
    std::map<std::string, std::string> config;
    while ( std::getline(ifs, line) )
        if ( std::stringstream(line) >> key >> split >> val && key[0] != ';' && split == "=")
            config[key] = val;

    if ( ! publisher.connect( config["host"] ) ) return 1;
    if ( ! subscriber.connect( config["host"], std::stoi( config["port"] ) ) ) return 1;

    BROKER_ID = config["broker"];
    INVESTOR_ID = config["investor"];
    PASSWORD = config["passwd"];
    IP_ADDRESS = config["ip"];
    MAC_ADDRESS = config["mac"];

    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi( (path+"trade/").c_str() );   // 创建TradeApi
    CTraderSpi *pTraderSpi = new CTraderSpi();
    pTraderApi->RegisterSpi(pTraderSpi);                       // 注册事件类
    pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);        // 注册公有流
    pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);       // 注册私有流
    pTraderApi->RegisterFront( (char *) config["trade"].c_str() );     // connect
    pTraderApi->RegisterFront( (char *) config["trade_off"].c_str() ); // connect

    pMdApi = CThostFtdcMdApi::CreateFtdcMdApi( (path+"md/").c_str() );                  // 创建MdApi
    CThostFtdcMdSpi *pMdSpi = new CMdSpi();
    pMdApi->RegisterSpi(pMdSpi);                               // 注册事件类
    pMdApi->RegisterFront( (char *) config["market"].c_str() );        // connect
    pMdApi->RegisterFront( (char *) config["market_off"].c_str() );    // connect

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
