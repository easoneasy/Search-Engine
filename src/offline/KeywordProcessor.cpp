//生成词典库、索引库
#include "../../include/offline/KeywordProcessor.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utfcpp/utf8/checked.h>
#include <utility>
#include <vector>
#include <cppjieba/Jieba.hpp>
#include <utfcpp/utf8.h>

using namespace std;

// 把词典文件读取到容器中
vector<pair<string,int>> put_dict(const string &dict)
{
    // 读取词典文件
    ifstream ifs{dict};
    if(!ifs)
    {
        cerr << "open dict file failed" << endl;
        throw std::runtime_error("Failed to open dictionary file: " + dict);
    }
    // 保存到vector  vector<pair<string,int>>
    // 为词典文件的单词建立索引
    vector<pair<string, int>> dict_words;
    string word;
    int freq;
    while(ifs >> word >> freq)
    {
        dict_words.push_back({word,freq});
    }
    return dict_words;
}


// 构造函数
KeyWordProcessor::KeyWordProcessor()
    : tokenizer_()
{
    // 加载停用词
    enStopWords_ = loadStopWords("data/stopwords/en_stopwords.txt");
    chStopWords_ = loadStopWords("data/stopwords/cn_stopwords.txt");
}

// 入口函数
void KeyWordProcessor::process(const std::string& chDir, const std::string& enDir)
{
    create_cn_dict(chDir, "data/cn_dict.dat");
    build_cn_index("data/cn_dict.dat", "data/cn_index.dat");
    create_en_dict(enDir, "data/en_dict.dat");
    build_en_index("data/en_dict.dat", "data/en_index.dat");
}

// 创建英文字典
void KeyWordProcessor::create_en_dict(const std::string& dir, const std::string& outfile)
{

    // 准备词频统计容器
    unordered_map<string, int> wordFreq;
    // 获取所有语料文件
    auto files = DirectoryScanner::scan(dir);
    // 遍历每个语料文件
    for(const auto &file : files)
    {
        // 打开文件
        ifstream ifs{file};
        if(!ifs)
        {
            cerr << "open file failed" << file << endl;
            continue;
        }

        // 按行读取
        string line;
        while(getline(ifs,line))
        {
            // 数据清洗
            for(char &ch : line)
            {
                // 转成小写
                if(isalpha(ch))
                {
                    ch = tolower(ch);
                }else{
                    // 将非字母字符替换为空格
                    ch = ' ';
                }
            }
            // 分词
            stringstream ss {line};
            string word;
            while(ss >> word)
            {
                // 过滤停用词
                if(enStopWords_.count(word))
                {
                    continue;
                }
                // 记录词频
                ++wordFreq[word];
            }
        }
        cout << "wordFreq success " << endl;

    }

    // 输出词典库文件
    ofstream ofs(outfile);
    if(!ofs)
    {
        cerr << "open outfile failed" << endl;
        return;
    }
    for(const auto &e : wordFreq)
    {
        ofs << e.first << " " << e.second << "\n";
    }
    cout << "create en_dict success,word count = " << wordFreq.size() << endl;
}

// 创建英文索引
void KeyWordProcessor::build_en_index(const std::string& dict, const std::string& index)
{
    // 把词典文件读取到容器中
    auto dict_words = put_dict(dict);

    // 建立倒排索引   unordered_map<char,set<int>>
    unordered_map<char, set<int>> dict_index;
    // 遍历单词中的字符
    for(size_t i=0;i<dict_words.size();++i)
    {
        const string &w = dict_words[i].first;
        for(char ch : w)
        {
            // 把单个字符放入倒排索引容器里
            // set会自动去重
            dict_index[ch].insert(i);
        }
    }
    // 输出索引文件
    ofstream ofs(index);
    if(!ofs)
    {
        cerr << "open outfile failed" << endl;
        return;
    }
    // 遍历索引容器
    for(const auto &e :dict_index)
    {
        ofs << e.first;
        for(int id : e.second)
        {
            ofs << " " << id;
        }
        ofs << "\n";
    }
}

// 创建中文字典
void KeyWordProcessor::create_cn_dict(const std::string& dir, const std::string& outfile)
{
    // 准备词频统计容器
    unordered_map<string, int> wordFreq;
    // 获取所有语料文件
    auto files = DirectoryScanner::scan(dir);
    // 遍历每个语料文件
    for(const auto &file : files)
    {
        // 打开文件
        ifstream ifs{file};
        if(!ifs)
        {
            cerr << "open file failed" << file << endl;
            continue;
        }

        // 把整个文件一次性读入string
        // （指向文件的第一个字符，文件结束位置）
        // istreambuf_iterator<char> it(ifs)指向文件的第一个字符
        // 从文件开始位置，一直读到EOF，全部放进content
        // 整个文件读成一个string，直接分词而不是一行一行处理
        string content{istreambuf_iterator<char>(ifs),istreambuf_iterator<char>()};
        vector<string> words;
        //
        tokenizer_.Cut(content,words);

        // 过滤无效词
        for(auto &word : words)
        {
            if(word.size() == 0)
            {
                continue;
            }
            if(chStopWords_.count(word))
            {
                continue;
            }
            if(!isChineseWord(word))
            {
                continue;
            }
            ++wordFreq[word];
        }
    }

    // 写出词典文件
    ofstream ofs(outfile);
    if(!ofs)
    {
        cerr << "open outfile failed" << endl;
        return;
    }
    for(const auto &e : wordFreq)
    {
        ofs << e.first << " " << e.second << "\n";
    }
    cout << "create ch_dict success,word count = " << wordFreq.size() << endl;
}

// 创建中文索引
void KeyWordProcessor::build_cn_index(const std::string& dict, const std::string& index)
{
    // 把词典文件读取到容器中
    auto dict_words = put_dict(dict);

    // 建立倒排索引   unordered_map<char,set<int>>
    unordered_map<string, set<int>> dict_index;
    // 遍历单词中的字符
    for(size_t i=0;i<dict_words.size();++i)
    {
        const string &w = dict_words[i].first;
        // 拆汉字
        const char* curr = w.c_str();
        const char* end = w.c_str()+w.size();
        while(curr != end)
        {
            auto start = curr;
            // 将it移动到下一个utf8字符所在位置
            utf8::next(curr,end);
            // 一个汉字会占用多个字节，需要string表示一个汉字
            string charater = string(start,curr);
            dict_index[charater].insert(i);
        }
    }
    // 输出索引文件
    ofstream ofs(index);
    if(!ofs)
    {
        cerr << "open outfile failed" << endl;
        return;
    }
    // 遍历索引容器
    for(const auto &e :dict_index)
    {
        ofs << e.first;
        for(int id : e.second)
        {
            ofs << " " << id;
        }
        ofs << "\n";
    }
}
