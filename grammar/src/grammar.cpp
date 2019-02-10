#include "grammar.hpp"
#include "dictionary.hpp"


void emplace(map<string, vector<Word>> &dict, const string &s, const Word &w) {

    auto [it, suc] = dict.emplace(toLower(s), vector({w}));
    if (!suc)
        it->second.push_back(w);
}
