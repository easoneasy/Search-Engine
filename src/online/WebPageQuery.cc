#include "../../include/online/WebPageQuery.h"
#include "../../include/common/TextUtils.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <muduo/base/Logging.h>
#include <sstream>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utfcpp/utf8/checked.h>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

// 初始化网页库路径
WebPageQuery::WebPageQuery()
:pageLibPath_("data/pages.dat")
,tokenizer_()
{
    // 加载停用词
    stopWords_ = loadStopWords("data/stopwords/en_stopwords.txt");
    // 加载偏移库
    loadOffsetLib("data/offsets.dat");
    // 加载索引库
    loadInvertIndex("data/index.dat");
}


// 加载偏移库
void WebPageQuery::loadOffsetLib(const string &filename)
{
    ifstream ifs{filename};
    if(!ifs)
    {
        cerr << "open offset file failed" << endl;
        return;
    }

    // 读取
    int docid;
    long offset;
    long length;
    while(ifs >> docid >> offset >> length)
    {
        // OffsetInfo info;
        // info.offset = offset;
        // info.length = length;
        // offsetLib_[docid] = info;
        offsetLib_.emplace(docid,OffsetInfo{offset,length});
    }
    cout << "offset size = " << offsetLib_.size() << endl;
}

// 加载索引库
void WebPageQuery::loadInvertIndex(const string &filename)
{
    ifstream ifs{filename};
    if(!ifs)
    {
        cerr << "open offset file failed" << endl;
        return;
    }
    string line;
    while (getline(ifs,line))
    {
        // 构造字符串流
        istringstream iss(line);
        string word;
        iss >> word;

        int docid;
        double weight;
        // 循环读取文章id+权重
        while(iss >> docid >> weight)
        {
            // 放入invertIndex_中
            invertIndex_[word].emplace_back(docid, weight);
        }

    }
    cout << "invert Index size = " << invertIndex_.size()  << endl;
}

// 把用户输入的一句话转换成可以用于搜索的关键词集合
vector<string> WebPageQuery::cutQuery(const string &query)
{
    // 分词
    // 保存jieba分词结果
    vector<string> words;
    // MIX模式
    tokenizer_.Cut(query,words);
    // 最终返回的关键词
    vector<string> result;
    for(const auto &word : words)
    {
        // 跳过空字符
        if(word.empty())
        {continue;}
        // 去掉空格
        if(word == " ")
        {continue;}
        // 去掉换行
        if(word == "\n")
        {continue;}
        // 过滤停用词
        if(stopWords_.count(word))
        {continue;}
        // 保留有效关键词
        result.push_back(word);
    }
    // ------------打印测试-------------------
    std::cout << "Cut Result : ";
    for(const auto &word : result)
    {
        std::cout << "[" << word << "] ";
    }
    std::cout << std::endl;
    // ------------------------------------
    return result;
}

// 计算Query每个关键词的TF-IDF权重
unordered_map<string,double> WebPageQuery::calcQueryWeight(const vector<string> &words)
{
    // 保存最终的TF-IDF权重
    unordered_map<string, double> weights;
    // 统计每个词出现的次数
    unordered_map<string,int> tf;
    for(const auto &word:words)
    {
        ++tf[word];
    }

    // 文档总数 --> 偏移库的大小
    size_t totalDocs = offsetLib_.size();

    // 计算每个词的 TF-IDF
    for(const auto &item : tf)
    {
        const string &word = item.first;
        // 查询该词是否存在于倒排索引中
        auto it = invertIndex_.find(word);
        if(it == invertIndex_.end())
        {
            // 不存在
            continue;
        }
        // TF = 当前词出现次数 / 查询的总词数
        double tf = static_cast<double>(item.second) / words.size();
        // DF = 包含该词的文档数量
        size_t df = it->second.size();
        // IDF
        double idf = log(static_cast<double>(totalDocs) / (df + 1));
        // TF-IDF
        weights[word] = tf * idf;
    }
    // 归一化
    double norm = 0.0;
    for(const auto &[word,weight] : weights)
    {
        norm += weight *weight;
    }
    norm = sqrt(norm);

    // 如果模长为0，返回空
    if(norm == 0.0)
    {
        return weights;
    }
    // 每个权重除模长，向量长度为1
    for(auto &[word,weight] : weights)
    {
        weight /= norm;
    }
    return weights;
}


// 根据关键词，找出候选docid
vector<int> WebPageQuery::getCandidateDocIds(const vector<string> &words)
{
    // 空查询直接返回
    if(words.empty())
    {    return {};}

    // 处理第一个词
    set<int> result;
    // 使用迭代器查找word
    auto it = invertIndex_.find(words[0]);
    // 找不到返回尾迭代器
    if(it == invertIndex_.end())
    {
        return {};
    }
    // 找到了 遍历invertIndex的第二个数据
    for(const auto &e: it->second)
    {
        // 把invertIndex中vector的第一个结果docid插入到result中
        result.insert(e.first);
    }
    // 处理后续关键词
    for(size_t i = 1;i < words.size();++i)
    {
        auto iter = invertIndex_.find(words[i]);
        if(iter == invertIndex_.end())
        {
            return {};
        }
        set<int> current;
        for(const auto &e:iter->second)
        {
            current.insert(e.first);
        }
        set<int> temp;
        set_intersection(result.begin(),result.end(),current.begin(),current.end(),inserter(temp, temp.begin()));
        result = move(temp);
        if(result.empty())
        {
            return {};
        }
    }
    return vector<int>(result.begin(),result.end());

}


// 排序函数
// 对候选文档按余弦相似度排序
vector<QueryResult> WebPageQuery::rankPage(const vector<int> &docids,
    const unordered_map<string, double> &queryWeight)
{
    vector<QueryResult> results;
    // 遍历每个候选文档
    for(int docid : docids)
    {
        // 余弦相似度
        double similarity = 0.0;
        for(const auto &[word,qweight] : queryWeight)
        {
            // 在倒排索引库中找这个词
            auto it = invertIndex_.find(word);
            // 不存在
            if(it == invertIndex_.end())
            {continue;}

            // 遍历该词的倒排列表，找到当前docid的权重
            for(const auto &[id,docWeight] : it->second)
            {
                if(id == docid)
                {
                    // 相似度累加
                    similarity += qweight * docWeight;
                    break;
                }
            }
        }
        // 保存结果
        results.push_back({docid,similarity});
    }
    // 按相似度降序排序
    sort(results.begin(),results.end(),[](const QueryResult &a,const QueryResult &b)
        {
            return a.score > b.score;
        });
    return results;
}

// 根据docid从网页库中获取网页内容
WebPage WebPageQuery::getDocById(int docid)
{
    WebPage page;
    page.docid = docid;

    // 从偏移库中查找位置信息
    auto it = offsetLib_.find(docid);
    if(it == offsetLib_.end())
    {
        cerr << " docid not found in offset : " << docid << endl;
        return page;
    }

    // 文档在网页库中的起始字节位置
    long offset = it->second.offset;
    // 文档的字节长度
    long length = it->second.length;

    // 打开网页库文件
    ifstream ifs(pageLibPath_);
    if(!ifs)
    {
        cerr << " connot open page lib : " << pageLibPath_ << endl;
        return page;
    }

    // 定位到指定偏移量
    ifs.seekg(offset,ios::beg);
    // 读取指定长度的内容
    string xmlBlock(length,'\0');
    ifs.read(&xmlBlock[0], length);
    // 解析XML
    // 提取 <title>
    auto titleStart = xmlBlock.find("<title>");
    auto titleEnd = xmlBlock.find("</title>");
    if(titleStart != string::npos && titleEnd != string::npos)
    {
        // 跳过<title> 这7个字符
        titleStart +=7;
        page.title = xmlBlock.substr(titleStart,titleEnd - titleStart);
    }
    // 提取<link>
    auto linkStart = xmlBlock.find("<link>");
    auto linkEnd = xmlBlock.find("</link>");
    if(linkStart != string::npos && linkEnd != string::npos)
    {
        linkStart += 6;
        page.link = xmlBlock.substr(linkStart,linkEnd-linkStart);
    }
    // 提取<content>
    auto contentStart = xmlBlock.find("<content>");
    auto contentEnd = xmlBlock.find("</content>");
    if(contentStart != string::npos && contentEnd != string::npos)
    {
        contentStart += 9;
        page.content = xmlBlock.substr(contentStart,contentEnd-contentStart);
    }
    return page;
}

// 在正文中找到第一个关键词出现的位置，截取前后段文字生成摘要
// 生成摘要
string WebPageQuery::getAbstract(const string &content,const vector<string> &words)
{
    // 将content 按utf8 字符拆成单个字
    auto chars = splitChinese(content);

    // 如果正文太短，直接返回全部内容
    if(chars.size() <= 80)
    {
        return content;
    }

    // 在正文中查找第一个关键词出现的位置
    size_t foundPos = string::npos;
    for(const auto &word:words)
    {
        // 找到word在content中的字节位置
        size_t bytePos = content.find(word);
        if(bytePos == string::npos)
        {
            continue;
        }
        // 把字节位置替换成字符位置，统计该字节之前有多少个UTF-8字符
        size_t charPos = 0;
        const char *p = content.c_str();
        const char *end = p + bytePos;
        while(p < end)
        {
            utf8::next(p,end);
            ++charPos;
        }

        if(foundPos == string::npos || charPos < foundPos)
        {
            foundPos = charPos;
        }
    }
    // 如果没找到任何关键词，取正文前80个字符
    if(foundPos == string::npos)
    {
        string result;
        for(std::size_t i = 0; i < min((size_t)80,chars.size()) ;++i)
        {
            result += chars[i];
        }
        return result + "...";
    }

    // 以关键词为中心，前后各取40个字符
    size_t start = (foundPos >= 40) ? (foundPos - 40 ) : 0;
    size_t end = min(foundPos + 40 , chars.size());

    // 如果还能往前扩展 (start > 0 ) , 前面 "..."
    string result;
    string prefix = (start > 0) ? "..." : "";
    // 向后扩展
    string suffix = (end < content.size()) ? "..." : "";
    for(size_t i = start ; i< end ; ++i)
    {
        result+= chars[i];
    }
    return prefix + result + suffix;
}


// 查询
// 分词-> 根据关键词找docid -> 获取网页内容 -> 排序 -> 输出
string WebPageQuery::query(const string &sentence)
{
    // cout << " 查询 : " << sentence << endl;
    LOG_INFO << "Query :" << sentence;

    // 分词
    vector<string> keywords = cutQuery(sentence);
    if(keywords.empty())
    {
        // cout << "未识别到有效关键词" << endl;
        return R"({"error":" 未识别到有效关键词 "})";
    }

    // 计算查询权重
    unordered_map<string, double> queryWeight = calcQueryWeight(keywords);
    if(queryWeight.empty())
    {
        // cout << "关键词在索引库中未找到匹配" << endl;
        return R"({"error":" 关键词在索引库中未找到匹配 "})";;
    }

    // 找候选文档
    vector<int> candidateIds = getCandidateDocIds(keywords);
    if(candidateIds.empty())
    {
        cout << "没有同时包含所有关键词的文档" << endl;
        return R"({"error":" 没有同时包含所有关键词的文档 "})";
    }

    // 排序
    vector<QueryResult> ranked = rankPage(candidateIds, queryWeight);

    // 构建JSON返回
    json response;
    response["total"] = ranked.size();

    json results = json::array();
    // 输出TOP5 结果
    int topN = min(5,(int)ranked.size());
    for(int i = 0 ; i < topN; ++i)
    {
        int docid = ranked[i].docid;
        double score = ranked[i].score;

        // 读取网页内容
        WebPage page = getDocById(docid);
        // 生成摘要
        string abstract = getAbstract(page.content, keywords);

        json item;
        item["rank"] = i+1;
        item["docid"] = docid;
        item["score"] = score;
        item["title"] = page.title;
        item["link"] = page.link;
        item["abstract"] = abstract;
        results.push_back(item);
        // // 打印结果
        // cout << "------------------" << endl;
        // cout << "排名： " << (i+1) << "相关度： " << score << "文档id" << docid << endl;
        // cout << "标题：" << page.title << endl;
        // cout << "链接：" << page.link << endl;
        // cout << "摘要：" << abstract << endl;
    }
    // cout << "共找到 ： " << ranked.size() << "条结果,显示前 " << topN << "条。" << endl;
    // // 返回topN结果
    // vector<QueryResult> topResults;
    // for(int i =0 ; i < topN; ++i)
    // {
    //     topResults.push_back(ranked[i]);
    // }
    response["result"] = results;
    LOG_INFO << "Found " << ranked.size() << " results, return top :" << topN;
    return response.dump(2);
}

















/*
// 根据docid从倒排索引中找到关键词在该网页的权重
vector<double> WebPageQuery::getDocVector(int docid,const vector<string> &words)
{
    vector<double> vec;
    for(const auto &word : words)
    {
        double weight = 0.0;
        auto it = invertIndex_.find(word);
        if(it != invertIndex_.end())
        {
            for(const auto &item : it->second)
            {
                if(item.first == docid)
                {
                    weight = item.second;
                    break;
                }
            }
        }
        vec.push_back(weight);
    }
    return vec;
}

// 计算关键词与网页的相关度
vector<CandidateDoc> WebPageQuery::caculateCandidateDocs(const vector<string> &words)
{
    vector<CandidateDoc> result;
    // 获取候选网页id
    auto docIds = getCandidateDocIds(words);

    for(auto docid : docIds)
    {
        CandidateDoc doc;
        doc.docid = docid;
        // 计算余弦相似度
        doc.similarity = 0.0;
        // docid + 相似度
        result.push_back(doc);
    }
    return result;
}*/
