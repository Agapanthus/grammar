#pragma once


#include <ostream>
using std::ostream;

#include <string>
using std::string, std::to_string;

#include "grammar.hpp"

ostream &operator<<(ostream &out, const NounType &n) {
    switch (n) {
    case NounType::Name:
        out << "Name";
        break;
    case NounType::Noun:
        out << "Noun";
        break;
    case NounType::Toponym:
        out << "Topo";
        break;
    case NounType::Numeral:
        out << "Num";
        break;
    default:
        out << "?";
        break;
    }
    return out;
}
ostream &operator<<(ostream &out, const pronounType &n) {
    switch (n) {
    case pronounType::Personal:
        out << "Personal";
        break;
    case pronounType::Reflexive:
        out << "Reflex";
        break;
    case pronounType::Reciprocal:
        out << "Recip";
        break;
    case pronounType::Possessive:
        out << "Poss";
        break;
    case pronounType::Demonstrative:
        out << "Demo";
        break;
    case pronounType::Indefinite:
        out << "Indef";
        break;
    case pronounType::Relative:
        out << "Relative";
        break;
    case pronounType::Interrogative:
        out << "Inter";
        break;
    default:
        out << "?";
        break;
    }
    return out;
}
ostream &operator<<(ostream &out, const Numeri &n) {
    if (n.singular.size() > 0)
        out << " ";
    for (const string s : n.singular)
        out << s << " ";
    if (n.plural.size() > 0)
        out << " ";
    for (const string p : n.plural)
        out << p << " ";
    return out;
}

ostream &operator<<(ostream &out, const Person &p) {
    out << "\n   1. " << p.first << "\n   2. " << p.second << "\n   3. "
        << p.third << "\n";
    return out;
}

ostream &operator<<(ostream &out, const Genus &n) {
    if (n.m)
        out << "m";
    if (n.f)
        out << "f";
    if (n.n)
        out << "n";
    return out;
}

ostream &operator<<(ostream &out, const Noun &n) {
    out << n.type << " " << n.genus << "\n   NOM: " << n.nominative << "\n";
    if (!n.genitive.empty())
        out << "   GEN: " << n.genitive << "\n";
    if (!n.dative.empty())
        out << "   DAT: " << n.dative << "\n";
    if (!n.accusative.empty())
        out << "   ACC: " << n.accusative << "\n";
    return out;
}

template <typename T>
void printVector(ostream &out, const vector<T> &t, const string &sep = " ") {
    for (const T tt : t)
        out << tt << sep;
}

ostream &operator<<(ostream &out, const Verb &v) {
    if (!v.base.present.empty())
        out << v.base.present << " ";

    if (!v.base.auxiliary.empty())
        printVector(out << "AUX: ", v.base.auxiliary);
    if (!v.base.participleII.empty())
        printVector(out << "PART2: ", v.base.participleII);
    if (!v.base.preterite_firstSingular.empty())
        printVector(out << "PRET: ", v.base.preterite_firstSingular);
    if (!v.base.subjunctiveI_firstSingular.empty())
        printVector(out << "SUB1: ", v.base.subjunctiveI_firstSingular);
    if (!v.base.subjunctiveII_firstSingular.empty())
        printVector(out << "SUB2: ", v.base.subjunctiveII_firstSingular);

    return out;
}

ostream &operator<<(ostream &out, const Adjective &a) {
    printVector(out, a.positive);
    printVector(out, a.comparative);
    printVector(out, a.superlative);
    return out;
}
ostream &operator<<(ostream &out, const Adverb &a) {
    printVector(out, a.positive);
    printVector(out, a.comparative);
    printVector(out, a.superlative);
    return out;
}

ostream &operator<<(ostream &out, const Pronoun &p) {
    out << p.type << p.nominative << p.genitive << p.dative << p.accusative;
    return out;
}

ostream &operator<<(ostream &out, const Word &w) {
    switch (w.w) {
    case WordType::Adjective:
        out << *w.adj;
        break;
    case WordType::Adverb:
        out << *w.adv;
        break;
    case WordType::Noun:
        out << *w.noun;
        break;
    case WordType::Pronoun:
        out << *w.pro;
        break;
    case WordType::Verb:
        out << *w.verb;
        break;
    case WordType::None:
        out << "EMPTY";
        break;
    default:
        out << "?";
        break;
    }

    return out;
}

ostream &operator<<(ostream &out, const vector<Word> &words) {
    for(auto w : words) out << w << "\n";
    return out;
}

ostream &operator<<(ostream &out, const Dictionary &dict) {
    const bool printList = false;

    out << "\n"
        << "##### " << dict.nouns.size() << " Nouns #####"
        << "\n"
        << "\n";
    if (printList)
        for (auto noun : dict.nouns)
            out << noun << "\n";

    out << "\n"
        << "##### " << dict.verbs.size() << " Verbs #####"
        << "\n"
        << "\n";
    if (printList)
        for (auto verb : dict.verbs)
            out << verb << "\n";

    out << "\n"
        << "##### " << dict.adjectives.size() << " Adjectives #####"
        << "\n"
        << "\n";
    if (printList)
        for (auto adj : dict.adjectives)
            out << adj << "\n";

    out << "\n"
        << "##### " << dict.adverbs.size() << " Adverbs #####"
        << "\n"
        << "\n";
    if (printList)
        for (auto adv : dict.adverbs)
            out << adv << "\n";

    out << "\n"
        << "##### " << dict.pronouns.size() << " Pronouns #####"
        << "\n"
        << "\n";
    if (printList)
        for (auto pron : dict.pronouns)
            out << pron << "\n";

    return out;
}
