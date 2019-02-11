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

void emplace(map<string, vector<Word>> &dict, const stringLower &s,
             const Word &w);

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
    for (const auto &e : v) {
        e.serialize(out);
        out << (nl ? "\n" : " ");
    }
}

template <>
inline void serializeVector(ostream &out, const vector<string> &v, bool nl) {
    out << (nl ? "\n" : " ") << v.size() << " ";
    if (v.size() > 1024 * 1024 * 1024)
        throw std::exception("invalid");
    for (const auto &e : v) {
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
        for (const auto &s : singular)
            emplace(dict, s, me);
        for (const auto &s : plural)
            emplace(dict, s, me);
    }
};

struct Genus {
    bool m;
    bool n;
    bool f;

    Genus() : m(false), n(false), f(false) {}

    bool empty() const { return !(m || f || n); }
    void serialize(ostream &out) const {
        out << m << " " << n << " " << f << " ";
    }
    void deserialize(istream &in) { in >> m >> n >> f; }
};

inline bool operator<=(const Genus &lhs, const Genus &rhs) {
    if (lhs.m == rhs.m && lhs.f == rhs.f && lhs.n == rhs.n)
        return true;
    if (rhs.empty())
        return true;
    return false;
}

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

enum class Cases { Nominative = 0, Genitive = 1, Dative = 2, Accusative = 3 };

struct WithCases {
  private:
    void copy(const WithCases &cp) {
        for (size_t i = 0; i < 4; i++)
            cases[i] = cp.cases[i];
    }

  public:
    WithCases()
        : nominative(cases[(size_t)Cases::Nominative]),
          genitive(cases[(size_t)Cases::Genitive]),
          dative(cases[(size_t)Cases::Dative]),
          accusative(cases[(size_t)Cases::Accusative]) {}

    WithCases(const WithCases &cp) : WithCases() { copy(cp); }

    WithCases &operator=(const WithCases &A) {
        copy(A);
        return *this;
    }

    Numeri cases[4];
    Numeri &nominative, &genitive, &dative, &accusative;

    void serialize(ostream &out) const {
        for (size_t i = 0; i < 4; i++)
            cases[i].serialize(out << " ");
    }
    void deserialize(istream &in) {
        for (size_t i = 0; i < 4; i++)
            cases[i].deserialize(in);
    }

    bool empty() {
        return nominative.empty() && genitive.empty() && dative.empty() &&
               accusative.empty();
    }
};