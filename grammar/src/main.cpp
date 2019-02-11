#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using std::cout, std::endl, std::wcout;
using std::make_shared, std::shared_ptr;
using std::string, std::vector;

//#include "find.hpp"
#include "dictionary.hpp"
#include "filters.hpp"
#include "grammarIO.hpp"
#include "kompositum.hpp"
#include "parseWiki.hpp"
#include "punctuation.hpp"
#include "util.hpp"

typedef int Finder;

// Features: Gegenteil bilden, Verallgemeinern, Präzisieren, Synonym,
// Verniedlichen, Verförmlichen, In-Dialekt, Satzbau vereinfachen

// TODO: Konjugierte / deklinierte Formen parsen und am Ende dann das gesamte
// Dictionary "zusammenfügen" !

// TODO: Komposita (mit Nuspell?) vor der Grammatikanalyse auftrennen

// TODO: Wichtig: Auch die Seiten "Flexion" und "Konjugierte/Deklinierte Form"
// parsen und danach das Dictionary zusammenfassen und vereinfachen!

void recursivePrint(Finder &fin, Dictionary &dict,
                    shared_ptr<punctuation::Node> sen) {
    for (const auto &c : sen->getChildren()) {
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

double asPercent(double in) { return (int(in * 1000) / 10.0); }

double testNouns(Cases c, bool plural, const Dictionary &d2,
                 size_t howMany = 10000, bool print = true) {

    size_t count = 0;
    size_t wrong = 0;

    for (const Noun &noun : d2.nouns) {
        if (noun.predefined(c, plural) &&
            noun.predefined(Cases::Nominative, false)) {
            const string res = noun.get(c, plural, d2, true);
            if (!noun.test(c, plural, d2, res)) {
                wrong++;
                if (print) {
                    wcout << wrong << L") " << widen(noun.ns()) << L" "
                          << widen(noun.get(c, plural, d2)) << L" "
                          << widen(res) << endl;

                    wcout.clear(); // Dirty hack. When outputting something like
                                   // ō to wcout, it will go bad. Using this it
                                   // will recover.
                }
            }
            if (++count >= howMany) {
                break;
            }
        }
    }

    if (count == 0) {
        count++;
        cout << "Warning: No tests have been run!" << endl;
    }

    if (print)
        cout << endl
             << asPercent(wrong / double(count)) << "% errors (" << wrong
             << " of " << count << ")" << endl;
    return wrong / double(count);
}

void nounsMatrixTest(const Dictionary &d2, size_t howMany = 10000) {
    double values[8];
    for (size_t i = 0; i < 4; i++) {
        values[i * 2] = testNouns(Cases(i), false, d2, howMany, false);
        values[i * 2 + 1] = testNouns(Cases(i), true, d2, howMany, false);
    }
    cout << "\tNOM\tGEN\tDAT\tACC\n"
         << "Sg\t" << asPercent(values[0]) << "%\t" << asPercent(values[2])
         << "%\t" << asPercent(values[4]) << "%\t" << asPercent(values[6])
         << "%\n"
         << "Pl\t" << asPercent(values[1]) << "%\t" << asPercent(values[3])
         << "%\t" << asPercent(values[5]) << "%\t" << asPercent(values[7])
         << "%" << endl;
}

// Prints plurals for words with no predefined plural
void testNPNeg(const Dictionary &d2, size_t howMany = 100) {
    size_t count = 0;
    for (Noun noun : d2.nouns) {
        if (noun.nominative.plural.empty()) {
            wcout << count << L") " << widen(noun.ns()) << L" "
                  << widen(noun.np(d2, true)) << endl;
            wcout.clear();
            if (++count >= howMany)
                break;
        }
    }
}

double testVerbs(Verbspec vs, const Dictionary &d2, size_t howMany = 10000,
                 bool print = true) {

    size_t count = 0;
    size_t wrong = 0;

    for (Verb verb : d2.verbs) {
        if (verb.predefined(Verbspec(Verbspec::Present, Verbspec::Active,
                                     Verbspec::AnyPerson,
                                     Verbspec::Singular))) {
            const string res = verb.get(vs, d2, true);
            if (!verb.test(vs, d2, res)) {
                wrong++;
                if (print) {
                    wcout << wrong << L") " << widen(verb.indPres1Sg()) << L" "
                          << widen(verb.get(vs, d2)) << L" " << widen(res)
                          << endl;

                    wcout.clear(); // Dirty hack. When outputting something like
                                   // ō to wcout, it will go bad. Using this it
                                   // will recover.
                }
            }
            if (++count >= howMany) {
                break;
            }
        }
    }

    if (count == 0) {
        count++;
        cout << "Warning: No tests have been run!" << endl;
    }

    if (print)
        cout << endl
             << asPercent(wrong / double(count)) << "% errors (" << wrong
             << " of " << count << ")" << endl;
    return wrong / double(count);
}

void verbsMatrixTest(const Dictionary &d2, size_t howMany = 10000) {
    double values[10];

    Verbspec::Person persons[5] = {Verbspec::First, Verbspec::Second,
                                   Verbspec::Third, Verbspec::Polite,
                                   Verbspec::Majestic};
    for (size_t i = 0; i < 5; i++) {
        values[i * 2] = testVerbs(Verbspec(Verbspec::Present, Verbspec::Active,
                                           persons[i], Verbspec::Singular),
                                  d2, howMany, false);
        values[i * 2 + 1] =
            testVerbs(Verbspec(Verbspec::Present, Verbspec::Active, persons[i],
                               Verbspec::Plural),
                      d2, howMany, false);
    }
    cout << "\t1s\t2nd\t3rd\tpolite\tmajestic\n"
         << "Sg\t" << asPercent(values[0]) << "%\t" << asPercent(values[2])
         << "%\t" << asPercent(values[4]) << "%\t" << asPercent(values[6])
         << "%\t" << asPercent(values[8]) << "%\n"
         << "Pl\t" << asPercent(values[1]) << "%\t" << asPercent(values[3])
         << "%\t" << asPercent(values[5]) << "%\t" << asPercent(values[7])
         << "%\t" << asPercent(values[9]) << "%" << endl;
}

// Prints decomposition of nouns
void testBreakKompositum(const Dictionary &d2, size_t howMany = 100) {
    size_t count = 0;
    for (const Noun &noun : d2.nouns) {
        if (noun.ns().size() > 10) {
            const vector<string> dec = breakKompositum(noun.ns(), d2);
            wcout << count << L") ";
            for (const string d : dec) {
                wcout << widen(d) << L" ";
                wcout.clear();
            }
            if (++count >= howMany)
                break;
            cout << endl;
        }
    }
}

double testNDeclination(const Dictionary &d2, size_t howMany = 10000,
                        bool print = true) {
    size_t count = 0;
    size_t wrong = 0;

    for (Noun noun : d2.nouns) {
        if (noun.predefined(Cases::Nominative, false) &&
            noun.predefined(Cases::Nominative, true)) {
            // This isn't to precise...
            if (noun.isNDeclination(false) != noun.isNDeclination(true)) {
                wrong++;
                if (print) {
                    wcout << wrong << L") " << noun.isNDeclination(true)
                          << L"   " << widen(noun.ns()) << L" "
                          << widen(noun.np(d2)) << endl;
                    wcout.clear();
                }
            }
            if (++count >= howMany) {
                if (print)
                    cout << endl
                         << asPercent(wrong / double(howMany)) << "% errors ("
                         << wrong << " of " << count << ")" << endl;
                return wrong / double(howMany);
            }
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    // TODO: Worte spalten! auxiliary ("haben") verb mit Originalwort verlinken
    // (als auxiliary) und z.B. "frage" mit "nach"
    // TODO: nuspell verwenden, um Zusammengesetzte Wörter zu zerkleinern!

    // TODO: Komposita spalten, wenn andere Kasi berechnet werden!

    try {

        ///////////////////////// Re-Create DB
#if 1
        cout << "Analyze..." << endl;
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
        cout << "Simplify..." << endl;
        dict.simplify();
        cout << "Serialize..." << endl;
        dict.serialize(fo);
        fo.close();
        cout << "Done." << endl;
        return 1;
#endif

        ///////////////////////// Load DB

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

        ///////////////////////////// Test Nouns
#if 0
        wcout << widen(d2.find("Weihnachtsbaum")[0].noun->np(d2, true)) << endl
              << endl;
        // testNPNeg(d2);
        // testBreakKompositum(d2);

        nounsMatrixTest(d2, 30000);
        cout << "N-Decl.: " << asPercent(testNDeclination(d2, 30000, false))
             << "%" << endl;
#endif

        ///////////////////////////// Test Verbs
#if 1
        verbsMatrixTest(d2, 10000);
#endif

        ///////////////////////////// Match against Alice
#if 0 
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
#endif

        cout << endl << "done" << endl;
        return 0;

    } catch (Exception &e) {
        std::cerr << "Error EXCEPTION: " << e.what() << endl;
    } catch (std::exception &e) {
        std::cerr << "Error: std::exception: " << e.what() << endl;
    } catch (...) {
        cout << endl << "Error: unknown exception" << endl;
    }
    return -1;
}
