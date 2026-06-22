#pragma once
#include "../../include/common/DirectoryScanner.h"
#include <cppjieba/Jieba.hpp>
#include <set>
#include <unordered_map>
#include <unordered_set>
class KeyWordProcessor {
    public:
        KeyWordProcessor();

        // chDir: 中文语料库
        // enDir: 英文语料库
        void process(const std::string& chDir, const std::string& enDir);

    // private:
        void create_cn_dict(const std::string& dir, const std::string& outfile);
        void build_cn_index(const std::string& dict, const std::string& index);

        void create_en_dict(const std::string& dir, const std::string& outfile);
        void build_en_index(const std::string& dict, const std::string& index);

    private:
        cppjieba::Jieba tokenizer_;
        // 保存英文停用词
        std::unordered_set<std::string> enStopWords_;
        // 保存中文停用词
        std::unordered_set<std::string> chStopWords_;
};
