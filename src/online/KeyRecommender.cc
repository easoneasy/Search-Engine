#include "../../include/common/TextUtils.h"
#include "../../include/online/KeyRecommender.h"
#include <cppjieba/limonp/StringUtil.hpp>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <utfcpp/utf8.h>

using namespace std;
using namespace nlohmann;


// 自定义比较规则
struct Compare{
    bool operator()(const Candidate &lhs,const Candidate &rhs)
    {
        // 编辑距离小优先
        if(lhs.distance != rhs.distance)
        {
            return lhs.distance > rhs.distance;
        }
        // 词频大优先
        if(lhs.frequency != rhs.frequency)
        {
            return lhs.frequency < rhs.frequency;
        }
        // 字典序小优先
        return lhs.word > rhs.word;
    }
};

// 构造函数
KeyRecommender::KeyRecommender()
{
    // 加载词典文件
    loadDict("data/cn_dict.dat");
    // 加载索引文件
    loadIndex("data/cn_index.dat");
}

// 加载词典文件
void KeyRecommender::loadDict(const string &filename)
{
    ifstream ifs{filename};
    if(!ifs)
    {
        cerr << "open dict file failed" << endl;
        return;
    }
    string word;
    int freq;
    while(ifs >> word >> freq)
    {
        dict_.emplace_back(word,freq);
    }
    cout << "dict size = " << dict_.size() << endl;
}
// 加载索引文件
void KeyRecommender::loadIndex(const string &filename)
{
    ifstream ifs{filename};
    if(!ifs)
    {
        cerr << "open index file failed" << endl;
        return;
    }
    string line;
    while(getline(ifs,line))
    {
        istringstream iss{line};
        string word;
        iss >> word;
        int id;
        while(iss >> id)
        {
            index_[word].insert(id);
        }
    }
    cout << "index size = " << index_.size() << endl;
}

// 计算编辑距离
int KeyRecommender::editDistance(const string &lhs,const string &rhs)
{
    // 拆分汉字
    auto left = splitChinese(lhs);
    auto right = splitChinese(rhs);

    // 长度
    size_t m = left.size();
    size_t n = right.size();

    // DP表
    vector<vector<int>> dp( m+1 , vector<int>(n+1) );

    // 初始化
    // 空串情况
    // 删除
    for(size_t i = 0;i <= m;++i)
    {
        dp[i][0] = i;
    }
    // 插入
    for(size_t j = 0; j <= n; ++j)
    {
        dp[0][j] = j;
    }

    // 状态转移
    for(size_t i=1;i<=m;++i)
    {
        for(size_t j=1;j<=n;++j)
        {
            if(left[i-1] == right[j-1])
            {
                dp[i][j] =
                    dp[i-1][j-1];
            }
            else
            {
                dp[i][j] =
                    min({
                        dp[i-1][j] + 1,  // 删除
                        dp[i][j-1] + 1,  // 插入
                        dp[i-1][j-1] + 1 // 替换
                    });
            }
        }
    }
    return dp[m][n];
}


// 输入keyword
// 索引库找到候选词
// 返回候选词集合
string KeyRecommender::query(const string &keyword)
{
    // 去重，候选词集合
    set<int> candidateIds;

    // 把keyword拆成汉字
    // chars是个vector<string>
    auto chars = splitChinese(keyword);

    // 查询索引库
    for(const auto &ch : chars)
    {
        // 从索引库中查找
        auto it = index_.find(ch);
        // 没找到
        if(it == index_.end())
        {
            continue;
        }
        // 找到了  取索引结果的并集
        // it->second.begin() 和 end() 代表这个 ID 集合的开头和结尾。
        // set可以一次插入整个集合
        candidateIds.insert(it->second.begin(),it->second.end());
    }

    // 排序
    priority_queue<Candidate,vector<Candidate>,Compare> pq;
    for(auto id : candidateIds)
    {
        Candidate candidate;
        candidate.word = dict_[id].first;
        candidate.frequency = dict_[id].second;
        candidate.distance = editDistance(keyword, candidate.word);
        pq.push(candidate);
    }

    // topK
    // vector<string> result;
    // int k = 5;
    // while(!pq.empty() && k--)
    // {
    //     result.push_back(pq.top().word);
    //     pq.pop();
    // }
    // return result;
    //
    // 改为JSON
    json jsonResult = json::array();
    int k = 5;
    while(!pq.empty() && k--)
    {
        // 每次循环取优先级队列的队头元素
        const Candidate &top = pq.top();
        json item;
        item["word"] = top.word;
        item["frequency"] = top.frequency;
        item["distance"] = top.distance;
        // 放入json结果集中
        jsonResult.push_back(item);
        // 出队
        pq.pop();
    }

    // 序列化为字符串返回
    return jsonResult.dump(2);
}
