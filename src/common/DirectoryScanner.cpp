#include "../../include/common/DirectoryScanner.h"
#include <cstdio>
#include <vector>
#include <dirent.h>

using namespace std;

// 获取目录里面的所有文件名
std::vector<string> DirectoryScanner::scan(const string& dir)
{
    vector<string> files;
    DIR *dp = opendir(dir.c_str());
    // 打不开，报错
    if(dp == nullptr)
    {
        perror("opendir");
        return files;
    }
    // 打开了
    // dirent结构体用于保存目录项的信息
    struct dirent *entry;
    // 循环读取目录中的每一个目录项
    while((entry = readdir(dp)) != nullptr)
    {
        // 获取当前目录项的名字
        string name = entry->d_name;
        // 跳过 . 和 ..
        if(name == "." || name == "..")
        {
            continue;
        }
        // 如果是普通文件就放入vector
        if(entry->d_type == DT_REG)
        {
            // 拼接完整路径
            files.push_back(dir + "/" + name);
        }
    }
    closedir(dp);
    return files;
}
