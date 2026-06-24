#include <iostream>
#include "../../include/common/TextUtils.h"
#include<fstream>
#include <utfcpp/utf8.h>

using namespace std;

// 加载停用词,放入容器中
unordered_set<std::string> loadStopWords(const string &path)
{
    std::unordered_set<std::string> stopWords_;
    // 加载停用词
    ifstream ifs{path};
    if(!ifs)
    {
        throw runtime_error("Cannot open stop words file:" + path);
    }
    string word;
    // 读取每一个单词
    while(ifs >> word)
    {
        // 将单词放入停用词容器中
        stopWords_.insert(word);
    }
    // cout << stopWords_.size()<< endl;

    return stopWords_;
}

// 判断是否为中文
bool isChineseWord(const string &word)
{
    const char *curr = word.c_str();
    const char *end = curr + word.size();
    while(curr < end)
    {
        uint32_t codepoint = utf8::next(curr, end);
        // 中文统一汉字区
        if(codepoint < 0x4E00 || codepoint > 0x9FFF)
        {
            return false;
        }
    }
    return true;
}
