#pragma once
#include "grammar.hpp"

enum class pronounType {
    Personal,
    Reflexive,
    Reciprocal,
    Possessive,
    Demonstrative,
    Indefinite,
    Relative,
    Interrogative,
    Article,
    unknown
};

struct Pronoun : public WithCases {
    pronounType type;

    void serialize(ostream &out) const {
        out << (int)type << " ";
        WithCases::serialize(out);
    }

    void deserialize(istream &in) {
        int iType;
        in >> iType;
        type = (pronounType)iType;
        WithCases::deserialize(in);
    }

    void buildMap(map<string, vector<Word>> &dict) {
        Word me({(Noun *)this, WordType::Pronoun});
        nominative.buildMap(me, dict);
        genitive.buildMap(me, dict);
        dative.buildMap(me, dict);
        accusative.buildMap(me, dict);
    }
};
