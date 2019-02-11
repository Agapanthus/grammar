#pragma once
#include "adjective.hpp"
#include "adverb.hpp"
#include "noun.hpp"
#include "pronoun.hpp"
#include "verb.hpp"

/*
struct Diathesis {
    Person active, processualPassive,
        statalPassive; // TODO: , causative, applicative;
};*/

class Dictionary {
  private:
    map<string, vector<Word>> dict;

  public:
    vector<Noun> nouns;
    vector<Verb> verbs;
    vector<Adjective> adjectives;
    vector<Adverb> adverbs;
    vector<Pronoun> pronouns;

  public:
    void add(const Noun &noun) { nouns.push_back(noun); }
    void add(const Verb &verb) { verbs.push_back(verb); }
    void add(const Adjective &adj) { adjectives.push_back(adj); }
    void add(const Adverb &adv) { adverbs.push_back(adv); }
    void add(const Pronoun &prn) { pronouns.push_back(prn); }

    size_t size() {
        return nouns.size() + verbs.size() + adjectives.size() +
               adverbs.size() + pronouns.size();
    }

    void buildMap() {
        dict.clear();

        for (Noun &noun : nouns)
            noun.buildMap(dict);
        for (Verb &verb : verbs)
            verb.buildMap(dict);
        for (Adjective &adj : adjectives)
            adj.buildMap(dict);
        for (Adverb &adv : adverbs)
            adv.buildMap(dict);
        for (Pronoun &pro : pronouns)
            pro.buildMap(dict);
    }

    vector<Word> find(const stringLower &what) const {
        auto res = dict.find(what);
        if (res == dict.end())
            return {};
        return res->second;
    }

    vector<Word> closest(const stringLower &what) const {
        auto low = dict.lower_bound(what);
        if (low == dict.end())
            return {};
        else if (low == dict.begin())
            return low->second;
        else {
            auto prev = std::prev(low);
            vector<Word> res = prev->second;
            for (const auto &w : low->second)
                res.push_back(w);
            return res;
        }
    }

  private:
    static size_t stripEmpty(WithCases &wc) {
        size_t stripped = 0;
        for (size_t i = 0; i < 4; i++) {
            stripped += removeWhere(wc.cases[i].singular, "") +
                        removeWhere(wc.cases[i].plural, "");
        }
        return stripped;
    }
    static size_t stripEmpty(Person &p) {
        return stripEmpty(p.first) + stripEmpty(p.second) + stripEmpty(p.third);
    }
    static size_t stripEmpty(Numeri &n) {
        return removeWhere(n.singular, "") + removeWhere(n.plural, "");
    }

  public:
    void simplify() {
        size_t emptyRemoved = 0;
        for (Noun &noun : nouns)
            emptyRemoved += stripEmpty(noun);
        for (Adjective &adj : adjectives) {
            for (size_t i = 0; i < 9; i++)
                emptyRemoved += stripEmpty(adj.cases[i]);
            emptyRemoved += removeWhere(adj.positive, "") +
                            removeWhere(adj.comparative, "") +
                            removeWhere(adj.superlative, "");
        }
        for (Verb &verb : verbs) {
            emptyRemoved +=
                stripEmpty(verb.imperative) + stripEmpty(verb.base.present) +
                removeWhere(verb.base.auxiliary, "") +
                removeWhere(verb.base.irrealis_firstSingular, "") +
                removeWhere(verb.base.participleII, "") +
                removeWhere(verb.base.presentInfinitive, "") +
                removeWhere(verb.base.preterite_firstSingular, "") +
                removeWhere(verb.base.subjunctive_firstSingular, "");
        }
        for (Adverb &adv : adverbs) {
            emptyRemoved += removeWhere(adv.positive, "") +
                            removeWhere(adv.comparative, "") +
                            removeWhere(adv.superlative, "");
        }
        for (Pronoun &pro : pronouns)
            emptyRemoved += stripEmpty(pro);

        cout << "   removed empty: " << emptyRemoved << endl;

        
    }

    void serialize(ostream &out) const {
        serializeVector(out, this->nouns);
        serializeVector(out, this->verbs);
        serializeVector(out, this->adjectives);
        serializeVector(out, this->adverbs);
        serializeVector(out, this->pronouns);
    }
    void deserialize(istream &in) {
        deserializeVector(in, this->nouns);
        deserializeVector(in, this->verbs);
        deserializeVector(in, this->adjectives);
        deserializeVector(in, this->adverbs);
        deserializeVector(in, this->pronouns);
    }
};
