#include "parseWiki.hpp"

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
#include "pronoun.hpp"
#include "util.hpp"

using namespace parseWiki;

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

vector<tuple<string, string>> parseInnerPar(const string &text) {
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
                    if (text[i] == '\n' &&
                        (text[i + 1] == ':' || text[i + 1] == '\n' ||
                         text[i + 1] == '*')) {
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

template <size_t which>
vector<tuple<string, string>> tailing(const string &text,
                                      const std::regex &re) {
    vector<tuple<string, string>> results;
    std::sregex_iterator next(text.begin(), text.end(), re,
                              std::regex_constants::match_any |
                                  std::regex_constants::match_not_null);
    std::sregex_iterator end;

    auto prev = text.begin();

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

            auto s = match[0].first;
            content = string(prev, s);
            prev = match[0].second;

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
        cout << "Error: " << title << " was empty (Noun)" << endl;
}

void buildPronoun(const string &type, const string &mType, const string &body,
                  const string &title, Dictionary &dict,
                  const string &worttrennung, const string &genderSuffix) {
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
        {"Artikel", pronounType::Article},
    };
    p.type = pronounType::unknown;
    for (const auto &l : lookup) {
        if (type == l.first || has(mType, l.first)) {
            p.type = l.second;
            break;
        }
    }
    if (p.type == pronounType::unknown) {
        cout << "Error: unknown pronoun " << type << endl;
    }

    bool some = false;
    bool genderM = false, genderF = false, genderN = false;
    for (const auto c_prop : leading<1>(body, vertSep)) {
        some = true;
        string prop(get<1>(c_prop));

        Numeri *theCase = nullptr;

        if (tryEat(prop, "Nominativ "))
            theCase = &p.nominative;
        else if (tryEat(prop, "Genitiv "))
            theCase = &p.genitive;
        else if (tryEat(prop, "Dativ "))
            theCase = &p.dative;
        else if (tryEat(prop, "Akkusativ "))
            theCase = &p.accusative;

        if (theCase) {
            string tryGender = "";
            if (tryEat(prop, "Singular ")) {
                if (tryEat(prop, genderSuffix + "="))
                    theCase->singular.push_back(prop);
                else
                    tryGender = prop;
            } else if (tryEat(prop, "Plural ")) {
                if (tryEat(prop, genderSuffix + "="))
                    theCase->plural.push_back(prop);
                else
                    tryGender = prop;
            }

            if (!tryGender.empty() && genderSuffix.empty()) {
                if (!genderM && tryEat(tryGender, "m=")) {
                    genderM = true;
                    buildPronoun(type, mType, body, title, dict, worttrennung,
                                 "m");
                }
                if (!genderF && tryEat(tryGender, "f=")) {
                    genderF = true;
                    buildPronoun(type, mType, body, title, dict, worttrennung,
                                 "f");
                }
                if (!genderN && tryEat(tryGender, "n=")) {
                    genderN = true;
                    buildPronoun(type, mType, body, title, dict, worttrennung,
                                 "n");
                }
            }
        } else if (tryEat(prop, "Bild")) {
            // drop
        } else {
            cout << "Error: unknown prop: " << prop << endl;
        }
    }

    if (genderM || genderF || genderN)
        return; // Don't add the plain one if there are gendered versions

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
        Numeri *base = &p.nominative;
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
                writeTo = &base->singular;
            else
                writeTo = &base->plural;

            writeTo->push_back(saniWiki(s));
            some = true;
        }
    }

    if (p.nominative.singular.empty()) {
        p.nominative.singular.push_back(title);
        some = true;
    }

    if (some)
        dict.add(p);
    else
        cout << "Error: " << title << " was empty (pronoun)" << endl;
}

vector<vector<string> *> eatToCases(string &s, WithCases &c, bool printErr) {
    if (tryEat(s, "Nominativ Singular"))
        return {&c.nominative.singular};
    else if (tryEat(s, "Nominativ Plural"))
        return {&c.nominative.plural};
    else if (tryEat(s, "Genitiv Singular"))
        return {&c.genitive.singular};
    else if (tryEat(s, "Genitiv Plural"))
        return {&c.genitive.plural};
    else if (tryEat(s, "Dativ Singular") ||
             tryEat(s, "{{Dativ-e}} Dativ Singular"))
        return {&c.dative.singular};
    else if (tryEat(s, "Dativ Plural"))
        return {&c.dative.plural};
    else if (tryEat(s, "Akkusativ Singular") || tryEat(s, "Akusativ Singular"))
        return {&c.accusative.singular};
    else if (tryEat(s, "Akkusativ Plural") || tryEat(s, "Akusativ Plural"))
        return {&c.accusative.plural};
    else if (tryEat(s, "Alle Kasus Singular") || tryEat(s, "Singular"))
        return {&c.nominative.singular, &c.genitive.singular,
                &c.dative.singular, &c.accusative.singular};
    else if (tryEat(s, "Alle Kasus Plural") || tryEat(s, "Plural"))
        return {&c.nominative.plural, &c.genitive.plural, &c.dative.plural,
                &c.accusative.plural};
    else if (tryEat(s, "Prädikative und adverbielle Form")) {
        return {}; // This is just what is
    } else {
        if (printErr)
            cout << "Error: unknown Noun/Adjective Declination: " << s << endl;
        return {};
    }
}

void buildDeklinierteForm(Dictionary &dict, string &body,
                          const string &targetWord, const string &value,
                          pronounType p, bool printErr) {

    Pronoun n;
    n.type = p;
    n.nominative.singular.push_back(saniWiki(targetWord));

    vector<vector<string> *> target = eatToCases(body, n, printErr);
    if (target.empty())
        return;
    const string sWord = saniWiki(value);
    for (const auto &t : target)
        t->push_back(sWord);
    dict.add(n);
}

void buildDeclAdjective(Adjective &a, size_t stAndGenus, const string &word,
                        string s, bool printErr) {

    if (tryEatB(s, " des Superlativs")) {
        stAndGenus += 6;
    } else if (tryEatB(s, " des Komparativs")) {
        stAndGenus += 3;
    } else /*if (tryEatB(s, " des Positivs"))*/ {
        stAndGenus += 0;
    }

    vector<vector<string> *> target =
        eatToCases(s, a.cases[stAndGenus], printErr);
    if (target.empty())
        return;

    const string sWord = saniWiki(word);
    for (const auto &t : target)
        t->push_back(sWord);
}

void buildDeklinierteFormVerb(Dictionary &dict, string &s,
                              const string &targetWord, const string &value) {

    const string sWord = saniWiki(value);

    Verb v;

    v.base.presentInfinitive.push_back(saniWiki(targetWord));

    Verbspec spec(Verbspec::Present);

    if(tryEat(s, "Imperativ Singular")) {
        spec.mode = Verbspec::Imperative;
        spec.number = Verbspec::Singular;
    } else {

        tryEat(s, " ");
        if (tryEat(s, "1. Person")) {
            spec.person = Verbspec::First;
        } else if (tryEat(s, "2. Person")) {
            spec.person = Verbspec::Second;
        } else if (tryEat(s, "3. Person")) {
            spec.person = Verbspec::Third;
        } else {
            cout << "Error: unknown person " << s << endl;
            return;
        }
        tryEat(s, " ");
        
        if (tryEat(s, "Singular"))
            spec.number = Verbspec::Singular;
        else if (tryEat(s, "Plural"))
            spec.number = Verbspec::Plural;
        else
            cout << "Error: unexpected number " << s << endl;
        tryEat(s, " ");

        if (tryEat(s, "Indikativ")) {
            spec.mode = Verbspec::Indicative;
        } else if (tryEat(s, "Konjunktiv II")) {
            spec.mode = Verbspec::Irrealis;
        } else if (tryEat(s, "Konjunktiv I") || tryEat(s, "Konjunktiv")) {
            spec.mode = Verbspec::Subjunctive;
        }  else if (tryEat(s, "Imperativ")) {
            spec.mode = Verbspec::Imperative;
        } else {
            cout << "Error: unknown mode " << s << endl;
            return;
        }
        tryEat(s, " ");

        if (tryEat(s, "Präsens")) {
            spec.tempus = Verbspec::Present;
        } else if (tryEat(s, "Perfekt")) {
            spec.tempus = Verbspec::Perfect;
        } else if (tryEat(s, "Präteritum")) {
            spec.tempus = Verbspec::Preterite;
        } else if (tryEat(s, "Plusqaumperfekt")) {
            spec.tempus = Verbspec::Pluperfect;
        } else if (tryEat(s, "Futur I")) {
            spec.tempus = Verbspec::FutureI;
        } else if (tryEat(s, "Futur II")) {
            spec.tempus = Verbspec::FutureII;
        } else {
            cout << "Error: unknown mode " << s << endl;
            return;
        }
        tryEat(s, " ");
    }

    if (!v.set(spec, sWord)) {
        cout << "Error: Couldn't set : " << sWord << endl;
    }
    

    dict.add(v);
}

void buildDeklinierteForm(Dictionary &dict, const string &grammatischeMerkmale,
                          const string &word, bool printErr) {

    const static thread_local std::regex gM(
        R"(\[\[(\w+)\]\])");
    for (const auto &ref : tailing<1>(grammatischeMerkmale, gM)) {

        // TODO: muss "der gemischten Flexion" und "der schwachen Flexion"
        // unterschieden werden?

        string s = trim(get<1>(ref));
        tryEat(s, "'''\n"), tryEat(s, "\n"), tryEat(s, "*");
        tryEatB(s, "'''");
        s = trim(s);

        if (tryEatB(s, " des Adjektivs")) {
            Adjective a;
            a.positive.push_back(saniWiki(get<0>(ref)));

            if (has(s, " Maskulinum")) {
                buildDeclAdjective(a, 0, word, s, printErr);
            } else if (has(s, " Femininum")) {
                buildDeclAdjective(a, 1, word, s, printErr);
            } else if (has(s, " Neutrum")) {
                buildDeclAdjective(a, 2, word, s, printErr);
            } else /*if (has(s, " alle Genera")) */ {
                buildDeclAdjective(a, 0, word, s, printErr);
                buildDeclAdjective(a, 1, word, s, printErr);
                buildDeclAdjective(a, 2, word, s, printErr);
            }

            dict.add(a);

        } else if (has(s, " des Substantivs")) {
            Noun n;
            n.type = NounType::Incomplete;
            n.nominative.singular.push_back(saniWiki(get<0>(ref)));

            vector<vector<string> *> target = eatToCases(s, n, printErr);
            if (target.empty())
                return;
            const string sWord = saniWiki(word);
            for (const auto &t : target)
                t->push_back(sWord);
            dict.add(n);

        } else if (tryEatB(s, " des Personalpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Personal, printErr);
        } else if (tryEatB(s, " des Possessivpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Possessive, printErr);
        } else if (tryEatB(s, " des Reflexivpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Reflexive, printErr);
        } else if (tryEatB(s, " des Indefinitpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Indefinite, printErr);
        } else if (tryEatB(s, " des Demonstrativpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Demonstrative, printErr);
        } else if (tryEatB(s, " des Relativpronomens")) {
            buildDeklinierteForm(dict, s, get<0>(ref), word,
                                 pronounType::Relative, printErr);
        } else if (has(s, " des Verbs")) {
            buildDeklinierteFormVerb(dict, s, get<0>(ref), word);
        } else {
            if (printErr)
                cout << "Error: unknown 'deklinierte Form' " << s << endl;
        }
    }
}

void parseWord(Dictionary &dict, Dictionary &dictIncom, const string &title,
               const string &type, const string &body,
               const string &worttrennung, const string &grammatischeMerkmale,
               bool print, const string &mType) {

    if (print)
        cout << "      " << title << endl;

    if (has(mType, "Deklinierte Form") || has(mType, "Konjugierte Form")) {
        buildDeklinierteForm(dictIncom, grammatischeMerkmale, title,
                             false); // TODO: debug: printErr = false

    } else if (has(type, "Verb") || type == "verb" || has(mType, "Verb")) {
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
                collectStarsNumbers<5>(prop, v.base.irrealis_firstSingular);
            else if (tryEat(prop, "Konjunktiv I_ich"))
                collectStarsNumbers<5>(prop, v.base.subjunctive_firstSingular);
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
            cout << "Error: " << title << " was empty (verb)" << endl;

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

        if (a.positive.empty()) {
            a.positive.push_back(saniWiki(title));
            some = true;
        }

        if (some)
            dict.add(a);
        else
            cout << "Error: " << title << " was empty (adjective)" << endl;

    } else if (has(type, "ronomen") || has(mType, "ronomen") ||
               has(mType, "Artikel")) {
        buildPronoun(type, mType, body, title, dict, worttrennung, "");

    } else if (has(mType, "partikel") || has(mType, "Partikel") ||
               has(mType, "Adverb") || has(mType, "adverb") ||
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
        for (const auto &l : lookup) {
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
                cout << "Error: unknown prop adv: " << prop << endl;
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
                  Dictionary &dict, Dictionary &dictIncom, bool isName,
                  const string &mType) {

    if (isName)
        parseWord(dict, dictIncom, title, "Name", "", "", "", print, mType);
    else {
        string grammarField, worttrennung, typeThere, grammatischeMerkmale;
        for (const auto &field : parseInnerPar(subsub)) {

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
                    if (has(get<0>(field), type)) {
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
            } else if (get<0>(field) == "Grammatische Merkmale") {
                grammatischeMerkmale = get<1>(field);
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
        parseWord(dict, dictIncom, title, typeThere, grammarField, worttrennung,
                  grammatischeMerkmale, print, mType);
    }
}

void parseSubsections(const string &title, const bool print, Dictionary &dict,
                      Dictionary &dictIncom, const string &sTitle,
                      const string &sBody, map<string, size_t> &wordClasses) {
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
    for (const auto &subsection : leading<1>(sBody, reSubSec)) {

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
        for (const auto &subsubsection :
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
                parseGrammar(get<1>(subsubsection), print, mTitle, dict,
                             dictIncom, isName, mType);
            }
        }
    }
}

template <typename T, typename F>
inline void merge(vector<T> &container, const vector<T> &source, size_t &merged,
                  size_t &tryMerge, F doMerge) {
    auto it = source.begin();
    while (it != source.end()) {
        const T &test = *it;
        bool matched = false;
        tryMerge++;
        for (auto i = container.begin(); i != container.end(); i++) {
            if (i != it && doMerge(*i, test)) {
                matched = true;
                // cout << "merged " << test.nominative.singular[0] <<
                // endl;
                merged++;
                break;
            }
        }
        /* if (!matched)
             cout << "Error: Couldn't merge "
                  << test.nominative.singular[0] << endl;*/

        it++;
    }
}

static inline void pushUnique(Numeri &a, const Numeri &b) {
    pushUnique(a.singular, b.singular);
    pushUnique(a.plural, b.plural);
}
static inline void pushUnique(WithCases &a, const WithCases &b) {
    for (size_t k = 0; k < 4; k++) {
        pushUnique(a.cases[k], b.cases[k]);
    }
}
static inline void pushUnique(Person &a, const Person &b) {
    pushUnique(a.first, b.first);
    pushUnique(a.second, b.second);
    pushUnique(a.third, b.third);    
}

void mergeDict(Dictionary &dict, Dictionary &dictIncom) {
    cout << endl << "merging..." << endl;
    size_t merged = 0, tryMerge = 0;

        merge(dict.nouns, dictIncom.nouns, merged, tryMerge,
              [](Noun &i, const Noun &test) {
                  if (!(i.genus <= test.genus) || i.type != NounType::Noun ||
                      !doIntersect(i.nominative.singular,
       test.nominative.singular)) return false; pushUnique(i, test); return
       true;
              });

        merge(dict.adjectives, dictIncom.adjectives, merged, tryMerge,
              [](Adjective &i, const Adjective &test) {
                  if (!doIntersect(i.positive, test.positive))
                      return false;
                  for (size_t j = 0; j < 9; j++) {
                      pushUnique(i.cases[j],
                                     test.cases[j]);
                         

                  }
                  pushUnique(i.positive, test.positive);
                  pushUnique(i.superlative, test.superlative);
                  pushUnique(i.comparative, test.comparative);

                  return true;
              });
        merge(dict.pronouns, dictIncom.pronouns, merged, tryMerge,
              [](Pronoun &i, const Pronoun &test) {
                  if (i.type != test.type ||
                      !doIntersect(i.nominative.singular,
       test.nominative.singular)) return false; pushUnique(i, test); return
       true;
              });
              
    merge(dict.verbs, dictIncom.verbs, merged, tryMerge,
          [](Verb &i, const Verb &test) {
              if (!doIntersect(i.base.presentInfinitive,
                               test.base.presentInfinitive))
                  return false;
              pushUnique(i.imperative, test.imperative);
              pushUnique(i.base.auxiliary, test.base.auxiliary);
              pushUnique(i.base.irrealis_firstSingular,
                         test.base.irrealis_firstSingular);
              pushUnique(i.base.participleII, test.base.participleII);
              pushUnique(i.base.present, test.base.present);
              pushUnique(i.base.presentInfinitive, test.base.presentInfinitive);
              pushUnique(i.base.preterite_firstSingular,
                         test.base.preterite_firstSingular);
              pushUnique(i.base.subjunctive_firstSingular,
                         test.base.subjunctive_firstSingular);

              return true;
          });

    cout << endl << "merged " << merged << " of " << tryMerge << endl;
}

namespace parseWiki {

Dictionary parseWiki(shared_ptr<istream> &&in) {
    const bool print = false;

    Dictionary dict;
    Dictionary dictIncom;
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
        for (const auto &section :
             leading<1>(match<2>(page, innerText), reSec)) {
            const string title = readTag(page, "title");
            if (get<0>(section) == "Deutsch") {
                parseSubsections(title, print, dict, dictIncom, get<0>(section),
                                 get<1>(section), wordClasses);

            } else {
                parseSubsections(title, print, dict, dictIncom, get<0>(section),
                                 get<1>(section), wordClasses);
            }
        }
        if (dict.size() > 10000) {
           // keepParsing = false;
        }
    }

    if (print) {
        cout << endl << wordClasses.size() << " word classes" << endl;
        for (const auto &wc : wordClasses)
            cout << wc.second << " " << wc.first << endl;
    }

    mergeDict(dict, dictIncom);

    return dict;
}

} // namespace parseWiki