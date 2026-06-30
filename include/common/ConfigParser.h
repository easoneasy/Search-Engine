#pragma once

#include <string>
#include <stdexcept>
#include <unordered_map>

class ConfigParser
{
public:
    explicit ConfigParser(const std::string& filepath);

    template <typename T>
    T get(const std::string& section, const std::string& key) const;

private:
    void parseLine(const std::string& line, std::string& currentSection);

    std::unordered_map<std::string,
        std::unordered_map<std::string, std::string>> data_;
};

// ============================================================
// 模板特化必须在头文件中定义（否则链接时找不到）
// ============================================================

// 特化1: string
template <>
inline std::string ConfigParser::get<std::string>(
    const std::string& section, const std::string& key) const
{
    auto secIt = data_.find(section);
    if (secIt == data_.end())
        throw std::runtime_error("Config section not found: " + section);
    auto keyIt = secIt->second.find(key);
    if (keyIt == secIt->second.end())
        throw std::runtime_error("Config key not found: " + section + "." + key);
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
