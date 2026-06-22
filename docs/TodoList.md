TodoList

阶段 A: 基础设施（不涉及算法，跑通编译链）
├── A1  修复 .gitignore（取消误忽略的 docs/ tests/ conf/*.conf）
├── A2  用 cmake 跑通一次空构建，确认依赖（tinyxml2/Threads）可找到
├── A3  实现 DirectoryScanner.h + .cpp，打印扫描到的文件路径验证
└── A4  实现 TextUtils.h + .cpp 工具函数骨架

阶段 B: 关键字推荐 — 英文
├── B1  实现 load_stop_words()，打印停用词数量验证
├── B2  实现 normalize_english_line() + 英文分词，打印前 10 个词频
├── B3  实现 create_en_dict()，生成 dict_en.dat
├── B4  实现 build_en_index()，生成 index_en.dat（按首字母建索引）
└── B5  手工验证：英文词典全小写、无数字标点、停用词已过滤、索引字符正确

阶段 C: 关键字推荐 — 中文
├── C1  引入 cppjieba（确认头文件和 dict 文件路径就绪）
├── C2  实现 create_cn_dict()，生成 dict_cn.dat
├── C3  引入 utfcpp，实现 split_utf8_characters()
├── C4  实现 build_cn_index()，生成 index_cn.dat（按单个汉字建索引）
└── C5  手工验证：中文词典无标点 token、索引 key 是完整 UTF-8 汉字

阶段 D: 网页搜索 — XML 解析
├── D1  引入 tinyxml2，实现 extract_documents()：从 1 个 XML 提取 Document 数组
├── D2  处理缺 content/description 标签的边界情况
└── D3  引入 simhash，实现 deduplicate_documents()

阶段 E: 网页搜索 — 离线产物
├── E1  实现 build_pages_and_offsets()，生成 pages.dat 和 offsets.dat
├── E2  验证：随机 seek 偏移库，读出的字节刚好是一篇完整 <doc>
├── E3  实现 TF-IDF 权重计算 + 归一化
├── E4  实现 build_inverted_index()，生成 invert_index.dat
└── E5  验证：高频关键词对应多个文档 id，且 id 都在 offsets.dat 中存在

阶段 F: 整合与验收
├── F1  编写 offline_main.cpp 统一入口，串联 KeywordProcessor → PageProcessor
├── F2  添加构建日志输出（文件数、词典规模、去重数、倒排规模）
├── F3  跑通完整流程，检查所有 7 个产物文件格式
└── F4  对照自查清单逐项确认（文档第 14 节）