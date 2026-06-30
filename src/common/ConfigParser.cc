#include "../../include/common/ConfigParser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

// ================================================================
// 构造函数：读取并解析整个配置文件
// ================================================================
ConfigParser::ConfigParser(const std::string& filepath)
{
    ifstream ifs(filepath);
    if (!ifs)
    {
        throw runtime_error("Cannot open config file: " + filepath);
    }

    string currentSection;
    string line;
    while (getline(ifs, line))
    {
        // 去掉行首行尾空格
        size_t start = line.find_first_not_of(" \t\r");
        if (start == string::npos) continue;      // 空行跳过
        size_t end   = line.find_last_not_of(" \t\r");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == '#' || line[0] == ';')
        {
            continue;   // 跳过注释行（# 或 ; 开头）
        }

        // 检测 [section]
        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // 解析 key = value
        parseLine(line, currentSection);
    }
}

// ================================================================
// 解析单行 key = value
// ================================================================
void ConfigParser::parseLine(const string& line, string& currentSection)
{
    size_t eqPos = line.find('=');
    if (eqPos == string::npos) return;   // 没有等号，跳过

    string key = line.substr(0, eqPos);
    string value = line.substr(eqPos + 1);

    // 去掉 key 和 value 的首尾空格
    auto trim = [](string& s) {
        size_t s0 = s.find_first_not_of(" \t");
        size_t s1 = s.find_last_not_of(" \t");
        if (s0 == string::npos) s = "";
        else s = s.substr(s0, s1 - s0 + 1);
    };
    trim(key);
    trim(value);

    // 存入 data_
    data_[currentSection][key] = value;
}

// ================================================================
// get 模板特化
// ================================================================

// 特化1: string — 直接返回
template <>
inline std::string ConfigParser::get<std::string>(
    const std::string& section, const std::string& key) const
{
    auto secIt = data_.find(section);
    if (secIt == data_.end())
        throw runtime_error("Config section not found: " + section);

    auto keyIt = secIt->second.find(key);
    if (keyIt == secIt->second.end())
        throw runtime_error("Config key not found: " + section + "." + key);

    return keyIt->second;
}

// 特化2: int
template <>
inline int ConfigParser::get<int>(
    const std::string& section, const std::string& key) const
{
    return std::stoi(get<std::string>(section, key));
}

// 特化3: double
template <>
inline double ConfigParser::get<double>(
    const std::string& section, const std::string& key) const
{
    return std::stod(get<std::string>(section, key));
}
