
#pragma once
#include "grammar.hpp"

enum class NounType {
    Name,
    Noun,
    Numeral,
    Toponym,
    Incomplete,
};

const static thread_local vector<string> nDeclSuffix = {
    "and", "ant", "at",   "end", "et", "ent",  "graph", "ist",
    "ik",  "it",  "loge", "nom", "ot", "soph", "urg"};

class Noun
    : public WithCases { // See
                         // http://www.dietz-und-daf.de/GD_DkfA/Gramm-List.htm
  
  public:
    bool noPlural, noSingular;
    NounType type;
    Genus genus;

    Noun() : noPlural(false), noSingular(false) {}

    //////////////////////////////////////////////

    string ns() const;

    // Tests if n is a nominative singular
    bool ns(const stringLower &n) const {
        for (const stringLower s : nominative.singular)
            if (s == n)
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

    string np(const Dictionary &dict, bool forceArtificial = false) const;

    // Warning: This one is a little less robust than the test for the other
    // cases
    bool np(const Dictionary &dict, const stringLower &n) const {
        for (const stringLower s : nominative.plural)
            if (s == n)
                return true;
        if (nominative.plural.empty())
            if (n == stringLower(np(dict)))
                return true;
        return false;
    }

    string gs(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noSingular)
            return "";
        if (forceArtificial || genitive.singular.empty())
            return gsRules(forceArtificial);
        return genitive.singular[0];
    }

    // TODO: quite vague test
    bool gs(const Dictionary &dict, const stringLower &test) const {
        if (noSingular)
            return false;
        for (const stringLower g : genitive.singular) {
            if (g == test)
                return true;
        }
        if (genitive.singular.empty()) {
            const string ns = stringLower(this->ns());
            const bool m = genus.empty() || genus.m;
            const bool f = genus.empty() || genus.f;
            const bool n = genus.empty() || genus.n;

            if (isNDeclination()) {
                if (endsWith(ns, "e"))
                    return ns + "n" == test;
                return ns + "en" == test;
            } else if ((n || m) && endsWithAny(ns, sibilants)) {
                return ns + "es" == test;
            } else if (test == stringLower(gs(dict))) {
                return true;
            } else if (ns == test || ns + "s" == test) {
                return true;
            }
        }
        return false;
    }

    string gp(const Dictionary &dict,
              const bool forceArtificial = false) const {
        if (noPlural)
            return "";
        if (forceArtificial || genitive.plural.empty())
            return np(dict, false);
        return genitive.plural[0];
    }

    bool gp(const Dictionary &dict, const stringLower &test) const {
        if (noPlural)
            return false;
        for (const stringLower g : genitive.plural) {
            if (g == test)
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

    bool ds(const stringLower &test) const {
        if (noSingular)
            return "";
        for (const stringLower g : dative.singular) {
            if (g == test)
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

    bool dp(const Dictionary &dict, const stringLower &test) const {
        if (noPlural)
            return false;
        for (const stringLower g : dative.plural) {
            if (g == test)
                return true;
        }
        if (dative.plural.empty()) {
            const string np = this->np(dict);
            if (endsWithAny(np, {"e", "er", "el", "erl"}) &&
                !endsWithAny(np, {"ae"})) {
                if (!endsWith(test, "n"))
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

    bool as(const stringLower &test) const {
        if (noSingular)
            return "";
        for (const stringLower g : accusative.singular) {
            if (g == test)
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

    bool ap(const Dictionary &dict, const stringLower test) const {
        if (noPlural)
            return false;
        for (const stringLower g : accusative.plural) {
            if (g == test)
                return true;
        }
        if (accusative.plural.empty()) {
            return np(dict, test);
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////

    bool isNDeclination(bool forceArtificial = false) const;

    string npRules(const bool forceArtificial = false) const;

    string gsRules(const bool forceArtificial = false) const;

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

    bool predefined(const Cases &c, bool plural) const {
        if (plural)
            return !cases[size_t(c)].plural.empty() &&
                   !cases[size_t(c)].plural[0].empty();
        else
            return !cases[size_t(c)].singular.empty() &&
                   !cases[size_t(c)].singular[0].empty();
    }

    ////////////////////////////////////////////////////////////////////////////

    void serialize(ostream &out) const {
        WithCases::serialize(out);
        genus.serialize(out);
        out << noSingular << " " << noPlural << " " << (int)type << " ";
    }

    void deserialize(istream &in) {
        WithCases::deserialize(in);
        genus.deserialize(in);
        int temp;
        in >> noSingular >> noPlural >> temp;
        type = (NounType)temp;
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({this, WordType::Noun});
        nominative.buildMap(me, dict);
    }
};