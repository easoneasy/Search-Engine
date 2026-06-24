#include "../../include/offline/PageProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <tinyxml2.h>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace tinyxml2;


// 构造函数
PageProcessor::PageProcessor()
:tokenizer_()
,hasher_()
{
    stopWords_ = loadStopWords("data/stopwords/en_stopwords.txt");
}

// 入口函数
void PageProcessor::process(const std::string& dir)
{
    // 解析 dir 目录下的 xml 文件，提取文档，放入 documents_中
    extract_documents(dir);
    /// 依据 SimHash 算法对 documents_去重
    deduplicate_documents();
    /// 构建网页库和网页偏移库
    build_pages_and_offsets("data/pages.dat", "data/offsets.dat");
    // 建立倒排索引库
    build_inverted_index("data/index.dat");

}
// 清洗html标签
string clean_html(string text)
{
    // 去标签
    regex tag("<[^>]*>");
    text = regex_replace(text,tag,"");
    // 去html实体
    text = regex_replace(text,regex("&nbsp;"),"");
    return text;
}

// xml->documents对象
void PageProcessor::extract_documents(const std::string& dir)
{
    // 获取语料库的路径
    auto files = DirectoryScanner::scan(dir);
    int docId = 1;
    // 处理每个file文件
    for(const auto &file : files)
    {
        // 解析xml
        XMLDocument xml;
        // 将xml读取到内存
        if(xml.LoadFile(file.c_str()) != XML_SUCCESS)
        {
            cerr << "load xml failed file :" << file << endl;
            continue;
        }

        auto *rss = xml.FirstChildElement("rss");
        if(!rss)
        {
            continue;
        }
        auto *channel = rss->FirstChildElement("channel");
        if(!channel)
        {
            continue;
        }
        auto *item = channel->FirstChildElement("item");
        // 遍历所有item
        for(;item; item = item->NextSiblingElement("item"))
        {
            Document doc;
            doc.id = docId++;
            // title
            XMLElement *titleNode = item->FirstChildElement("title");
            if(titleNode && titleNode->GetText())
            {
                doc.title = clean_html(titleNode->GetText());
            }
            // link
            XMLElement *linkNode = item->FirstChildElement("link");
            if(linkNode && linkNode->GetText())
            {
                doc.link = clean_html(linkNode->GetText());
            }
            // content
            XMLElement *contentNode = item->FirstChildElement("content");
            if(contentNode && contentNode->GetText())
            {
                doc.content = clean_html(contentNode->GetText());
            }else{
                XMLElement *descNode = item->FirstChildElement("description");
                if(descNode && descNode->GetText())
                {
                    doc.content = clean_html(descNode->GetText());
                }else{
                    continue;
                }
            }
            // 将doc对象放入vector
            documents_.push_back(doc);
        }
    }
    cout << "document count = " << documents_.size() << endl;
}

// 去重
void PageProcessor::deduplicate_documents()
{
    // 准备指纹库
    vector<uint64_t> fingerPrint;
    // 遍历每篇文档
    for(auto &doc : documents_)
    {
        // 空变量
        uint64_t hash;
        int topN = max(5,min(200,(int)doc.content.size()/120));
        // hash内就有了当前文档的Simhash指纹
        hasher_.make(doc.content,topN,hash);
        // 和已有指纹比较
        bool duplicate = false;
        for(auto oldHash : fingerPrint)
        {
            // 判断新hash和已有hash是否相同
            // 相同则跳出循环
            if(simhash::Simhasher::isEqual(hash, oldHash))
            {
                duplicate = true;
                break;
            }
        }
        // 不同则放入已有指纹库 、 存入去重数据
        if(!duplicate)
        {
            fingerPrint.push_back(hash);
            uniqueDocuments_.push_back(doc);
        }
    }
}


// 构建网页库和网页偏移库
void PageProcessor::build_pages_and_offsets(const std::string& pages, const std::string& offsets)
{
    ofstream pageFile(pages);
    ofstream offsetFile(offsets);

    if(!pageFile || !offsetFile)
    {
        cerr << "open file failed" << endl;
        return;
    }

    for(const auto &doc :uniqueDocuments_)
    {
        // 序列化为XML格式
        ostringstream oss;
        oss << "<doc>\n"
        << "<id>" << doc.id << "</id>\n"
        << "<title>" << doc.title << "</title>\n"
        << "<link>" << doc.link << "</link>\n"
        << "<content>" << doc.content << "</content>\n"
        << "</doc>\n";
        // doc1开始的位置为0 ，本篇文件的开始位置
        // 下一篇文件的开始位置
        long offset = pageFile.tellp();
        string docStr = oss.str();
        // 把xml格式的字符串写入文件
        pageFile << docStr;

        // 下一篇的开始位置
        long length = docStr.size();

        offsetFile << doc.id << " " << offset << " " << length << "\n";
    }
    cout << "pages and offsets success" << endl;
}


// 构建倒排索引库
void PageProcessor::build_inverted_index(const string& filename)
{
    // 存放每篇文章的tf
    unordered_map<int,unordered_map<string,double>> tfMap_;
    // TF--统计一个词在文章中出现的频率
    // 遍历每篇去重后的文档
    for(const auto &doc : uniqueDocuments_)
    {
        // 分词
        vector<string> words;
        tokenizer_.Cut(doc.content,words);
        vector<string> vaildWords;
        // 过滤无效词和停用词
        for(auto &word : words)
        {
            if(word.size() == 0) // 空字符串
            {
                continue;
            }
            if(stopWords_ .count(word)) //停用词
            {
                continue;
            }
            if(!isChineseWord(word))  // 非中文
            {
                continue;
            }
            vaildWords.push_back(word);
        }
        // 获取总词数
        size_t totalWords = vaildWords.size();
        // cout << "totalWords" << totalWords << endl;
        if(totalWords == 0)
        {
            continue;
        }
        // 统计词出现的次数
        unordered_map<string, int> countMap;
        for(auto &word : vaildWords)
        {
            ++countMap[word];
        }
        // 计算TF = count / totalWords
        unordered_map<string, double> tf;
        for(const auto &[word,count] :countMap)
        {
            tf[word] = static_cast<double>(count) / totalWords;
        }
        // 保存到tfMap

        tfMap_[doc.id] = std::move(tf);

    }
    // 存放df
    unordered_map<string, int> dfMap_;
    // 获取文档的id，tf的内容
    for(const auto & [id,tf] : tfMap_)
    {
        // 获取tf里的内容
        for(const auto &[word,value] :tf)
        {
            // 放在df中，统计数目
            ++dfMap_[word];
        }
    }

    // 计算IDF
    unordered_map<string, double> idfMap_;
    for(const auto &[word,df] : dfMap_)
    {
        double idf = log(static_cast<double>(uniqueDocuments_.size()) / df+1);
        idfMap_[word] = idf;
    }
    // TF-IDF
    unordered_map<int, unordered_map<string,double>> weightMap_;
    for(const auto &[id,tf] : tfMap_)
    {
        unordered_map<string, double> weights;
        for(const auto &[word,tfValue] : tf)
        {
            double idfValue = idfMap_[word];
            weights[word] = tfValue * idfValue;
        }
        weightMap_[id] = std::move(weights);
    }
    // 归一化
    for(auto &[id,weights] : weightMap_)
    {
        double norm = 0.0;
        // 求平方和
        for(const auto &[word,weight] : weights)
        {
            norm += weight * weight;
        }
        // 开根号
        // 向量模长
        norm = sqrt(norm);
        if(norm == 0)
        {
            continue;
        }
        // 归一化
        for(auto &[word,weight] : weights)
        {
            // 每个权重 除 模长
            weight /= norm;
        }
    }
    // 倒排索引
    for(const auto &[id,weights] : weightMap_)
    {
        for(const auto &[word,weight] : weights)
        {
            invertedIndex_[word].push_back({id,weight});
        }
    }
    // 排序
    for(auto &[word,docList] :invertedIndex_)
    {
        sort(docList.begin(),docList.end(),[](const auto &lhs,const auto &rhs)
            {
                return lhs.second > rhs.second;
            });
    }
    // 写入索引文件
    ofstream ofs(filename);
    for(const auto &[word,docList] :invertedIndex_)
    {
        ofs << word;
        for(const auto &[id,weight] : docList)
        {
            ofs << " " << id << " " << weight;
        }
        ofs << '\n';
    }
    cout << "reverse index.dat success!" << endl;
}
