#pragma once
#include "KeywordProcessor.h"
#include "../../include/common/TextUtils.h"
#include "../../include/common/DirectoryScanner.h"
#include "cppjieba/Jieba.hpp"
#include "simhash/Simhasher.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <set>



class PageProcessor
{
    public:
        PageProcessor();
        void process(const std::string& dir);

    // private:
        /// 解析 dir 目录下的 xml 文件，提取文档，放入 documents_中
        void extract_documents(const std::string& dir);
        /// 依据 SimHash 算法对 documents_去重
        void deduplicate_documents();
        /// 构建网页库和网页偏移库
        void build_pages_and_offsets(const std::string& pages, const std::string& offsets);
        /// 构建倒排索引库
        void build_inverted_index(const std::string& filename);
    private:
        struct Document {
            int id;
            std::string link;
            std::string title;
            std::string content;
        };
    private:
        cppjieba::Jieba tokenizer_;
        // Simhasher对象 只创建一个
        simhash::Simhasher hasher_;

        std::unordered_set<std::string> stopWords_; // 使用 set, 而非 vector, 是为了方便查找
        // 存放原始数据
        std::vector<Document> documents_;
        // 存放去重数据
        std::vector<Document> uniqueDocuments_;
        // 存放倒排结果
        std::unordered_map<std::string, std::vector<std::pair<int, double>>> invertedIndex_;

};
