#pragma once
#include "LRUCache.h"
#include <memory>
#include <functional>
#include <vector>
#include <algorithm>

// ================================================================
// ShardedCache<Key, Value> — 分片缓存
//
// 将总容量平摊到 N 个独立的 LRUCache 分片。
// 每个分片有自己的锁，不同 key 大概率落在不同分片，
// 从而大幅减少锁竞争。
//
// 路由规则：分片索引 = std::hash(key) % slices
// ================================================================
template <typename Key, typename Value>
class ShardedCache
{
public:
    // ============================================================
    // 构造函数
    //   totalCapacity —— 所有分片的总容量
    //   slices        —— 分片数量（建议取 CPU 核数或略大）
    // ============================================================
    ShardedCache(size_t totalCapacity, size_t slices = 8)
        : slices_(slices)
    {
        // 每个分片的容量 = 总容量 / 分片数（至少 1）
        size_t perSlice = std::max(size_t(1), totalCapacity / slices);

        caches_.reserve(slices);
        for (size_t i = 0; i < slices; ++i)
        {
            caches_.push_back(std::make_unique<LRUCache<Key, Value>>(perSlice));
        }
    }

    // 查询缓存
    bool get(const Key& key, Value& value)
    {
        return getSlice(key)->get(key, value);
    }

    // 写入缓存
    void put(const Key& key, const Value& value)
    {
        getSlice(key)->put(key, value);
    }

private:
    // 根据 key 的哈希值定位到对应的分片
    LRUCache<Key, Value>* getSlice(const Key& key)
    {
        size_t hashValue = std::hash<Key>{}(key);
        size_t index     = hashValue % slices_;
        return caches_[index].get();
    }

private:
    size_t slices_;
    std::vector<std::unique_ptr<LRUCache<Key, Value>>> caches_;
};
