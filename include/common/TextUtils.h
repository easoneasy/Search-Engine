#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>

// 加载停用词,放入容器中
std::unordered_set<std::string> loadStopWords(const std::string &path);

// 判断是否为中文
bool isChineseWord(const std::string &word);
