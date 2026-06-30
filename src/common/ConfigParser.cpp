#include "../../include/common/ConfigParser.h"
#include <fstream>
#include <stdexcept>

using namespace std;

ConfigParser::ConfigParser(const string& filepath)
{
    ifstream ifs(filepath);
    if (!ifs)
        throw runtime_error("Cannot open config file: " + filepath);

    string currentSection;
    string line;
    while (getline(ifs, line))
    {
        // 去掉首尾空格
        size_t start = line.find_first_not_of(" \t\r");
        if (start == string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == string::npos) continue;

        string key   = line.substr(0, eqPos);
        string value = line.substr(eqPos + 1);

        auto trim = [](string& s) {
            size_t s0 = s.find_first_not_of(" \t");
            size_t s1 = s.find_last_not_of(" \t");
            if (s0 == string::npos) s = "";
            else s = s.substr(s0, s1 - s0 + 1);
        };
        trim(key);
        trim(value);

        data_[currentSection][key] = value;
    }
}
