#include "../../include/online/searchServer.h"
#include <charconv>
#include <cppjieba/Unicode.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <string>

using namespace std;
using namespace placeholders;


searchServer::searchServer(muduo::net::EventLoop *loop,const muduo::net::InetAddress &listenAddr)
:server_(loop, listenAddr, "SearchServer")
{
    // 注册回调
    server_.setConnectionCallback(
        std::bind(&searchServer::onConnection,this,_1));
    server_.setMessageCallback(
        std::bind(&searchServer::onMessage,this,_1,_2,_3));

    // 启动时一次性加载离线数据
    LOG_INFO << "loading KeyRecommender data...";
    keyRecommender_ = make_unique<KeyRecommender>();
    LOG_INFO << "loading WebPageQuery data...";
    webPageQuery_ = make_unique<WebPageQuery>();
    LOG_INFO << "All data loaded";
}

void searchServer::start()
{
    server_.start();
    LOG_INFO << "SearchServer started , listening on port" << server_.ipPort();
}


// 处理关键词推荐
string searchServer::handleKeyRecommend(const string &keyword)
{
    LOG_INFO << "KeyRecommend query : " << keyword;
    // auto results = keyRecommender_->query(keyword);
    return keyRecommender_->query(keyword);
    // // 临时： 用纯文本拼接结果  --> 下一步要改成json
    // string output;
    // for(size_t i = 0; i < results.size(); ++i)
    // {
    //     if(i > 0)
    //     {output += "\n";}
    //     output += to_string(i+1) + ". " + results[i];
    // }
    // return output.empty() ? "No results." : output ;
}

// 处理网页搜索
string searchServer::handleWebPageSearch(const string &query)
{
    LOG_INFO << "WebPage query :" << query;

    // 调用WebPageQuery::query  --> 下一步改为json
    // auto results = webPageQuery_->query(query);
    return webPageQuery_->query(query);
    // // 临时：用纯文本拼接结果
    // string output;
    // for(size_t i =0; i< results.size();++i)
    // {
    //     if(i > 0)
    //     {output += "\n";}
    //     output += "[" +to_string(i+1) + "]"
    //         + "docid = " + to_string(results[i].docid)
    //         + "score = " + to_string(results[i].score);
    // }
    // return output.empty() ? "No results." : output;
}

// 连接建立/断开时调用
void searchServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if(conn->connected())
    {
        LOG_INFO << "New connection from " << conn->peerAddress().toIpPort();
        // 为这个连接创建一个空的缓冲区（用于粘包处理）
        connectionBufer_[conn->name()].clear();

    }else{
        LOG_INFO << "connection closed " << conn->peerAddress().toIpPort();
        // 断开时清理缓冲区
        connectionBufer_.erase(conn->name());
    }
}


// 处理消息事件
void searchServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
    muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
{
    LOG_INFO << " onMessage start ..." + receiveTime.toFormattedString();
    // 把收到的数据追加到该连接的缓冲区
    string &connBuf = connectionBufer_[conn->name()];
    connBuf.append(buf->retrieveAllAsString());

    // 循环拆包
    uint8_t type;
    string value;
    // 防止恶意攻击
    bool error = false;
    while(TlvProtocol::decode(connBuf, type, value, &error))
    {

        // 根据type  路由到不同的处理器
        string response;
        switch (type)
        {
            case TYPE_KEYWORD_RECOMMEND:
            response = handleKeyRecommend(value);
            break;

            case TYPE_WEBPAGE_SEARCH:
            response = handleWebPageSearch(value);
            break;

            default:
            LOG_WARN << "Unknow TLV type : " << static_cast<int>(type);
            response = "ERROR : unknow type";
            break;
        }
        if(error)
        {
            LOG_WARN << "Invalied packet from :" << conn->peerAddress().toIpPort()
                << ", force closing connection";
            // 强制关闭
            conn->forceClose();
        }
        // 把响应用TLV编码后发给客户端
        string packet = TlvProtocol::encode(type, response);
        conn->send(packet);
    }
}
