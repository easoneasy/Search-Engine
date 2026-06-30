#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <mutex>
#include <nlohmann/json.hpp>

// 热搜关键词统计
struct HotKeyword
{
    std::string keyword;
    int    clickCount     = 0;
    double totalDuration  = 0.0;
    double hotScore       = 0.0;
};

class HotSearchTracker
{
public:
    HotSearchTracker(double clickWeight = 1.0, double durationWeight = 0.1)
        : clickWeight_(clickWeight)
        , durationWeight_(durationWeight)
    {
    }

    // 记录一次搜索
    void recordSearch(const std::string& keyword)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(keyword);
        if (it == stats_.end())
            stats_[keyword] = {keyword, 1, 0.0, 0.0};
        else
            it->second.clickCount++;
    }

    // 记录浏览时长（秒）
    void recordBrowseTime(const std::string& keyword, double seconds)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(keyword);
        if (it != stats_.end())
            it->second.totalDuration += seconds;
    }

    // 计算热度分数并返回 Top N
    std::vector<HotKeyword> getHotKeywords(int topN = 10)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<HotKeyword> hotList;
        for (auto& [key, kw] : stats_)
        {
            kw.hotScore = clickWeight_ * kw.clickCount
                        + durationWeight_ * kw.totalDuration;
            hotList.push_back(kw);
        }

        std::sort(hotList.begin(), hotList.end(),
                  [](const HotKeyword& a, const HotKeyword& b) {
                      return a.hotScore > b.hotScore;
                  });

        if ((int)hotList.size() > topN)
            hotList.resize(topN);

        return hotList;
    }

    // 返回 JSON 格式热搜榜
    std::string getHotKeywordsJson(int topN = 10)
    {
        auto hotList = getHotKeywords(topN);

        nlohmann::json jsonResult = nlohmann::json::array();
        for (size_t i = 0; i < hotList.size(); ++i)
        {
            nlohmann::json item;
            item["rank"]       = i + 1;
            item["keyword"]    = hotList[i].keyword;
            item["clickCount"] = hotList[i].clickCount;
            item["hotScore"]   = hotList[i].hotScore;
            jsonResult.push_back(item);
        }
        return jsonResult.dump(2);
    }

private:
    double clickWeight_;
    double durationWeight_;
    std::unordered_map<std::string, HotKeyword> stats_;
    std::mutex mutex_;
};
