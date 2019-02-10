
#pragma once
#include "grammar.hpp"

enum class NounType {
    Name,
    Noun,
    Numeral,
    Toponym,
};

const static thread_local vector<string> nDeclSuffix = {
    "and", "ant", "at",   "end", "et", "ent",  "graph", "ist",
    "ik",  "it",  "loge", "nom", "ot", "soph", "urg"};

enum class Cases { Nominative = 0, Genitive = 1, Dative = 2, Accusative = 3 };

struct Noun { // See http://www.dietz-und-daf.de/GD_DkfA/Gramm-List.htm
    Numeri cases[4];
    Numeri &nominative, &genitive, &dative, &accusative;

    Genus genus;
    NounType type;
    bool noPlural, noSingular;
    Noun()
        : noPlural(false), noSingular(false),
          nominative(cases[(size_t)Cases::Nominative]),
          genitive(cases[(size_t)Cases::Genitive]),
          dative(cases[(size_t)Cases::Dative]),
          accusative(cases[(size_t)Cases::Accusative]) {}

    string ns() const;

    // Tests if n is a nominative singular
    // Warning: This one is a little less robust than the test for the other
    // cases
    bool ns(string n) const {
        n = toLower(n);
        for (auto s : nominative.singular)
            if (toLower(s) == n)
                return true;
        return false;
    }

    bool np(const Dictionary &dict, string n) const {
        n = toLower(n);
        for (auto s : nominative.plural)
            if (toLower(s) == n)
                return true;
        if (nominative.plural.empty())
            if (n == toLower(np(dict)))
                return true;
        return false;
    }

    // Don't use this if you can have a dictionary
    string npWithoutDict(const bool forceArtificial = false) const {
        if (noPlural)
            return "";
        if (forceArtificial || nominative.plural.empty())
            return npRules(forceArtificial);
        return nominative.plural[0];
    }

    // Better version using the dictionary
    string np(const Dictionary &dict, bool forceArtificial = false) const;

    string npRules(const bool forceArtificial = false) const;

    string gs(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noSingular)
            return "";
        if (forceArtificial || genitive.singular.empty())
            return gsRules(forceArtificial);
        return genitive.singular[0];
    }

    // TODO: quite vague test
    bool gs(const Dictionary &dict, string test) const {
        if (noSingular)
            return false;
        test = toLower(test);
        for (auto g : genitive.singular) {
            if (toLower(g) == test)
                return true;
        }
        if (genitive.singular.empty()) {
            const string ns = toLower(this->ns());
            const bool m = genus.empty() || genus.m;
            const bool f = genus.empty() || genus.f;
            const bool n = genus.empty() || genus.n;

            if (isNDeclination()) {
                if (endsWith(ns, "e"))
                    return ns + "n" == test;
                return ns + "en" == test;
            } else if ((n || m) && endsWithAny(ns, sibilants)) {
                return ns + "es" == test;
            } else if (test == toLower(gs(dict))) {
                return true;
            } else if (ns == test || ns + "s" == test) {
                return true;
            }
        }
        return false;
    }

    string gsRules(const bool forceArtificial = false) const;

    string gp(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noPlural)
            return "";
        if (forceArtificial || genitive.plural.empty())
            return np(dict, false);
        return genitive.plural[0];
    }

    bool gp(const Dictionary &dict, string test) const {
        if (noPlural)
            return false;
        test = toLower(test);
        for (auto g : genitive.plural) {
            if (toLower(g) == test)
                return true;
        }
        if (genitive.plural.empty()) {
            return np(dict, test);
        }
        return false;
    }

    string ds(const bool forceArtificial = false) const {
        if (noSingular)
            return "";
        if (forceArtificial || dative.singular.empty()) {
            const string ns = this->ns();
            const bool m = genus.empty() || genus.m;
            if (m && endsWithAny(ns, nDeclSuffix)) {
                return ns + "en";
            } else if (endsWithAny(ns,
                                   {"er", "ar", "är", "eur", "ier", "or"})) {
                return ns;
            }
            if (isNDeclination()) {
                if (endsWith(ns,
                             "e")) { // missing: Bauer, Herr, Nachbar, Ungar...
                    return ns + "n";
                } else if (!endsWithAny(ns, vocals) && !endsWith(ns, "n")) {
                    return ns + "en";
                }
            }
            return ns;
        }
        return dative.singular[0];
    }

    bool ds(string test) const {
        test = toLower(test);
        if (noSingular)
            return "";
        for (auto g : dative.singular) {
            if (toLower(g) == test)
                return true;
        }
        if (dative.singular.empty()) {
            return ds() == test;
        }
        return false;
    }

    string dp(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noPlural)
            return "";
        if (forceArtificial || dative.plural.empty()) {
            const string np = this->np(dict, false);
            if (endsWithAny(np, {"e", "er", "el", "erl"}) &&
                !endsWithAny(np, {"ae"}))
                return np + "n";
            return np;
        }
        return dative.plural[0];
    }

    bool dp(const Dictionary &dict, string test) const {
        if (noPlural)
            return false;
        test = toLower(test);
        for (auto g : dative.plural) {
            if (toLower(g) == test)
                return true;
        }
        if (dative.plural.empty()) {
            const string np = this->np(dict);
            if (endsWithAny(np, {"e", "er", "el", "erl"}) &&
                !endsWithAny(np, {"ae"})) {
                if (!tryEatB(test, "n"))
                    return false;
            }
            return this->np(dict, test);
        }
        return false;
    }

    string as(const bool forceArtificial = false) const {
        if (noSingular)
            return "";
        if (forceArtificial || accusative.singular.empty()) {
            return ds();
        }
        return accusative.singular[0];
    }

    bool as(string test) const {
        test = toLower(test);
        if (noSingular)
            return "";
        for (auto g : accusative.singular) {
            if (toLower(g) == test)
                return true;
        }
        if (accusative.singular.empty()) {
            return as() == test;
        }
        return false;
    }

    string ap(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noPlural)
            return "";
        if (forceArtificial || accusative.plural.empty())
            return np(dict, false);
        return accusative.plural[0];
    }

    bool ap(const Dictionary &dict, string test) const {
        if (noPlural)
            return false;
        test = toLower(test);
        for (auto g : accusative.plural) {
            if (toLower(g) == test)
                return true;
        }
        if (accusative.plural.empty()) {
            return np(dict, test);
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////

    bool isNDeclination(bool forceArtificial = false) const;

    bool nullArticle() const {
        // TODO
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////

    string get(const Cases &c, bool plural, const Dictionary &dict,
               bool fa = false) const {
        switch (c) {
        case Cases::Nominative:
            return !plural ? ns() : np(dict, fa);
            break;
        case Cases::Genitive:
            return !plural ? gs(dict, fa) : gp(dict, fa);
            break;
        case Cases::Dative:
            return !plural ? ds(fa) : dp(dict, fa);
            break;
        case Cases::Accusative:
            return !plural ? as(fa) : ap(dict, fa);
            break;
        }
    }

    bool test(const Cases &c, bool plural, const Dictionary &dict,
              const string &t) const {
        switch (c) {
        case Cases::Nominative:
            return !plural ? ns(t) : np(dict, t);
            break;
        case Cases::Genitive:
            return !plural ? gs(dict, t) : gp(dict, t);
            break;
        case Cases::Dative:
            return !plural ? ds(t) : dp(dict, t);
            break;
        case Cases::Accusative:
            return !plural ? as(t) : ap(dict, t);
            break;
        }
    }

    bool prefined(const Cases &c, bool plural) {
        if (plural)
            return !cases[size_t(c)].plural.empty() &&
                   !cases[size_t(c)].plural[0].empty();
        else
            return !cases[size_t(c)].singular.empty() &&
                   !cases[size_t(c)].singular[0].empty();
    }

    ////////////////////////////////////////////////////////////////////////////

    void serialize(ostream &out) const {
        for (size_t i = 0; i < 4; i++)
            cases[i].serialize(out << " ");
        out << " " << genus.m << " " << genus.n << " " << genus.f << " "
            << noSingular << " " << noPlural << " " << (int)type << " ";
    }
    void deserialize(istream &in) {
        for (size_t i = 0; i < 4; i++)
            cases[i].deserialize(in);
        int temp;
        in >> genus.m >> genus.n >> genus.f >> noSingular >> noPlural >> temp;
        type = (NounType)temp;
    }
    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({this, WordType::Noun});
        nominative.buildMap(me, dict);
    }
};