#pragma once
#include "grammar.hpp"


enum class AdverbType {
    adverb,
    particle,
    subjunction,
    unknown,
    article,
    conjunction,
    preposition,
    other
};

// Not really just adverbs... More like, simple words
struct Adverb {
    vector<string> positive, comparative, superlative;

    AdverbType type;

    void serialize(ostream &out) const {
        serializeVector(out, positive);
        serializeVector(out, comparative);
        serializeVector(out, superlative);
        out << (int)type << " ";
    }

    void deserialize(istream &in) {
        deserializeVector(in, positive);
        deserializeVector(in, comparative);
        deserializeVector(in, superlative);
        int temp;
        in >> temp;
        type = (AdverbType)temp;
    }

    void buildMap(map<string, vector<Word>> &dict) {
        Word me({(Noun *)this, WordType::Adverb});
        for (auto s : positive)
            emplace(dict, s, me);
        for (auto s : comparative)
            emplace(dict, s, me);
        for (auto s : superlative)
            emplace(dict, s, me);
    }
};