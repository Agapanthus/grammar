#pragma once
#include "noun.hpp"
#include "verb.hpp"
#include "adjective.hpp"
#include "adverb.hpp"
#include "pronoun.hpp"

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

    vector<Word> find(const string &what) const {
        auto res = dict.find(toLower(what));
        if (res == dict.end())
            return {};
        return res->second;
    }

    vector<Word> closest(const string &what) const {
        auto low = dict.lower_bound(toLower(what));
        if (low == dict.end())
            return {};
        else if (low == dict.begin())
            return low->second;
        else {
            auto prev = std::prev(low);
            vector<Word> res = prev->second;
            for (auto w : low->second)
                res.push_back(w);
            return res;
        }
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
