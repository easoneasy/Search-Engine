#pragma once
#include "TlvProtocol.h"
#include "KeyRecommender.h"
#include "WebPageQuery.h"
#include <memory>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <string>
#include <unordered_map>

class searchServer
{
public:
    searchServer(muduo::net::EventLoop *loop,const muduo::net::InetAddress& listenAddr);
    void start();

private:

    // 处理连接建立和断开事件
    void onConnection(const muduo::net::TcpConnectionPtr &conn);

    // 处理消息事件
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
        muduo::net::Buffer *buf,muduo::Timestamp receiveTime);

    // 处理关键词推荐
    std::string handleKeyRecommend(const std::string &keyword);
    // 处理网页搜索
    std::string handleWebPageSearch(const std::string &query);

private:
    // TCP服务器
    muduo::net::TcpServer server_;

    // 每个连接的缓冲区，key = 连接名， value = 未处理完的数据
    std::unordered_map<std::string, std::string> connectionBufer_;

    // 查询处理器，启动时实例化，加载离线数据
    std::unique_ptr<KeyRecommender> keyRecommender_;
    std::unique_ptr<WebPageQuery> webPageQuery_;
};
