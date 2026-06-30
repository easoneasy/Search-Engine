#pragma once
#include "ShardedCache.h"
#include "TlvProtocol.h"
#include "KeyRecommender.h"
#include "WebPageQuery.h"
#include "HotSearchTracker.h"
#include "LRUCache.h"
#include <memory>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <unordered_map>

using KeyCache = LRUCache<std::string, std::string>;
using PageCache = LRUCache<std::string, std::string>;


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

    // 关键词推荐缓存 key = 查询词，value = JSON结果
    // 容量1024
    // KeyCache keyCache_{1024};
    ShardedCache<std::string, std::string> keyCache_{1024,8};

    // 查询结果缓存，容量512
    // PageCache pageCache_{521};
    ShardedCache<std::string, std::string> pageCache_{512,8};
    // 热门文档缓存，容量1024
    DocCache docCache_{1024};

    // 热搜追踪器（点击权重1.0，浏览时长权重0.1）
    HotSearchTracker hotTracker_{1.0, 0.1};
};
