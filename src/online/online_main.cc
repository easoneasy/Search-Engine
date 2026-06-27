#include <iostream>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include "../../include/online/KeyRecommender.h"
#include "../../include/online/WebPageQuery.h"
#include "../../include/online/searchServer.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;
int main()
{
    EventLoop loop;

    InetAddress listenAddr(8888);
    searchServer server(&loop,listenAddr);
    server.start();
    loop.loop();

    // KeyRecommender kr;
    // // auto ret = kr.query("新闻");
    // // for(auto &r : ret)
    // // {
    // //     cout << r << endl;
    // // }
    // WebPageQuery wb;

    // wb.query("计算机");

    return 0;
}
