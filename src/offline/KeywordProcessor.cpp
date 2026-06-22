//生成词典库、索引库
#include "../../include/offline/KeywordProcessor.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cppjieba/Jieba.hpp>

// cppjieba 词典路径
const char* const DICT_PATH      = "/usr/local/dict/jieba.dict.utf8";
const char* const HMM_PATH       = "/usr/local/dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "/usr/local/dict/user.dict.utf8";
const char* const IDF_PATH       = "/usr/local/dict/idf.utf8";
const char* const STOP_WORD_PATH = "/usr/local/dict/stop_words.utf8";

using namespace std;

// 构造函数
KeyWordProcessor::KeyWordProcessor()
    : tokenizer_(DICT_PATH,
                 HMM_PATH,
                 USER_DICT_PATH,
                 IDF_PATH,
                 STOP_WORD_PATH)
{
}


void KeyWordProcessor::create_en_dict(const std::string& dir, const std::string& outfile)
{
    // 加载停用词
    ifstream ifs{"data/stopwords/en_stopwords.txt"};
    if(!ifs)
    {
        cerr << "open en stopwords file failed" << endl;
        return;
    }
    string word;
    // 读取每一个单词
    while(ifs >> word)
    {
        // 将单词放入停用词容器中
        enStopWords_.insert(word);
    }
    cout << enStopWords_.size()<< endl;
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
        cout << "open file success!" << file << endl;

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


void KeyWordProcessor::build_en_index(const std::string& dict, const std::string& index)
{
    // 读取词典文件
    ifstream ifs{dict};
    if(!ifs)
    {
        cerr << "open dict file failed" << endl;
        return;
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


void KeyWordProcessor::create_cn_dict(const std::string& dir, const std::string& outfile)
{
    // 加载停用词
    ifstream ifs{"data/stopwords/cn_stopwords.txt"};
    if(!ifs)
    {
        cerr << "open ch stopwords file failed" << endl;
        return;
    }
    string word;
    // 读取每一个单词
    while(ifs >> word)
    {
        // 将单词放入停用词容器中
        chStopWords_.insert(word);
    }
    cout << chStopWords_.size()<< endl;

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
        cout << "open file success!" << file << endl;

        // 把整个文件一次性读入string
        // （指向文件的第一个字符，文件结束位置）
        // 从文件开始位置，一直读到EOF，全部放进content
        string content(istreambuf_iterator<char>(ifs),istreambuf_iterator<char>);
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
void KeyWordProcessor::build_cn_index(const std::string& dict, const std::string& index)
{

}
