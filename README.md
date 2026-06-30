# NutShell Search — 基于 C++ 的高性能搜索引擎

一个完整的搜索引擎项目，分为**三期递进开发**：离线建库 → 在线服务 → 缓存优化。

## 项目结构

```
NutShell-Search/
├── include/
│   ├── common/          # 公共模块头文件
│   │   ├── DirectoryScanner.h   # 目录扫描器
│   │   └── TextUtils.h          # 文本工具（停用词/中文判断/汉字拆分）
│   ├── offline/         # 离线建库头文件
│   │   ├── KeywordProcessor.h   # 关键字推荐离线处理
│   │   └── PageProcessor.h      # 网页搜索离线处理
│   └── online/          # 在线服务头文件
│       ├── TlvProtocol.h        # TLV 通信协议
│       ├── searchServer.h       # muduo TCP 服务器
│       ├── KeyRecommender.h     # 关键字推荐（在线）
│       ├── WebPageQuery.h       # 网页搜索（在线）
│       ├── LRUCache.h           # LRU 缓存
│       └── ShardedCache.h       # 分片缓存
├── src/
│   ├── common/          # 公共模块实现
│   │   ├── DirectoryScanner.cpp
│   │   └── TextUtils.cpp
│   ├── offline/         # 离线建库实现
│   │   ├── KeywordProcessor.cpp
│   │   ├── PageProcessor.cpp
│   │   └── offline_main.cpp
│   └── online/          # 在线服务实现
│       ├── KeyRecommender.cc
│       ├── WebPageQuery.cc
│       ├── searchServer.cc
│       └── online_main.cc
├── data/                # 数据文件
│   ├── corpus/          # 原始语料（EN/CN/webpages）
│   ├── stopwords/       # 停用词表
│   ├── *.dat            # 离线生成的索引/词典/网页数据
├── static/              # 前端页面
│   ├── index.html       # Google 风格搜索页
│   └── app.py           # HTTP 桥接服务器
├── docs/                # 项目文档
├── conf/                # 配置文件
├── CMakeLists.txt
├── build.sh
└── README.md
```

## 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C++17 |
| 构建 | CMake 3.15+ |
| 网络框架 | muduo（Reactor 模式） |
| 中文分词 | cppjieba |
| UTF-8 处理 | utfcpp |
| XML 解析 | tinyxml2 |
| 文本去重 | simhash |
| JSON 序列化 | nlohmann/json |
| 前端 | HTML/CSS/JS + Python HTTP 桥接 |

## 快速开始

### 编译

```bash
cd /home/zyt/Search-Engine
./build.sh
```

生成两个可执行文件：
- `offline_search` — 离线建库（处理语料，生成数据文件）
- `online_search` — 在线搜索服务器（muduo TCP 服务器，端口 8888）

### 启动在线搜索服务

```bash
# 终端 1：启动 C++ 后端
./online_search

# 终端 2：启动 HTTP 桥接（前端）
python3 static/app.py
```

浏览器打开 `http://localhost:8080` 即可使用。

### 离线建库（如需要重新生成数据）

编辑 `src/offline/offline_main.cpp`，取消注释后编译运行：

```bash
./build.sh && ./offline_search
```

## 三期开发总览

| 期数 | 内容 | 核心功能 |
|------|------|----------|
| **第一期** | 离线建库 | 英文/中文分词 → 词典库+索引库，XML解析 → SimHash去重 → 网页库+偏移库+倒排索引 |
| **第二期** | 在线服务 | muduo TCP 服务器 + TLV 协议，编辑距离关键字推荐，TF-IDF + 余弦相似度网页搜索 |
| **第三期** | 缓存优化 | LRU 缓存 + 分片缓存，加速关键字推荐和网页搜索的热点查询 |

## 通信协议

基于 TLV（Type-Length-Value）的自定义二进制协议：

```
┌────────┬──────────────┬──────────────────┐
│  type  │    length     │      value       │
│ 1 byte │    4 bytes    │   length bytes   │
│ uint8  │ uint32 (大端)  │   UTF-8/JSON     │
└────────┴──────────────┴──────────────────┘

type = 1: 关键字推荐
type = 2: 网页搜索
```

## 离线数据文件

| 文件 | 格式 | 用途 |
|------|------|------|
| `data/cn_dict.dat` | 词汇 词频 | 中文候选词库 |
| `data/cn_index.dat` | 汉字 id1 id2... | 按汉字定位候选词 |
| `data/en_dict.dat` | word frequency | 英文候选词库 |
| `data/en_index.dat` | char id1 id2... | 按首字母定位候选词 |
| `data/pages.dat` | XML <doc> 块 | 网页全文存储 |
| `data/offsets.dat` | docid offset length | 随机访问网页库 |
| `data/index.dat` | keyword docid weight ... | 倒排索引 + TF-IDF权重 |

## 依赖安装

```bash
# 基础工具
sudo apt install -y cmake g++ libboost-dev

# tinyxml2
sudo apt install -y libtinyxml2-dev

# nlohmann-json
sudo apt install -y nlohmann-json3-dev

# cppjieba / utfcpp / simhash（header-only，复制到 /usr/local/include）
# muduo（源码编译安装：https://github.com/chenshuo/muduo）
```

## 文档

- `docs/项目第一期开发思路.md` — 离线建库开发指南
- `docs/第二期在线服务流程.md` — 在线服务完整流程
- `docs/网页搜索模块流程详解.md` — WebPageQuery 模块详解
- `docs/搜索引擎项目.pdf` — 项目需求规格说明书

## License

MIT
