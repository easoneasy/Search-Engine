#include "../../include/offline/KeywordProcessor.h"
#include <iostream>

using namespace std;

int main()
{
        KeyWordProcessor kw;
        kw.create_en_dict("data/corpus/EN", "data/dict_en.dat");
        kw.build_en_index("data/dict_en.dat", "data/index_en.dat");

    return 0;
}
