#pragma once
#include "grammar.hpp"


struct Adjective {
    vector<string> positive, comparative, superlative;

    void serialize(ostream &out) const {
        serializeVector(out, positive);
        serializeVector(out, comparative);
        serializeVector(out, superlative);
    }

    void deserialize(istream &in) {
        deserializeVector(in, positive);
        deserializeVector(in, comparative);
        deserializeVector(in, superlative);
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({(Noun *)this, WordType::Adjective});
        for (auto s : positive)
            emplace(dict, s, me);
        for (auto s : comparative)
            emplace(dict, s, me);
        for (auto s : superlative)
            emplace(dict, s, me);
    }
};
