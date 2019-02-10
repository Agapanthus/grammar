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
    unknown
};

struct Pronoun {
    pronounType type;
    Person nominative, genitive, dative, accusative;

    void serialize(ostream &out) const {
        out << (int)type << " ";
        nominative.serialize(out);
        genitive.serialize(out);
        dative.serialize(out);
        accusative.serialize(out);
    }

    void deserialize(istream &in) {
        int iType;
        in >> iType;
        type = (pronounType)iType;
        nominative.deserialize(in);
        genitive.deserialize(in);
        dative.deserialize(in);
        accusative.deserialize(in);
    }

    void buildMap(map<string, vector<Word>> &dict) {
        Word me({(Noun *)this, WordType::Pronoun});
        nominative.buildMap(me, dict);
        genitive.buildMap(me, dict);
        dative.buildMap(me, dict);
        accusative.buildMap(me, dict);
    }
};
