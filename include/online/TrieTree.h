#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <algorithm>

// 前缀搜索结果
struct TrieResult
{
    std::string word;
    int frequency;
};

class TrieTree
{
public:
    TrieTree() : root_(std::make_unique<TrieNode>()) {}

    // 插入一个词和词频
    void insert(const std::string& word, int frequency)
    {
        TrieNode* node = root_.get();
        for (char ch : word)
        {
            if (node->children.find(ch) == node->children.end())
                node->children[ch] = std::make_unique<TrieNode>();
            node = node->children[ch].get();
        }
        node->isEndOfWord = true;
        node->frequency   = frequency;
        node->word        = word;
        ++totalWords_;
    }

    // 前缀搜索，返回词频最高的 topK 个结果
    std::vector<TrieResult> searchByPrefix(const std::string& prefix,
                                            int topK = 10) const
    {
        // 1. 走到前缀对应节点
        TrieNode* node = root_.get();
        for (char ch : prefix)
        {
            auto it = node->children.find(ch);
            if (it == node->children.end()) return {};
            node = it->second.get();
        }

        // 2. 收集该节点下所有完整词
        std::vector<TrieResult> allWords;
        collectWords(node, allWords);

        // 3. 大顶堆按词频排序
        auto cmp = [](const TrieResult& a, const TrieResult& b) {
            return a.frequency < b.frequency;
        };
        std::priority_queue<TrieResult, std::vector<TrieResult>, decltype(cmp)> pq(cmp);
        for (const auto& r : allWords) pq.push(r);

        // 4. 取 Top K
        std::vector<TrieResult> result;
        while (!pq.empty() && (int)result.size() < topK)
        {
            result.push_back(pq.top());
            pq.pop();
        }
        return result;
    }

    int size() const { return totalWords_; }

private:
    struct TrieNode
    {
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
        bool isEndOfWord = false;
        int  frequency   = 0;
        std::string word;
    };

    void collectWords(const TrieNode* node, std::vector<TrieResult>& results) const
    {
        if (!node) return;
        if (node->isEndOfWord)
            results.push_back({node->word, node->frequency});
        for (const auto& [ch, child] : node->children)
            collectWords(child.get(), results);
    }

    std::unique_ptr<TrieNode> root_;
    int totalWords_ = 0;
};
