#pragma once
#include <cppjieba/Jieba.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


struct QueryResult
{
    int docid;      // 网页编号
    double score;   // 相关度
};

struct OffsetInfo
{
    long offset;
    long length;
};

struct WebPage
{
    int docid;
    std::string title;
    std::string link;
    std::string content;
};

class WebPageQuery
{
public:
    // 构造函数
    WebPageQuery();

    // 查询
    // 分词-> 根据关键词找docid -> 获取网页内容 -> 排序 -> 输出
    // std::vector<QueryResult> query(const std::string &sentence);
    std::string query(const std::string &sentence);

private:
    // 加载偏移库
    void loadOffsetLib(const std::string &filename);
    // 加载倒排索引库
    void loadInvertIndex(const std::string &filename);

    // 把用户输入的一句话转换成可以用于搜索的关键词集合
    std::vector<std::string> cutQuery(const std::string &query);
    // 计算Query每个关键词的TF-IDF权重
    std::unordered_map<std::string, double> calcQueryWeight(const std::vector<std::string> &words);

    // 根据关键词，从倒排索引库中找出候选docid
    std::vector<int> getCandidateDocIds(const std::vector<std::string> &words);
    // 排序函数
    std::vector<QueryResult> rankPage(const std::vector<int> &docids,
        const std::unordered_map<std::string, double> &queryWeight);

    // 根据docid获取网页内容
    WebPage getDocById(int docid);
    // 生成摘要
    std::string getAbstract(const std::string &content,const std::vector<std::string> &words);

private:

    cppjieba::Jieba tokenizer_;
    // 存放停用词
    std::unordered_set<std::string> stopWords_;
    // 存储网页库路径
    std::string pageLibPath_;
    // 存储网页偏移库
    std::unordered_map<int, OffsetInfo> offsetLib_;
    // 存储倒排索引库
    std::unordered_map<std::string, std::vector<std::pair<int, double>>> invertIndex_;
};
