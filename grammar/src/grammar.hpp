#pragma once

#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>
using std::map;
using std::ostream, std::istream;
using std::pair;
using std::string, std::to_string;
using std::vector;

#include "util.hpp"

//////////////////////////////////////

enum class WordType { Noun, Verb, Adjective, Adverb, Pronoun, None };

struct Noun;
struct Verb;
struct Adjective;
struct Adverb;
struct Pronoun;
class Dictionary;
struct Word {
    union {
        const Noun *noun;
        const Verb *verb;
        const Adjective *adj;
        const Adverb *adv;
        const Pronoun *pro;
    };
    WordType w;
};

void emplace(map<string, vector<Word>> &dict, const string &s, const Word &w);

//////////////////////////////////////

const thread_local vector<string> vocals = {"a", "e", "i", "o",
                                            "u", "ö", "ä", "ü"};
const thread_local vector<string> vocalsPlus = {
    "au", "ei", "eu", "äu", "a", "e", "i", "o", "u", "ö", "ä", "ü"};

const thread_local vector<string> sibilants = {"s",    "ss", "ß",
                                               "tsch", "x",  "z"};

//////////////////////////////////////

template <typename T>
inline void serializeVector(ostream &out, const vector<T> &v, bool nl = true) {
    out << v.size() << (nl ? "\n" : " ");
    if (v.size() > 1024 * 1024 * 1024)
        throw std::exception("invalid");
    for (auto e : v) {
        e.serialize(out);
        out << (nl ? "\n" : " ");
    }
}

template <>
inline void serializeVector(ostream &out, const vector<string> &v, bool nl) {
    out << (nl ? "\n" : " ") << v.size() << " ";
    if (v.size() > 1024 * 1024 * 1024)
        throw std::exception("invalid");
    for (auto e : v) {
        out << e.size() << " " << e << "";
    }
}

template <typename T> inline void deserializeVector(istream &in, vector<T> &v) {
    size_t size;
    if (!(in >> size) || in.bad() || size > 1024 * 1024 * 1024) {
        throw std::exception("invalid");
    }
    v.clear();
    v.resize(size);
    for (size_t i = 0; i < size; i++) {
        v[i].deserialize(in);
    }
}

template <> inline void deserializeVector(istream &in, vector<string> &v) {
    size_t size;
    if (!(in >> size) || in.bad() || size > 1024 * 1024 * 1024) {
        throw std::exception("invalid");
    }
    v.clear();
    v.resize(size);
    for (size_t i = 0; i < size; i++) {
        size_t count;
        in >> count;
        string str(count, ' ');
        in.read(&str[0], 1); // Skip whitespace
        in.read(&str[0], count);
        v[i] = str;
    }
}

struct Numeri {
    vector<string> singular, plural;

    bool empty() const { return singular.empty() && plural.empty(); }

    void serialize(ostream &out) const {
        serializeVector(out, singular);
        serializeVector(out, plural, false);
    }
    void deserialize(istream &in) {
        deserializeVector(in, singular);
        deserializeVector(in, plural);
    }

    void buildMap(const Word &me, map<string, vector<Word>> &dict) const {
        for (auto s : singular)
            emplace(dict, s, me);
        for (auto s : plural)
            emplace(dict, s, me);
    }
};

struct Genus {
    bool m;
    bool n;
    bool f;

    Genus() : m(false), n(false), f(false) {}

    bool empty() const { return !(m || f || n); }
};

struct Person {
    Numeri first, second, third;
    bool empty() const {
        return first.empty() && second.empty() && third.empty();
    }

    void serialize(ostream &out) const {
        first.serialize(out);
        second.serialize(out);
        third.serialize(out);
    }

    void deserialize(istream &in) {
        first.deserialize(in);
        second.deserialize(in);
        third.deserialize(in);
    }

    void buildMap(const Word &me, map<string, vector<Word>> &dict) const {
        first.buildMap(me, dict);
        second.buildMap(me, dict);
        third.buildMap(me, dict);
    }
};