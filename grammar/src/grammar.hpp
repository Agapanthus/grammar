#pragma once

#include <ostream>
using std::ostream;

#include <string>
using std::string, std::to_string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <tuple>
using std::pair;

#include "util.hpp"

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

vector<string> breakKompositum(const string &word, const Dictionary &dict);
void emplace(map<string, vector<Word>> &dict, const string &s, const Word &w);

const thread_local vector<string> vocals = {"a", "e", "i", "o",
                                            "u", "ö", "ä", "ü"};
const thread_local vector<string> vocalsPlus = {
    "au", "ei", "eu", "äu", "a", "e", "i", "o", "u", "ö", "ä", "ü"};

//////////////////////////////////////

template <typename T>
void serializeVector(ostream &out, const vector<T> &v, bool nl = true) {
    out << v.size() << (nl ? "\n" : " ");
    if (v.size() > 1024 * 1024 * 1024)
        throw std::exception("invalid");
    for (auto e : v) {
        e.serialize(out);
        out << (nl ? "\n" : " ");
    }
}

template <>
void serializeVector(ostream &out, const vector<string> &v, bool nl) {
    out << (nl ? "\n" : " ") << v.size() << " ";
    if (v.size() > 1024 * 1024 * 1024)
        throw std::exception("invalid");
    for (auto e : v) {
        out << e.size() << " " << e << "";
    }
}

template <typename T> void deserializeVector(istream &in, vector<T> &v) {
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

template <> void deserializeVector(istream &in, vector<string> &v) {
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

enum class NounType {
    Name,
    Noun,
    Numeral,
    Toponym,
};

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

struct Noun {
    Numeri nominative, genitive, dative, accusative;
    Genus genus;
    NounType type;

    // Tests if n is a nominative singular
    bool ns(string n) const {
        n = toLower(n);
        for (auto s : nominative.singular)
            if (toLower(s) == n)
                return true;
        return false;
    }

    string ns() const;

    bool np(string n) const {
        n = toLower(n);
        for (auto s : nominative.plural)
            if (toLower(s) == n)
                return true;
        if (nominative.plural.empty())
            if (n == toLower(np()))
                return true;
        return false;
    }

    // Don't use this if you can have a dictionary (see the overladed method)
    string np(bool forceArtificial = false) const {
        if (forceArtificial || nominative.plural.empty()) {
            string s = ns();
            return npRules(s);
        }
        return nominative.plural[0];
    }

    // Better version using the dictionary
    string np(const Dictionary &dict, bool forceArtificial = false) const;

    string npRules(string s) const {
        const bool m = genus.empty() || genus.m;
        const bool f = genus.empty() || genus.f;
        const bool n = genus.empty() || genus.n;

        ///////////////////////////////////// Special Rules

        if (startsWith(s, "Ge") && endsWith(s, "e")) {
            return s;
        }

        ///////////////////////////////////// Eric's Suffix Rules

        if (tryEatB(s, "ismus"))
            return s + "ismen";
        else if (endsWithAny(s, {"y", "ck"})) // English
            return s + "s";
        else if (endsWithAny(s, {"é"})) // Französisch
            return s + "s";

        ///////////////////////////////////// Strong Suffix Rules

        if (m && endsWith(s, "loge")) {
            return s + "n";
        } else if (f && endsWithAny(s, {"ade", "age", "euse", "elle", "ie",
                                        "ine", "isse", "ive", "ose"})) {
            return s + "n";
        }
        if (m && endsWithAny(
                     s, {"chen", "iker", "lein", "ler", "men", "ner", "sel"})) {
            return s;
        }

        if (m && endsWithAny(s, {"and", "ant", "ent", "ismus", "ist", "it",
                                 "nom", "os", "soph"})) { // nicht: Labor
            if (tryEatB(s, "us")) {
            } else if (tryEatB(s, "os")) {
            }
            return s + "en";
        } else if (n && endsWithAny(s, {"os"})) {
            if (tryEatB(s, "os")) {
            }
            return s + "en";
        } else if (n && endsWithAny(s, {"tum"})) {
            if (tryEatB(s, "tum"))
                return s + "tümer";
            return s + "er";
        }

        if ((m || n) &&
            endsWithAny(s, {"al", "är", "an", "an", "ar", "ell", "eur", "iv",
                            "il", "ling", "nis", "onym",
                            "sal"})) { // nicht: Kanal, Doktor, Atelier, Schal
            return s + "e";
        }

        if (f && endsWithAny(s, {"anz", "enz", "heit", "igkeit", "ik", "in",
                                 "keit", "schaft", "tät", "ung", "ur"})) {
            /*if (!endsWith(s, "tion")) {
                tryEatB(s, "ion");
            } else*/
            if (tryEatB(s, "a")) {
            } else if (tryEatB(s, "um")) {
            }
            return s + "en";
        } else if (f && endsWith(s, "nis")) {
            return s + "se";
        }

        // Last vocal accentuated
        if (m && endsWith(s, "on")) {
            return s + "s";
        }

        ///////////////////////////////////// Weak Suffix Rules

        if (m && endsWithAny(s, {"ier"})) {
            return s;
        }

        if (m && endsWithAny(s, {"ekt", "at", "et", "on", "or",
                                 "us"})) { // nicht: Labor
            if (tryEatB(s, "us")) {
            }
            return s + "en";
        } else if (n && endsWithAny(s, {"ion", "ma", "um", "ut"})) {
            if (tryEatB(s, "a")) {
            } else if (tryEatB(s, "on")) {
            } else if (tryEatB(s, "um"))
                return s + "en";
        }

        if ((m || n) &&
            endsWithAny(s, {"at", "ekt", "et", "ett", "ier", "in", "ion", "ix",
                            "ling", "ment", "or", "on",
                            "ut"})) { // nicht: Kanal, Doktor, Atelier, Schal
            return s + "e";
        }

        if (f && endsWithAny(s, {"ion"})) {
            if (!endsWith(s, "tion")) {
                tryEatB(s, "ion");
            } else if (tryEatB(s, "a")) {
            } else if (tryEatB(s, "um")) {
            }
            return s + "en";
        }

        // Last vocal accentuated
        if (n && endsWithAny(s, {"et", "ie", "ier", "in", "ment",
                                 "or"})) { // nicht: Offizier
            return s + "s";
        } else if (f && endsWith(s, "ie")) {
            return s + "s";
        }

        ///////////////////////////////////// Advanced Rules

        if ((m || n) &&
            endsWithAny(s, {"el", "er", "en"})) { // 90% TODO: unbetont
            return s;
        } else if (f &&
                   endsWithAny(
                       s, {"el", "er"})) { // 90% (nicht: Mutter, Tochter...)
            return s + "n";
        }

        ///////////////////////////////////// Additional Rules

        if (f && endsWithAny(s, {"ee", "ie", "ei"})) {
            return s + "n";
        } else if (endsWith(s, "e")) { // 98%
            return s + "n";
        } else if (endsWithAny(s, {"a", "o", "i",
                                   "u"})) { // 70% (nicht: Villa, Tempo...)
            return s + "s";
        }

        ///////////////////////////////////// Basic Rules

        if (m) { // 90% (nicht: Mensch...)
            if (countInOrder(toLower(s), vocalsPlus) <=
                1) { // one syllable in stem
                return umlautify(s) + "e";
            } else
                return s + "e";
        } else if (n) { // 75% (nicht: Maß, Haus...)
            return s + "er";
        } else /*if (f)*/ { // 75% (nicht: Hand...)
            return s + "en";
        }
    }

    void serialize(ostream &out) const {
        nominative.serialize(out);
        genitive.serialize(out << " ");
        dative.serialize(out << " ");
        accusative.serialize(out << " ");
        out << " " << genus.m << " " << genus.n << " " << genus.f << " "
            << (int)type << " ";
    }
    void deserialize(istream &in) {
        nominative.deserialize(in);
        genitive.deserialize(in);
        dative.deserialize(in);
        accusative.deserialize(in);
        int temp;
        in >> genus.m >> genus.n >> genus.f >> temp;
        type = (NounType)temp;
    }
    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({this, WordType::Noun});
        nominative.buildMap(me, dict);
    }
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

/*
struct Diathesis {
    Person active, processualPassive,
        statalPassive; // TODO: , causative, applicative;
};*/

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

struct Adjective {
    vector<string> positive, comparative, superlative;

    void serialize(ostream &out) const {
        serializeVector(out, positive);
        serializeVector(out, comparative);
        serializeVector(out, superlative);
    }

    void deserialize(istream &in) {
        deserializeVector(in, positive);
        deserializeVector(in, comparative);
        deserializeVector(in, superlative);
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({(Noun *)this, WordType::Adjective});
        for (auto s : positive)
            emplace(dict, s, me);
        for (auto s : comparative)
            emplace(dict, s, me);
        for (auto s : superlative)
            emplace(dict, s, me);
    }
};

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

/////////////////////////////////

void emplace(map<string, vector<Word>> &dict, const string &s, const Word &w) {

    auto [it, suc] = dict.emplace(toLower(s), vector({w}));
    if (!suc)
        it->second.push_back(w);
}

bool isKStem(const string &str,
             const Dictionary &dict) { // TODO: not very robust, isn't it?
    vector<Word> words = dict.find(str);
    for (const Word w : words) {
        switch (w.w) {
        case WordType::Noun:
            if (w.noun->type == NounType::Noun) {
                if (w.noun->ns(str)) {
                    return true;
                } else if (w.noun->np(str)) {
                    return true;
                }
            }
            break;

        case WordType::Adjective:
            if (!w.adj->positive.empty())
                for (string pos : w.adj->positive)
                    if (toLower(pos) == toLower(str))
                        return true;
            break;

        default:
            break;
        }
    }
    return false;
}

#include "grammarIO.hpp"

bool isKHead(const string &str, const Dictionary &dict) {
    vector<Word> words = dict.find(str);
    for (const Word w : words) {
        switch (w.w) {
        case WordType::Noun:
            if (w.noun->type == NounType::Noun)
                return true; // It's a Noun, perfect match. We want nothing
                             // more.
            break;
        default:
            break;
        }
    }
    return false;
}

vector<string> breakKompositum(const string &word, const Dictionary &dict) {
    if (word.size() < 4)
        return {word};
    for (size_t i = 3; i < word.size() - 3; i++) {
        const string start = word.substr(0, i);
        string end = word.substr(i, word.size() - i);
        bool allOk = false;
        vector<string> begin = vector({start});
        if (isKStem(start, dict)) {
            if (isKHead(end, dict)) {
                allOk = true;
            }

            // Fugenelement (there are also others, but I don't really care -
            // see https://de.wikipedia.org/wiki/Komposition_(Grammatik))
            else if (word[i] == 's' &&
                     !isAnyOf(string("") + word[i - 1], vocals) &&
                     !isAnyOf(string("") + word[i + 1], vocals)) {
                end = word.substr(i + 1, word.size() - i - 1);
                if (isKHead(end, dict)) {
                    begin.push_back("s");
                    allOk = true;
                }
            }
        }
        if (allOk) {
            const vector<string> a = breakKompositum(end, dict);
            begin.insert(begin.end(), a.begin(), a.end());
            return begin;
        }
    }
    return {word};
}

string Noun::np(const Dictionary &dict, bool forceArtificial) const {
    if (forceArtificial || nominative.plural.empty()) {
        string s = ns();

        vector<string> v = breakKompositum(s, dict);
        if (v.size() <= 1)
            return np(forceArtificial);
        const string head = v[v.size() - 1];
        string headPl;
        for (Word w : dict.find(head)) {
            if (w.w == WordType::Noun) {
                if (w.noun->ns(head)) {
                    headPl = toLower(w.noun->np(forceArtificial));
                    break;
                }
            }
        }
        if (headPl.empty())
            return np(forceArtificial);

        v.pop_back();
        string res;
        for (auto vv : v)
            res += vv;
        return res + headPl;
    }
    return nominative.plural[0];
}

string Noun::ns() const {
    if (nominative.singular.empty()) {
        if (nominative.plural.empty()) {
            cout << endl << "Error: nominative!" << endl;
            cout << *this << endl;
            throw Exception("nominative needed!");
        } else {
            cout << endl
                 << "Error: Singular doesn't exist! " << nominative.plural[0]
                 << endl;
            return "";
        }
    }
    return nominative.singular[0];
}
