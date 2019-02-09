
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
using std::string, std::vector;

#include <fstream>

#include <ios>
#include <iostream>
using std::cout, std::endl, std::wcout;
using std::make_shared, std::shared_ptr;

#include "filters.hpp"
//#include "find.hpp"
#include "punctuation.hpp"
#include "util.hpp"

#include "grammar.hpp"
#include "grammarIO.hpp"
#include "parseWiki.hpp"

typedef int Finder;

// Features: Gegenteil bilden, Verallgemeinern, Präzisieren, Synonym,
// Verniedlichen, Verförmlichen, In-Dialekt, Satzbau vereinfachen

// TODO: Konjugierte / deklinierte Formen parsen und am Ende dann das gesamte
// Dictionary "zusammenfügen" !

// TODO: Komposita (mit Nuspell?) vor der Grammatikanalyse auftrennen

void recursivePrint(Finder &fin, Dictionary &dict,
                    shared_ptr<punctuation::Node> sen) {
    for (auto c : sen->getChildren()) {
        if (c->type == punctuation::NodeTypes::word) {
            string con = c->getContent();
            vector<Word> words = dict.closest(con);
            Word word = words[0];
            // cout << words << endl;
            wstring wcon = widen(con);

            switch (word.w) {
            case WordType::Adjective:
                wcout << blue << wcon;
                break;
            case WordType::Adverb:
                wcout << yellow << wcon;
                break;
            case WordType::Noun:
                wcout << green << wcon;
                break;
            case WordType::Pronoun:
                wcout << greenblue << wcon;
                break;
            case WordType::Verb:
                wcout << red << wcon;
                break;
            default:
                wcout << grey << wcon;
                break;
            }
            cout << " ";

            /*string bn = fin.getBaseName(c->getContent());
            if (bn.empty())
                cout << "? ";
            else {
                std::wcout << widen(bn) << L" ";
            }*/
        } else if (c->type == punctuation::NodeTypes::sentence) {
            recursivePrint(fin, dict, c);
        } else {
            std::wcout << black << widen(c->getContent()) << L" ";
        }
    }
}

// Prints plurals for words with predefined plural where the prediction failed
void testNomPlural(const Dictionary &d2, size_t howMany = 10000) {

    size_t count = 0;
    size_t wrong = 0;

    for (Noun noun : d2.nouns) {
        if (!noun.nominative.plural.empty() &&
            !noun.nominative.singular.empty()) {
            if (!noun.nominative.plural[0].empty()) {
                const string np = noun.np(true);
                if (!noun.np(np)) {
                    wrong++;
                    wcout << wrong << L") " << widen(noun.ns()) << L" "
                          << widen(noun.nominative.plural[0]) << L" "
                          << widen(np) << endl;
                }
                if (++count >= howMany) {
                    cout << int((wrong / double(howMany)) * 1000) / 10.0
                         << "% errors" << endl;
                    break;
                }
            }
        }
    }
}

// Prints plurals for words with predefined plural where the prediction failed
void testNomPluralD(const Dictionary &d2, size_t howMany = 10000) {

    size_t count = 0;
    size_t wrong = 0;

    for (Noun noun : d2.nouns) {
        if (!noun.nominative.plural.empty() &&
            !noun.nominative.singular.empty()) {
            if (!noun.nominative.plural[0].empty()) {
                const string np = noun.np(d2, true);
                if (!noun.np(np)) {
                    wrong++;
                    wcout << wrong << L") " << widen(noun.ns()) << L" "
                          << widen(noun.nominative.plural[0]) << L" "
                          << widen(np) << endl;
                }
                if (++count >= howMany) {
                    cout << endl
                         << int((wrong / double(howMany)) * 1000) / 10.0
                         << "% errors (" << wrong << " of " << count << ")"
                         << endl;
                    break;
                }
            }
        }
    }
}

// Prints plurals for words with no predefined plural
void testNomPlural2(const Dictionary &d2, size_t howMany = 100) {
    size_t count = 0;
    for (Noun noun : d2.nouns) {
        if (noun.nominative.plural.empty()) {
            wcout << count << L") " << widen(noun.ns()) << L" "
                  << widen(noun.np(true)) << endl;
            if (++count >= howMany)
                break;
        }
    }
}

// Prints decomposition of nouns
void testBreakKompositum(const Dictionary &d2, size_t howMany = 100) {
    size_t count = 0;
    for (Noun noun : d2.nouns) {
        if (noun.ns().size() > 10) {
            const vector<string> dec = breakKompositum(noun.ns(), d2);
            for (const string d : dec) {
                wcout << count << L") " << widen(d) << L" ";
            }
            if (++count >= howMany)
                break;
            cout << endl;
        }
    }
}

int main(int argc, char **argv) {

    // TODO: Worte spalten! auxiliary ("haben") verb mit Originalwort verlinken
    // (als auxiliary) und z.B. "frage" mit "nach"
    // TODO: nuspell verwenden, um Zusammengesetzte Wörter zu zerkleinern!

    // TODO: Lower String als Typ einführen um Fehler zu reduzieren!

    try {
        /*
        Dictionary dict = parseWiki::parseWiki(
            shared_ptr<istream>(std::make_shared<std::ifstream>(
                //"dict/test.dict")));
                "dict/dewiktionary-20190201-pages-articles-multistream.xml")));

        cout << dict << endl;
        std::ofstream fo("dict/db.txt", std::ios::binary);
        if (!fo.good()) {
            std::cerr << "BAD FILE" << endl;
            return -1;
        }
        dict.serialize(fo);
        fo.close();

        return 1;
        */

        std::ifstream fi("dict/db.txt", std::ios::binary);
        LineNumberStreambuf lb(fi);
        if (!fi.good()) {
            std::cerr << "BAD FILE" << endl;
            return -1;
        }
        Dictionary d2;
        try {
            d2.deserialize(fi);
            cout << endl << endl << d2 << endl;
        } catch (std::exception &e) {
            cout << endl << "Error: " << e.what() << endl;
            cout << " at line " << lb.lineNumber() << " " << fi.good() << endl;
        }

        d2.buildMap();

        wcout << widen(d2.find("Weihnachtsbaum")[0].noun->np(d2, true)) << endl;

        // testNomPlural(d2);
        testNomPluralD(d2, 2000);
        // testNomPlural2(d2,300);
        // testBreakKompositum(d2);

        return 1;

        iStrFil<SkipTo, string> in(
            "Hinunter in den Kaninchenbau.",
            make_shared<iStrFil<RemoveChar, char>>(
                '*',
                make_shared<iStrFil<RemoveChar, char>>(
                    '_', make_shared<std::ifstream>("corpus/de/alice2.txt"))));

        punctuation::pLexer pl(in);

        Finder fin;
        for (size_t i = 0; i < 10; i++) {
            auto sen = pl.nextSentence();
            // std::wcout << sen << endl << endl;
            recursivePrint(fin, d2, sen);
            cout << endl;
        }
        cout << endl << "done" << endl;
        return 0;

    } catch (Exception &e) {
        std::cerr << "Error: " << e.what() << endl;
    } catch (std::exception &e) {
        std::cerr << "Error: std::exception: " << e.what() << endl;
    } catch (...) {
        cout << endl << "Error: unknown exception" << endl;
    }
    return -1;
}
