#include "noun.hpp"
#include "dictionary.hpp"
#include "grammarIO.hpp"
#include "kompositum.hpp"

#include <iostream>
using std::cout, std::endl;

string Noun::np(const Dictionary &dict, bool forceArtificial) const {
    if (noPlural)
        return "";
    if (forceArtificial || nominative.plural.empty()) {
        string s = ns();

        vector<string> v = breakKompositum(s, dict);
        if (v.size() <= 1)
            return npWithoutDict(forceArtificial);
        const string head = v[v.size() - 1];
        string headPl;
        for (Word w : dict.find(head)) {
            if (w.w == WordType::Noun) {
                if (w.noun->ns(head)) {
                    headPl = toLower(w.noun->npWithoutDict(forceArtificial));
                    break;
                }
            }
        }
        if (headPl.empty())
            return npWithoutDict(forceArtificial);

        v.pop_back();
        string res;
        for (auto vv : v)
            res += vv;
        return res + headPl;
    }
    return nominative.plural[0];
}

string Noun::ns() const {
    if (noSingular)
        return "";
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

string Noun::npRules(const bool forceArtificial) const {
    string s = ns();

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
    } else if (f && endsWithAny(s, {"ade", "age", "euse", "elle", "ie", "ine",
                                    "isse", "ive", "ose"})) {
        return s + "n";
    }
    if (m &&
        endsWithAny(s, {"chen", "iker", "lein", "ler", "men", "ner", "sel"})) {
        return s;
    }

    if (m && endsWithAny(s, {"and", "ant", "ent", "ismus", "ist", "it", "nom",
                             "os", "soph"})) { // nicht: Labor
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
        endsWithAny(s, {"al", "är", "an", "an", "ar", "ell", "eur", "iv", "il",
                        "ling", "nis", "onym",
                        "sal"})) { // nicht: Kanal, Doktor, Atelier, Schal
        if (tryEatB(s, "nis"))
            return s + "nisse";
        return s + "e";
    }

    if (f && endsWithAny(s, {"anz", "enz", "heit", "igkeit", "ik", "in", "keit",
                             "schaft", "tät", "ung", "ur"})) {
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

    if (m &&
        endsWithAny(s, {"ekt", "at", "et", "on", "or", "us"})) { // nicht: Labor
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

    if ((m || n) && endsWithAny(s, {"el", "er", "en"})) { // 90% TODO: unbetont
        /*if (countInOrder(toLower(s), vocalsPlus) <=
            2) { // one syllable without the ending
            return umlautify(s);
        }*/
        return s;
    } else if (f && endsWithAny(
                        s, {"el", "er"})) { // 90% (nicht: Mutter, Tochter...)
        return s + "n";
    }

    ///////////////////////////////////// Additional Rules

    if (f && endsWithAny(s, {"ee", "ie", "ei"})) {
        return s + "n";
    } else if (endsWith(s, "e")) { // 98%
        return s + "n";
    } else if (endsWithAny(
                   s, {"a", "o", "i", "u"})) { // 70% (nicht: Villa, Tempo...)
        return s + "s";
    }

    ///////////////////////////////////// Basic Rules

    if (m) { // 90% (nicht: Mensch...)
        if (countInOrder(toLower(s), vocalsPlus) <= 1) { // one syllable
            return umlautify(s) + "e";
        } else
            return s + "e";
    } else if (n) { // 75% (nicht: Maß, Haus...)
        return s + "er";
    } else /*if (f)*/ { // 75% (nicht: Hand...)
        return s + "en";
    }
}

string Noun::gsRules(const bool forceArtificial) const {
    string s = ns();

    const bool m = genus.empty() || genus.m;
    const bool f = genus.empty() || genus.f;
    const bool n = genus.empty() || genus.n;

    switch (type) {
    case NounType::Name:
        if (endsWithAny(s, sibilants)) {
            return s + "'"; // or -ens
            // TODO: Herr -> Herrn, Kollege -> Kollegen
        } else {
            return s + "s";
        }
        break;
    case NounType::Toponym:
        if (f || endsWithAny(s, sibilants)) {
            if (nullArticle()) {
                return "von " + ds();
            } else {
                return s;
            }
        } else if (m || n) {
            return s + "s"; // -s is optional if there is an adjective attribute
        }
        // Fallthrough
    case NounType::Noun:
        if (endsWithAny(s, {"ismus", "os"})) {
            return s;
        } else if (endsWithAny(s, {"us"}) &&
                   !endsWithAny(s, {"aus", "eus"})) { // IMHO this is true
            return s;
        } else if (m || n) {
            if (isNDeclination(forceArtificial)) {
                tryEatB(s, "e");
                return s + "en";
            } else {
                if (endsWithAny(s, vocals) || endsWith(s, "h")) {
                    return s + "s";
                } else if (endsWith(s, "is") &&
                           !endsWithAny(s, {"eis", "ais"})) {
                    return s + "ses";
                } else if (endsWithAny(s, sibilants)) {
                    return s + "es";
                } else if (endsWithAny(s, {"sch", "st", "zt"})) { // Mostly true
                    return s + "es";
                } /*else if (dict.isAdjective(s) ||
                           dict.isVerb(s)) { // Singen, Grün
                    return s + "s";
                } else if() {
                    // If there is only one syllable or the last syllable is
                    // pronounced it is not important if you use -s or -es
                    return {s + "s", s + "es"};
                } */
                else {
                    return s + "s";
                }
            }
        } else if (true || f) {
            return s;
        }
        break;
    case NounType::Numeral:
        return s;
        break;
    }
}

bool Noun::isNDeclination(bool forceArtificial) const {

    const bool f = genus.empty() || genus.f;
    const bool n = genus.empty() || genus.n;
    if (f)
        return false;
    if (n)
        return ns() == "Herz";

    if (endsWith(ns(), "en")) // TODO: overthink this.
        return false;

    if (!forceArtificial) {
        bool allOk = true;
        for (size_t i = 0; i < 4; i++)
            if (cases[i].singular.empty() || cases[i].plural.empty())
                allOk = false;
        if (allOk) {
            allOk = true;
            for (size_t i = 0; i < 4; i++) {
                bool nConform = false;
                for (auto g : cases[i].singular) {
                    nConform |= endsWith(g, "n");
                    if (i == size_t(Cases::Genitive))
                        nConform |= endsWith(g, "ns");
                }
                if (!nConform) {
                    allOk = false;
                    break;
                }
                nConform = false;
                for (auto g : cases[i].plural)
                    nConform |= endsWith(g, "n");
                if (!nConform) {
                    allOk = false;
                    break;
                }
            }
            return allOk;
        }
    }

    {
        // Mostly humans and animals,
        // but also Friede, Name, Buchstabe, Herz...
        const bool m = genus.empty() || genus.m;
        const string ns = this->ns();
        if (m && endsWith(ns, "e") && !endsWithAny(ns, {"ie"}))
            return true;
        if (m && endsWithAny(ns, nDeclSuffix))
            return true;
    }
    return false;
}
