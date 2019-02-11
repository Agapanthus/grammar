#pragma once
#include "grammar.hpp"


struct Adjective {
    vector<string> positive, comparative, superlative; // predicative and adverbial form
    WithCases cases[3 * 3]; // Pos, Comp, Sup je m, n, f

    Adjective() {}

    void serialize(ostream &out) const {
        serializeVector(out, positive);
        serializeVector(out, comparative);
        serializeVector(out, superlative);
        for (size_t i = 0; i < 9; i++)
            cases[i].serialize(out);
    }

    void deserialize(istream &in) {
        deserializeVector(in, positive);
        deserializeVector(in, comparative);
        deserializeVector(in, superlative);
        for (size_t i = 0; i < 9; i++)
            cases[i].deserialize(in);
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({(Noun *)this, WordType::Adjective});
        for (const auto &s : positive)
            emplace(dict, s, me);
        for (const auto &s : comparative)
            emplace(dict, s, me);
        for (const auto &s : superlative)
            emplace(dict, s, me);
    }
};
