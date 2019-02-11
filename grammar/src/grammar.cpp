#include "grammar.hpp"
#include "dictionary.hpp"


void emplace(map<string, vector<Word>> &dict, const stringLower &s, const Word &w) {
    auto [it, suc] = dict.emplace(s, vector({w}));
    if (!suc)
        it->second.push_back(w);
}
