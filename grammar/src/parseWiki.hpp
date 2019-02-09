#pragma once

#include <algorithm>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
using std::cout, std::cin, std::endl;
using std::istream, std::ostream;
using std::shared_ptr, std::make_shared;
using std::string;
using std::tuple, std::get;
using std::vector;

#include "grammar.hpp"
#include "util.hpp"

namespace parseWiki {

// If the string looks like something empty, delete it
string saniWiki(const string &cstr) {
    string str;
    str.reserve(cstr.size());
    for (size_t i = 0; i < cstr.size(); i++) {
        if (cstr[i] == '&') {
            // Ampersand will usually start some HTML-Comment but if it
            // is the &-HTML-Entity, keep it and proceed
            if (cstr.size() - i > 5) {
                if (cstr.substr(i, 5) == "&amp;") {
                    i += 4;
                    str.push_back('&');
                    continue;
                }
            }
            break;
        }
        str.push_back(cstr[i]);
    }
    str = trim(str);
    if (str == "-" || str == "0" || str == "—" || str == "?")
        return "";

    replaceHere(str, "’", "'");
    str = removeSub(str, "·");
    fixUTF8(str);
    trimHere(str);

    if (str.length() > 100)
        return ""; // invalid

    return str;
}

string readUntil(shared_ptr<istream> &in, const string &ref) {
    string content = "";
    string buf = "";
    for (size_t i = 0; i < ref.size(); i++)
        buf += "\n";
    while (true) {
        if (buf == ref)
            break;
        for (size_t i = 1; i < buf.size(); i++)
            buf[i - 1] = buf[i];
        char n;
        if (!in->get(n))
            return "\0";
        buf[buf.size() - 1] = n;
        content += n;
    }
    return content;
}

string readTag(shared_ptr<istream> &in, const string &tag) {
    if (readUntil(in, "<" + tag + ">") == "\0")
        return "\0";
    const string content = readUntil(in, "</" + tag + ">");
    if (content.length() < (tag.length() + 3))
        return "\0";
    return content.substr(0, content.length() - tag.length() - 3);
}

string readTag(const string &container, const string &tag) {
    shared_ptr<istream> is(make_shared<std::istringstream>(container));
    return readTag(is, tag);
}

template <size_t which> string match(const string &text, const std::regex &re) {

    std::sregex_iterator next(text.begin(), text.end(), re,
                              std::regex_constants::match_any |
                                  std::regex_constants::match_not_null);
    std::sregex_iterator end;
    while (next != end) {
        std::smatch match = *next;
        for (size_t i = 0; i < match.size(); ++i) {
            if (i == which)
                return match[i];
        }
        next++;
    }

    return "";
}

template <size_t which>
vector<string> all(const string &text, const std::regex &re) {
    vector<string> results;
    std::sregex_iterator next(text.begin(), text.end(), re,
                              std::regex_constants::match_any |
                                  std::regex_constants::match_not_null);
    std::sregex_iterator end;
    while (next != end) {
        std::smatch match = *next;
        for (size_t i = 0; i < match.size(); ++i) {
            if (i == which) {
                results.push_back(match[i]);
                break;
            }
        }
        next++;
    }

    return results;
}

template <size_t which>
vector<tuple<string, string>> leading(const string &text) {
    vector<tuple<string, string>> res;
    bool lastWasNewLineOrFirst = true;
    for (size_t i = 0; i < text.size() - 1; i++) {
        if (lastWasNewLineOrFirst) {
            if (text[i] == '{' && text[i + 1] == '{') {
                string match;
                match.reserve(512);
                for (i += 2; i < text.size() - 1; i++) {
                    if (text[i] == '}' && text[i + 1] == '}')
                        break;
                    match.push_back(text[i]);
                }
                match.shrink_to_fit();
                string contentA;
                contentA.reserve(512);
                if (text.size() > i + 4) {
                    i += 2;
                    if (text[i] == '\n' && text[i + 1] == ':') {
                        for (i += 2; i < text.size() - 2; i++) {
                            contentA.push_back(text[i]);
                            if (text[i] == '\n' && text[i + 1] == '{' &&
                                text[i + 2] == '{')
                                break;
                        }
                    }
                }
                contentA.shrink_to_fit();
                res.push_back(tuple(match, contentA));
            }
        }
        if (text[i] == '\n')
            lastWasNewLineOrFirst = true;
        else
            lastWasNewLineOrFirst = false;
    }
    return res;
}

template <size_t which>
vector<tuple<string, string>> leading(const string &text, const std::regex &re,
                                      const bool includeFirst = false) {
    vector<tuple<string, string>> results;
    std::sregex_iterator next(text.begin(), text.end(), re,
                              std::regex_constants::match_any |
                                  std::regex_constants::match_not_null);
    std::sregex_iterator end;

    if (includeFirst) {
        string content;
        if (next != end) {
            std::smatch nextMatch = *next;
            auto f = nextMatch[0].first;
            content = string(text.begin(), f);
        } else
            content = text;
        results.push_back(tuple("", content));
    }

    while (next != end) {
        std::smatch match = *next;
        bool needsInc = true;
        string matches;
        for (size_t i = 0; i < match.size(); ++i) {
            if (i == which) {
                matches = match[i];
                break;
            }
        }

        next++;
        if (match.size() > 0) {
            string content = "";
            if (next != end) {
                auto s = match[0].second;
                std::smatch nextMatch = *next;
                auto f = nextMatch[0].first;
                content = string(s, f);
            } else {
                auto s = match[0].second;
                content = string(s, text.end());
            }
            results.push_back(tuple(matches, trim(content)));
            continue;
        }
    }

    return results;
}

vector<string> parseSectionTitle(const string &title) {
    const static thread_local std::regex parens(
        R"(\}\}, \{\{)");
    return vecMap<string>(
        leading<1>(title, parens, true),
        [](const tuple<string, string> x) { return trim(get<1>(x)); });
}

template <size_t max> bool collectNumeri(string &prop, Numeri &target) {
    if (tryEat(prop, "Singular"))
        return collectStarsNumbers<max>(prop, target.singular);
    else if (tryEat(prop, "Plural"))
        return collectStarsNumbers<max>(prop, target.plural);
    return false;
}

template <size_t max>
bool collectNumbered(string &prop, vector<string> &target) {
    trimHere(prop);
    for (size_t i = 0; i <= max; i++) {
        const string base = (i > 0 ? std::to_string(i) : "");
        if (tryEat(prop, base + "=") || tryEat(prop, base + "*=") ||
            tryEat(prop, base + " =") || tryEat(prop, base + "**=")) {
            target.push_back(saniWiki(prop));
            return true;
        }
    }
    return false;
}

template <size_t max> bool collectStars(string &prop, vector<string> &target) {
    string stars = "";
    trimHere(prop);
    for (size_t i = 0; i <= max; i++) {
        if (tryEat(prop, stars + "=") || tryEat(prop, "stark" + stars + "=") ||
            tryEat(prop, "schwach" + stars + "=") ||
            tryEat(prop, "gemischt" + stars + "=")) {
            target.push_back(saniWiki(prop));
            return true;
        }
        stars += "*";
    }
    return false;
}

template <size_t max>
bool collectStarsNumbers(string &prop, vector<string> &target) {
    if (collectStars<max>(prop, target))
        return true;
    if (collectNumbered<max>(prop, target))
        return true;

    cout << "Error: Collection failed : " << prop << endl;
    return false;
}

void collectPersons(string &prop, Person &target) {
    string stars = "";
    for (size_t i = 0; i < 4; i++) {
        if (tryEat(prop, "ich" + stars + "="))
            target.first.singular.push_back(saniWiki(prop));
        else if (tryEat(prop, "du" + stars + "="))
            target.second.singular.push_back(saniWiki(prop));
        else if (tryEat(prop, "er, sie, es" + stars + "="))
            target.third.singular.push_back(saniWiki(prop));
        else
            stars += "*";
    }
}

const static thread_local std::regex vertSep(R"(\n\|)");
void buildNoun(const string &type, const string &mType, const string &body,
               const string &title, Dictionary &dict,
               const string &worttrennung) {
    Noun n;
    bool some = false;
    if (has(type, "name") || has(mType, "name") || has(type, "Name") ||
        has(mType, "Name")) {
        n.type = NounType::Name;
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    } else if (has(type, "Toponym") || has(mType, "Toponym")) {
        n.type = NounType::Toponym;
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    } else if (has(mType, "Numeral")) {
        n.type = NounType::Numeral;
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    } else if (has(type, "Substantiv") || has(mType, "Substantiv")) {
        n.type = NounType::Noun;
    } else {
        n.type = NounType::Name;
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    }

    // TODO: Toponym genus in title
    // TODO: kPl. in Worttrennung

    for (const auto c_prop : leading<1>(body, vertSep)) {
        some = true;
        string prop(get<1>(c_prop));
        if (tryEat(prop, "Nominativ "))
            collectNumeri<5>(prop, n.nominative);
        else if (tryEat(prop, "Nominativ") || tryEat(prop, "Singular"))
            collectStarsNumbers<5>(prop, n.nominative.singular);
        else if (tryEat(prop, "Plural"))
            collectStarsNumbers<5>(prop, n.nominative.plural);
        else if (tryEat(prop, "Genitiv "))
            collectNumeri<5>(prop, n.genitive);
        else if (tryEat(prop, "Genitiv"))
            collectStarsNumbers<5>(
                prop,
                n.genitive.singular); // names like Dativ*=Jesu
        else if (tryEat(prop, "Dativ "))
            collectNumeri<5>(prop, n.dative);
        else if (tryEat(prop, "Dativ"))
            collectStarsNumbers<5>(prop, n.dative.singular);
        else if (tryEat(prop, "Akkusativ "))
            collectNumeri<5>(prop, n.accusative);
        else if (tryEat(prop, "Akkusativ"))
            collectStarsNumbers<5>(prop, n.accusative.singular);
        else if (tryEat(prop, "Genus=") || tryEat(prop, "Genus =") ||
                 tryEat(prop, "Genus 1=") || tryEat(prop, "Genus 2=") ||
                 tryEat(prop, "Genus 3=") || tryEat(prop, "Genus 4=")) {
            n.genus.n = has(prop, "n");
            n.genus.m = has(prop, "m");
            n.genus.f = has(prop, "f");
        } else if (tryEat(prop, "kein Singular=") ||
                   has(worttrennung, "{{kSg.}}")) {
            n.noSingular = true;
        } else if (tryEat(prop, "kein Plural=") ||
                   has(worttrennung, "{{kPl.}}")) {
            n.noPlural = true;
        } else if (tryEat(prop, "Bild") || tryEat(prop, "Artikel=Klammer") ||
                   tryEat(prop, "Kein-ens=") || tryEat(prop, "Kein-s=") ||
                   tryEat(prop, "Stamm=") || tryEat(prop, "kein-s=")) {
            // drop
        } else {
            cout << "Error: unknown prop subs: " << prop << " (" << mType << ")"
                 << endl;
        }
    }

    if (n.nominative.singular.empty() && endsWith(title, "sch")) {
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    }

    // Is this generally ok?
    if (!some) {
        n.nominative.singular.push_back(saniWiki(title));
        some = true;
    }

    if (some || n.type == NounType::Name || n.type == NounType::Toponym)
        dict.add(n);
    else
        cout << "Error: " << title << " was empty" << endl;
}

void parseWord(Dictionary &dict, const string &title, const string &type,
               const string &body, const string &worttrennung, bool print,
               const string &mType) {

    if (print)
        cout << "      " << title << endl;

    if (has(type, "Verb") || type == "verb" || has(mType, "Verb")) {
        Verb v;
        bool some = false;
        for (const auto c_prop : leading<1>(body, vertSep)) {
            some = true;
            string prop(get<1>(c_prop));
            if (tryEat(prop, "Präsens_"))
                collectPersons(prop, v.base.present);
            else if (tryEat(prop, "Gegenwart_"))
                collectPersons(prop, v.base.present);
            else if (tryEat(prop, "Präteritum_ich"))
                collectStarsNumbers<5>(prop, v.base.preterite_firstSingular);
            else if (tryEat(prop, "Partizip II"))
                collectStarsNumbers<5>(prop, v.base.participleII);
            else if (tryEat(prop, "Konjunktiv II_ich"))
                collectStarsNumbers<5>(prop,
                                       v.base.subjunctiveII_firstSingular);
            else if (tryEat(prop, "Konjunktiv I_ich"))
                collectStarsNumbers<5>(prop, v.base.subjunctiveI_firstSingular);
            else if (tryEat(prop, "Imperativ "))
                collectNumeri<5>(prop, v.imperative);
            else if (tryEat(prop, "Befehl_du"))
                collectStarsNumbers<5>(prop, v.imperative.singular);
            else if (tryEat(prop, "Befehl_ihr"))
                collectStarsNumbers<5>(prop, v.imperative.plural);
            else if (tryEat(prop, "Hilfsverb")) {
                collectStarsNumbers<5>(prop, v.base.auxiliary);
            } else if (tryEat(prop, "unpersönlich=")) {
                // TODO
            } else if (tryEat(prop, "Weitere Konjugationen=") ||
                       tryEat(prop, "Weitere_Konjugationen=")) {
                // TODO
            } else if (tryEat(prop, "Bild") || tryEat(prop, "Flexion=")) {
                // drop
            } else {
                cout << "Error: unknown prop verb: " << prop << endl;
            }
        }
        if (v.base.presentInfinitive.empty())
            v.base.presentInfinitive.push_back(saniWiki(title));
        if (some)
            dict.add(v);
        else
            cout << "Error: " << title << " was empty" << endl;

    } else if (has(type, "Adjektiv") || has(mType, "Adjektiv")) {
        Adjective a;
        bool some = false;
        for (const auto c_prop : leading<1>(body, vertSep)) {
            some = true;
            string prop(get<1>(c_prop));
            if (tryEat(prop, "Positiv"))
                collectStarsNumbers<5>(prop, a.positive);
            else if (tryEat(prop, "Komparativ"))
                collectStarsNumbers<5>(prop, a.comparative);
            else if (tryEat(prop, "Superlativ"))
                collectStarsNumbers<5>(prop, a.superlative);
            else if (tryEat(prop, "keine weiteren Formen=ja") ||
                     tryEat(prop, "keine weiteren Formen=1")) {
                // TODO
            } else if (tryEat(prop, "am=0")) {
                // TODO
            } else if (tryEat(prop, "Bild") || tryEat(prop, "Flexion=")) {
                // drop
            } else {
                cout << "Error: unknown prop adj: " << prop << endl;
            }
        }
        if (some)
            dict.add(a);
        else
            cout << "Error: " << title << " was empty" << endl;

    } else if (has(type, "ronomen") || has(mType, "ronomen")) {
        Pronoun p;
        const static thread_local vector<pair<string, pronounType>> lookup = {
            {"Personalpronomen", pronounType::Personal},
            {"Reflexives Personalpronomen", pronounType::Personal},
            {"Possessivpronomen", pronounType::Possessive},
            {"Reflexivpronomen", pronounType::Reflexive},
            {"Interrogativpronomen", pronounType::Interrogative},
            {"Reziprokpronomen", pronounType::Reciprocal},
            {"Demonstrativpronomen", pronounType::Demonstrative},
            {"Relativpronomen", pronounType::Relative},
            {"Indefinitpronomen", pronounType::Indefinite},
        };
        p.type = pronounType::unknown;
        for (auto l : lookup) {
            if (type == l.first || has(mType, l.first)) {
                p.type = l.second;
                break;
            }
        }
        if (p.type == pronounType::unknown) {
            cout << "Error: unknown pronoun " << type << endl;
        }

        bool some = false;
        for (const auto c_prop : leading<1>(body, vertSep)) {
            some = true;
            string prop(get<1>(c_prop));
            if (tryEat(prop, "Nominativ "))
                collectNumeri<5>(
                    prop,
                    p.nominative.first); // TODO: Determine Person! (Or better:
                                         // Remove person from structure. Simply
                                         // use 'person' as enum)
            else if (tryEat(prop, "Genitiv "))
                collectNumeri<5>(prop, p.genitive.first);
            else if (tryEat(prop, "Dativ "))
                collectNumeri<5>(prop, p.dative.first);
            else if (tryEat(prop, "Akkusativ "))
                collectNumeri<5>(prop, p.accusative.first);
            else if (tryEat(prop, "Bild")) {
                // drop
            } else {
                cout << "Error: unknown prop: " << prop << endl;
            }
        }

        // Something like
        // {{Worttrennung}}
        // ich, {{Gen.}} mei·ner, {{va.|:}} mein, {{Dat.}} mir, {{Akk.}} mich;
        // {{Pl.}} wir
        if (worttrennung.size() > 3) {
            const static thread_local std::regex wortRe(R"(,|;)");
            vector<string> sections =
                vecMap<string>(leading<1>(worttrennung, wortRe, true),
                               [](auto p) { return trim(get<1>(p)); });
            bool plural = false;
            Person *base = &p.nominative;
            bool first = true;
            bool pluralAp = false;

            for (string s : sections) {
                if (tryEat(s, "{{Gen.}}")) {
                    base = &p.genitive;
                } else if (tryEat(s, "{{Dat.}}")) {
                    base = &p.dative;
                } else if (tryEat(s, "{{Akk.}}")) {
                    base = &p.accusative;
                } else if (tryEat(s, "{{Pl.}}")) {
                    base = &p.nominative;
                    plural = true;
                    if (pluralAp)
                        cout << "Error: Multiple Plural declarations -  "
                             << worttrennung << endl;
                    pluralAp = true;
                } else if (tryEat(s, "{{va.|:}}") ||
                           tryEat(s, R"(''veraltet:'')")) { // Veraltet
                    // just use it
                } else if (tryEat(s, "small&gt")) { // Commented line. Don't try
                                                    // to parse this!
                    break;
                } else if (!first) {
                    cout << "Error: unknown case - \"" << s << "\"  "
                         << worttrennung << endl;
                    break;
                }
                first = false;

                vector<string> *writeTo;
                if (!plural)
                    writeTo = &base->first.singular;
                else
                    writeTo = &base->first.plural;

                writeTo->push_back(saniWiki(s));
                some = true;
            }
        }

        if (some)
            dict.add(p);
        else
            cout << "Error: " << title << " was empty" << endl;

    } else if (has(mType, "rtikel") || has(mType, "Adverb") ||
               has(mType, "adverb") ||
               hasAnyOf(mType,
                        {"Subjunktion", "Onomatopoetikum", "Postposition",
                         "Adverb", "Interjektion", "Präposition", "Kontraktion",
                         "Grußformel", "Konjunktion"})) {

        bool some = false;
        Adverb a;

        const static thread_local vector<pair<string, AdverbType>> lookup = {
            {"Partikel", AdverbType::particle},
            {"Antwortpartikel", AdverbType::particle},
            {"Gradpartikel", AdverbType::particle},
            {"Modalpartikel", AdverbType::particle},
            {"Negationspartikel", AdverbType::particle},
            {"Fokuspartikel", AdverbType::particle},
            {"Vergleichspartikel", AdverbType::particle},

            {"Artikel", AdverbType::article},

            {"Subjunktion", AdverbType::subjunction},
            {"Konjunktion", AdverbType::conjunction},

            {"Grußformel", AdverbType::other},
            {"Kontraktion", AdverbType::other},
            {"Interjektion", AdverbType::other},
            {"Postposition", AdverbType::other},
            {"Onomatopoetikum", AdverbType::other},

            {"Präposition", AdverbType::preposition},

            {"Adverb", AdverbType::adverb},
            {"Temporaladverb", AdverbType::adverb},
            {"Pronominaladverb", AdverbType::adverb},
            {"Lokaladverb", AdverbType::adverb},
            {"Modaladverb", AdverbType::adverb},
            {"Konjunktionaladverb", AdverbType::adverb},
            {"Relativadverb", AdverbType::adverb},
            {"Interrogativadverb", AdverbType::adverb},
        };
        a.type = AdverbType::unknown;
        for (auto l : lookup) {
            if (type == l.first || has(mType, l.first)) {
                a.type = l.second;
                break;
            }
        }
        if (a.type == AdverbType::unknown) {
            cout << "Error: unknown adverb " << type << " " << mType << endl;
        }

        for (const auto c_prop : leading<1>(body, vertSep)) {
            some = true;
            string prop(get<1>(c_prop));
            if (tryEat(prop, "Positiv"))
                collectStarsNumbers<5>(prop, a.positive);
            else if (tryEat(prop, "Komparativ"))
                collectStarsNumbers<5>(prop, a.comparative);
            else if (tryEat(prop, "Superlativ"))
                collectStarsNumbers<5>(prop, a.superlative);
            else if (tryEat(prop, "Bild") || tryEat(prop, "Flexion=")) {
                // drop
            } else {
                cout << "Error: unknown prop adj: " << prop << endl;
            }
        }

        if (!some)
            a.positive.push_back(saniWiki(title));
        dict.add(a);

    } else if (has(type, "Substantiv") || has(mType, "Substantiv") ||
               has(mType, "name") || has(type, "name") || has(type, "Name") ||
               has(type, "Toponym") || has(mType, "Toponym") ||
               has(mType, "Numeral")) {

        buildNoun(type, mType, body, title, dict, worttrennung);

    } else if (has(mType, "Erweiterter Infinitiv")) {
        // TODO:Add to existing entries!
    } else if (has(type, "adjektivisch")) {
        // TODO: Drop?
    } else if (hasAnyOf(mType, {"Abkürzung",
                                "Deklinierte Form",
                                "Konjugierte Form",
                                "Wortverbindung",
                                "Zahlzeichen",
                                "Suffix",
                                "Präfix",
                                "Affix",
                                "Gebundenes Lexem",
                                "gebundenes Lexem",
                                "Dekliniertes Gerundivum",
                                "Redewendung",
                                "Suffixoid",
                                "Präfixoid",
                                "Partizip II",
                                "Partizip I",
                                "Sprichwort",
                                "Zirkumposition"
                                "Ortsnamengrundwort",
                                "Komparativ",
                                "Superlativ",
                                "Merkspruch"})) {
        // TODO: Drop?
    } else {
        cout << "Error: unknown class: (" << type << ") (" << mType << ") "
             << title << endl;
    }
}

void parseGrammar(const string &subsub, const bool print, const string &title,
                  Dictionary &dict, bool isName, const string &mType) {

    if (isName)
        parseWord(dict, title, "Name", "", "", print, mType);
    else {
        string grammarField, worttrennung, typeThere;
        for (auto field : leading<1>(subsub)) {

            const static thread_local vector<string> types = {
                "Substantiv Übersicht",   "Substantiv Dialekt",
                "Vorname Übersicht",      "Nachname Übersicht",
                "Name Übersicht",         "Eigenname Übersicht",
                "Adjektiv Übersicht",     "Verb Übersicht",
                "Adverb Übersicht",       "Possessivpronomen",
                "Demonstrativpronomen",   "Personalpronomen",
                "Toponym Übersicht",      "Pronomen Übersicht",
                "adjektivisch Übersicht", "Eigenname",
                "Pronomina-Tabelle"};

            if (grammarField.empty()) {
                bool found = false;
                for (const string type : types) {
                    if (has(get<0>(field), "sch " + type)) {
                        found = true;
                        grammarField = get<0>(field);
                        typeThere = type;
                        break;
                    }
                }
                if (found)
                    continue;
            }

            if (get<0>(field) == "Worttrennung") {
                worttrennung = get<1>(field);
                continue;
            }

            if (startsWith(get<0>(field), "Deutsch ") && grammarField.empty()) {

                cout << "Error: unknown field: " << get<0>(field) << endl;

            } else {
                /* if (print)
                     cout << "   "
                          << "   "
                          << "   " << get<0>(field) << endl;*/
            }
        }
        parseWord(dict, title, typeThere, grammarField, worttrennung, print,
                  mType);
    }
}

void parseSubsections(const string &title, const bool print, Dictionary &dict,
                      const string &sTitle, const string &sBody,
                      map<string, size_t> &wordClasses) {
    // Iterate the section title
    const string mTitle = trim(title);
    const vector<string> titleP = parseSectionTitle(sTitle);
    if (print) {
        cout << mTitle << " ";
        for (const string descr : titleP) {
            cout << descr << " ";
        }
        cout << endl;
    }

    // Split into subsections
    const static thread_local std::regex reSubSec(
        R"(=== \{\{(([^\}]|\}\}, \{\{)*)\}\} ===\n)");
    for (auto subsection : leading<1>(sBody, reSubSec)) {

        // Iterate the subsection title // TODO: Extract ===
        // {{Wortart|Reziprokpronomen|Deutsch}} ===
        vector<string> sTitle = parseSectionTitle(get<0>(subsection));
        if (print) {
            cout << "   ";
            for (const string descr : sTitle) {
                cout << descr << " ";
            }
            cout << endl;
        }

        // Check if we are interested in such a word
        const static thread_local std::regex vert(
            R"(\|)");
        bool skipSubsection = true;
        bool isName = false;
        string mType;
        for (const string fie : sTitle) {
            auto descrs = leading<0>(fie, vert, true);

            if (descrs.size() == 3) {
                const string d2 = trim(get<1>(descrs[1]));
                mType += " " + d2;
                if (get<1>(descrs[0]) == "Wortart") {

                    auto [it, ok] = wordClasses.emplace(d2, 1);
                    if (!ok)
                        it->second++;

                    if (get<1>(descrs[2]) == "Deutsch") {
                        skipSubsection = false;
                    } else if (d2 == "Vorname" || d2 == "Nachname" ||
                               d2 == "Name") {
                        skipSubsection = false;
                        isName = true;
                    }
                }
            }
        }
        if (skipSubsection)
            continue;

        // Split into subsubsections
        const static thread_local std::regex reSubSubSec(
            R"(==== \{\{(([^\}]|\}\}, \{\{)*)\}\} ====\n)");
        for (auto subsubsection :
             leading<1>(get<1>(subsection), reSubSubSec, true)) {

            // Iterate the subsubsection title
            vector<string> ssTitle = parseSectionTitle(get<0>(subsubsection));
            /*if (print) {
                cout << "      ";
                for (const string descr : ssTitle) {
                    cout << descr << " ";
                }
                cout << endl;
            }*/

            if (only(ssTitle, "")) {
                parseGrammar(get<1>(subsubsection), print, mTitle, dict, isName,
                             mType);
            }
        }
    }
}

Dictionary parseWiki(shared_ptr<istream> &&in) {
    const bool print = false;

    Dictionary dict;
    bool keepParsing = true;
    map<string, size_t> wordClasses;

    while (keepParsing) {
        const string page = readTag(in, "page");
        if (page == "\0")
            break;

        // Split into sections separated by == Title {{Sprache:lang}} ==
        const static thread_local std::regex innerText(
            R"(<text( [^>]*)?>([^<]*)</text>)");
        const static thread_local std::regex reSec(
            R"(== [^\(]+ \(\{\{Sprache\|([^\}]+)\}\}\) ==\n)");
        for (auto section : leading<1>(match<2>(page, innerText), reSec)) {
            const string title = readTag(page, "title");
            if (get<0>(section) == "Deutsch") {
                parseSubsections(title, print, dict, get<0>(section),
                                 get<1>(section), wordClasses);

            } else {
                parseSubsections(title, print, dict, get<0>(section),
                                 get<1>(section), wordClasses);
            }
        }
        if (dict.size() > 2000) {
            //  keepParsing = false;
        }
    }

    if (print) {
        cout << endl << wordClasses.size() << " word classes" << endl;
        for (auto wc : wordClasses)
            cout << wc.second << " " << wc.first << endl;
    }

    return dict;
}

} // namespace parseWiki