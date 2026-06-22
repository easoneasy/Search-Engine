cmake_minimum_required(VERSION 3.15)

# 项目名称和版本
project(NutShellSearch VERSION 1.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启所有警告，有助于写出健壮的代码
# 注意: 不再硬编码 -pthread (由 find_package(Threads) 处理)、-g (由 CMAKE_BUILD_TYPE 控制)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 设置可执行文件的输出路径为项目根目录下的 bin 文件夹
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# 头文件搜索路径
# /usr/local/include: cppjieba, simhash, utfcpp 等 header-only 库的安装位置
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/include
)

# --- 查找系统依赖库 ---
# 查找多线程库
find_package(Threads REQUIRED)

# tinyxml2: 用于离线模块解析 XML 语料
# 手动查找 tinyxml2（apt 安装版不含 cmake 配置文件）
find_path(TINYXML2_INCLUDE_DIR tinyxml2.h)
find_library(TINYXML2_LIBRARY tinyxml2)
if(NOT TINYXML2_INCLUDE_DIR OR NOT TINYXML2_LIBRARY)
    message(FATAL_ERROR "tinyxml2 not found")
endif()

# nlohmann_json: 用于在线模块 JSON 序列化（暂未启用）
# find_package(nlohmann_json REQUIRED)

# muduo: 用于在线模块的 TCP 网络框架（暂未启用）
# find_package(muduo REQUIRED)  # muduo 库分为 muduo_net 和 muduo_base

# =========================================================================
# 模块一：收集公共模块代码 (common)
# =========================================================================
file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS "src/common/*.cpp")

# =========================================================================
# 模块二：离线处理程序 (Offline Builder)
# =========================================================================
file(GLOB OFFLINE_SOURCES CONFIGURE_DEPENDS "src/offline/*.cpp")

# 生成离线可执行文件 search_engine
add_executable(search_engine ${OFFLINE_SOURCES} ${COMMON_SOURCES})

# 链接离线程序需要的库

target_link_libraries(search_engine
    ${TINYXML2_LIBRARY}
    Threads::Threads
)
target_include_directories(search_engine PRIVATE ${TINYXML2_INCLUDE_DIR})


# =========================================================================
# 模块三：在线检索服务器 (Online Engine)
# =========================================================================
# file(GLOB ONLINE_SOURCES CONFIGURE_DEPENDS "src/online/*.cpp")

# 生成在线可执行文件 nutshell_online
# add_executable(nutshell_online ${ONLINE_SOURCES} ${COMMON_SOURCES})

# 链接在线程序需要的库：muduo_net, muduo_base, nlohmann_json, pthread
# target_link_libraries(nutshell_online
#     muduo::muduo_net
#     muduo::muduo_base
#     nlohmann_json::nlohmann_json
#     Threads::Threads
# )

# =========================================================================
# (预留) 单元测试模块
# =========================================================================
# 如果后续有 tests 目录下的测试文件，可以在这里添加编译逻辑
