#pragma once
#include "grammar.hpp"


struct Verb {
    struct {
        vector<string> presentInfinitive;
        Person present;

        vector<string> participleII;
        vector<string> subjunctiveI_firstSingular;
        vector<string> subjunctiveII_firstSingular;
        vector<string> preterite_firstSingular;
        vector<string> auxiliary;

    } base;
    Numeri imperative;
    // Diathesis present, preterite, perfect, pluperfect, futureI, futureII;
    // subjunctive I , II
    // Infinitive
    // participle
    // Gerundiv

    void serialize(ostream &out) const {
        serializeVector(out, base.presentInfinitive);
        serializeVector(out, base.participleII);
        serializeVector(out, base.subjunctiveI_firstSingular);
        serializeVector(out, base.subjunctiveII_firstSingular);
        serializeVector(out, base.preterite_firstSingular);
        serializeVector(out, base.auxiliary);
        base.present.serialize(out);
        imperative.serialize(out);
    }

    void deserialize(istream &in) {
        deserializeVector(in, base.presentInfinitive);
        deserializeVector(in, base.participleII);
        deserializeVector(in, base.subjunctiveI_firstSingular);
        deserializeVector(in, base.subjunctiveII_firstSingular);
        deserializeVector(in, base.preterite_firstSingular);
        deserializeVector(in, base.auxiliary);
        base.present.deserialize(in);
        imperative.deserialize(in);
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({(Noun *)this, WordType::Verb});
        imperative.buildMap(me, dict);
        for (auto s : base.presentInfinitive)
            emplace(dict, s, me);
        for (auto s : base.participleII)
            emplace(dict, s, me);
        for (auto s : base.subjunctiveI_firstSingular)
            emplace(dict, s, me);
        for (auto s : base.subjunctiveII_firstSingular)
            emplace(dict, s, me);
        for (auto s : base.preterite_firstSingular)
            emplace(dict, s, me);
        for (auto s : base.auxiliary)
            emplace(dict, s, me);
        base.present.buildMap(me, dict);
    }
};
