#pragma once
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstdint>
#include <iostream>

#define MAX_PACKAGE_SIZE 4096

enum TlvType : uint8_t
{
    TYPE_KEYWORD_RECOMMEND = 1,  // 关键词推荐
    TYPE_WEBPAGE_SEARCH = 2      // 网页搜索
};

namespace TlvProtocol
{
    // 编码：把type+value打包成网络字节流
    inline std::string encode(uint8_t type,const std::string &value)
    {
        // 计算value的长度
        uint32_t len = static_cast<uint32_t>(value.size());
        // 把长度转成网络字节序
        uint32_t netLen = htonl(len);
        // 拼包
        std::string packet;
        packet.reserve(1+4+len);
        // type
        packet.push_back(static_cast<char>(type));
        // length
        packet.append(reinterpret_cast<const char*>(&netLen),4);
        // value
        packet.append(value);
        return packet;
    }

    // 解码： 从字节流中提取一个完整的TLV包
    inline bool decode(std::string &buffer,uint8_t &type,std::string &value,bool *error = nullptr)
    {
        if(error) *error = false;

        // 头部5字节没到齐，等更多数据
        if(buffer.size() < 5)
        {
            return false;
        }
        // 解析type（第一个字节）
        type = static_cast<uint8_t>(buffer[0]);
        // 解析length (2-5字节，网络序 -> 主机序)
        uint32_t netLen;
        // 将buff.data的数据，从第一个字节开始，读取4个字节放入netLen中
        // （value的长度）
        memcpy(&netLen, buffer.data()+1, 4);
        // 网络字节序 -> 主机序
        uint32_t len = ntohl(netLen);
        // 避免恶意攻击发送过长的数据
        if (len > MAX_PACKAGE_SIZE) {
            // 报错并断开连接
            if(error) *error = true;
            // 调用方看到error = true 后断开连接
            return false;
        }
        // 数据包没收全，等更多数据
        if(buffer.size() < 5 + len)
        {
            return false;
        }
        // 提取value
        //将 buffer 中属于 Value 的片段赋值给输出变量 value。
        // buffer.data() + 5 表示跳过 5 字节的头部，指向 Value 的起始地址。
        // len 表示读取的字节长度。
        value.assign(buffer.data() + 5 ,len);
        // 从缓冲区中删除已消费的字节
        // 从缓冲区的指定位置开始删除 n 个字节
        buffer.erase(0,5+len);
        return true;
    }
};
