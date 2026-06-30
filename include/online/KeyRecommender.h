#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include "TrieTree.h"

class KeyRecommender
{
public:
    // 构造函数
    KeyRecommender();
    // 查询函数
    // std::vector<std::string> query(const std::string &keyword);
    std::string query(const std::string &keyword);

    // 前缀搜索（Trie树，用于前端实时建议）
    std::string queryByPrefix(const std::string& prefix, int topK = 10);
private:
    // 加载词典文件
    void loadDict(const std::string &filename);
    // 加载索引文件
    void loadIndex(const std::string &filename);
    // 计算编辑距离函数
    int editDistance(const std::string &lhs,const std::string &rhs);

private:
    // 存放词典
    std::vector<std::pair<std::string,int>> dict_;
    // 存放索引
    std::unordered_map<std::string, std::set<int>> index_;

    // 前缀搜索 Trie 树
    TrieTree trie_;
};

// 结构体
struct Candidate
{
    std::string word;   // 单词
    int frequency;      // 词频
    int distance;       // 编辑距离
};
