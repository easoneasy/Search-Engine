#pragma once
#include <list>
#include <cstddef>
#include <unordered_map>
#include <mutex>

// ================================================================
// LRUCache<Key, Value> — 线程安全的最近最少使用缓存
//
// 特性：
//   - get(key, value)  → O(1)，命中时刷新访问时间
//   - put(key, value)  → O(1)，容量满时淘汰最久未访问的条目
//   - 内部使用 std::mutex 保证线程安全
//
// 实现原理：
//   - std::list<Key> 维护访问顺序（头部=最近，尾部=最旧）
//   - std::unordered_map<Key, pair<Value, list::iterator>> 实现 O(1) 查找
//   - 每次 get/put 命中时 splice 到链表头部（LRU 语义）
// ================================================================
template <typename Key, typename Value>
class LRUCache
{
public:
    // 构造函数：指定缓存最大容量
    explicit LRUCache(size_t capacity)
        : capacity_(capacity)
    {
    }

    // ============================================================
    // get：从缓存中读取数据
    // 参数：key 缓存键, value [out] 缓存值
    // 返回：true=命中, false=未命中
    // 副作用：命中时将该 key 提升为最近访问
    // ============================================================
    bool get(const Key& key, Value& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end())
        {
            return false;   // 未命中
        }

        // 命中：将 key 移到链表头部（标记为最近访问）
        // splice 是 O(1)，不拷贝数据，只是重新链接节点
        accessList_.splice(accessList_.begin(), accessList_, it->second.second);

        // 返回缓存的值
        value = it->second.first;
        return true;
    }

    // ============================================================
    // put：向缓存中写入数据
    // 逻辑：
    //   1. key 已存在 → 更新 value，刷新访问时间
    //   2. key 不存在 + 缓存未满 → 直接插入
    //   3. key 不存在 + 缓存已满 → 淘汰最久未访问，再插入
    // ============================================================
    void put(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);

        if (it != map_.end())
        {
            // key 已存在 → 更新
            it->second.first = value;
            accessList_.splice(accessList_.begin(), accessList_, it->second.second);
            return;
        }

        // key 不存在 → 需要插入
        if (map_.size() >= capacity_)
        {
            // 缓存已满 → 淘汰链表尾部（最久未访问）
            const Key& lruKey = accessList_.back();
            map_.erase(lruKey);
            accessList_.pop_back();
        }

        // 插入到链表头部
        accessList_.push_front(key);
        map_[key] = {value, accessList_.begin()};
    }

    // 返回当前缓存条目数
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    // 返回缓存最大容量
    size_t capacity() const
    {
        return capacity_;
    }

private:
    size_t capacity_;
    std::list<Key> accessList_;   // 头部=最近，尾部=最旧
    std::unordered_map<Key, std::pair<Value, typename std::list<Key>::iterator>> map_;
    mutable std::mutex mutex_;
};
