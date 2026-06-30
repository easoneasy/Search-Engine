#include "../../include/offline/KeywordProcessor.h"
#include "../../include/offline/PageProcessor.h"
#include <iostream>

using namespace std;

int main()
{
    // 关键字推荐离线建库：生成 cn_dict/en_dict/cn_index/en_index
    KeyWordProcessor kw;
    kw.process("data/corpus/CN", "data/corpus/EN");

    // 网页搜索离线建库：SimHash去重 → pages/offsets → TF-IDF倒排索引
    PageProcessor ps;
    ps.process("data/corpus/webpages");

    return 0;
}
