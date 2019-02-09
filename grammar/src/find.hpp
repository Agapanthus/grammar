#include <nuspell/dictionary.hxx>
#include <nuspell/locale_utils.hxx>
#include <nuspell/string_utils.hxx>
#include <nuspell/structures.hxx>

#include <boost/locale.hpp>

#include <exception>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
using std::vector, std::string, std::wstring;

using nuspell::Dictionary, nuspell::Hash_Multiset, nuspell::utf8_to_wide,
    nuspell::to_narrow, nuspell::Casing;
using nuspell::v2::List_Strings, nuspell::split_on_whitespace_v;
using std::cout, std::cin, std::cerr, std::endl, std::clog, std::exception;
using std::ostream, std::istream, std::locale, std::streamoff, std::begin,
    std::end, std::use_facet;

auto external_to_internal_encoding2(const string &in, wstring &wide_out,
                                    string &narrow_out) -> bool {

    using ed = nuspell::Encoding_Details;
    auto ok = true;

    utf8_to_wide(in, wide_out);
    /*
switch (enc_details) {
case ed::EXTERNAL_U8_INTERNAL_U8:
    ok = utf8_to_wide(in, wide_out);
    break;
case ed::EXTERNAL_OTHER_INTERNAL_U8:
    ok = to_wide(in, external_locale, wide_out);
    break;
case ed::EXTERNAL_U8_INTERNAL_OTHER:
    ok = utf8_to_wide(in, wide_out);
    ok &= to_narrow(wide_out, narrow_out, internal_locale);
    break;
case ed::EXTERNAL_OTHER_INTERNAL_OTHER:
    ok = to_wide(in, external_locale, wide_out);
    ok &= to_narrow(wide_out, narrow_out, internal_locale);
    break;
case ed::EXTERNAL_SAME_INTERNAL_AND_SINGLEBYTE:
    narrow_out = in;
    ok = true;
    break;
}*/
    return ok;
}

template <typename CHAR_T> class My_Dictionary : public Dictionary {
    Hash_Multiset<string> personal;

  public:
    auto &operator=(const Dictionary &d) {
        static_cast<Dictionary &>(*this) = d;
        return *this;
    }
    auto &operator=(Dictionary &&d) {
        static_cast<Dictionary &>(*this) = std::move(d);
        return *this;
    }
    auto spell(const string &word) {
        auto correct = Dictionary::spell(word);
        if (correct)
            return true;
        auto r = personal.equal_range(word);
        correct = r.first != r.second;
        return correct;
    }

    string my_spell_sharps(std::basic_string<CHAR_T> &base, size_t pos = 0,
                           size_t n = 0, size_t rep = 0) const {
        const size_t MAX_SHARPS = 5;
        pos = base.find(LITERAL(CHAR_T, "ss"), pos);
        if (pos != std::string::npos && n < MAX_SHARPS) {
            base[pos] = static_cast<CHAR_T>(223); // ß
            base.erase(pos + 1, 1);
            string res = my_spell_sharps(base, pos + 1, n + 1, rep + 1);
            base[pos] = 's';
            base.insert(pos + 1, 1, 's');
            if (!res.empty())
                return res;
            res = my_spell_sharps(base, pos + 2, n + 1, rep);
            if (!res.empty())
                return res;
        } else if (rep > 0) {
            return my_check_word(base);
        }
        return "";
    }

    string my_check_word(std::basic_string<CHAR_T> &s) const {
        for (auto &we : boost::make_iterator_range(words.equal_range(s))) {
            auto &word_flags = we.second;
            if (word_flags.contains(need_affix_flag))
                continue;
            if (word_flags.contains(compound_onlyin_flag))
                continue;
            return we.first;
        }
        {
            auto ret3 = strip_suffix_only(s);
            if (ret3)
                return ret3->first;
        }
        {
            auto ret2 = strip_prefix_only(s);
            if (ret2)
                return ret2->first;
        }
        {
            auto ret4 = strip_prefix_then_suffix_commutative(s);
            if (ret4)
                return ret4->first;
        }
        if (!complex_prefixes) {
            auto ret6 = strip_suffix_then_suffix(s);
            if (ret6)
                return ret6->first;

            auto ret7 = strip_prefix_then_2_suffixes(s);
            if (ret7)
                return ret7->first;

            auto ret8 = strip_suffix_prefix_suffix(s);
            if (ret8)
                return ret8->first;

            // this is slow and unused so comment
            // auto ret9 = strip_2_suffixes_then_prefix(s);
            // if (ret9)
            //	return &ret9->second;
        } else {
            auto ret6 = strip_prefix_then_prefix(s);
            if (ret6)
                return ret6->first;
            auto ret7 = strip_suffix_then_2_prefixes(s);
            if (ret7)
                return ret7->first;

            auto ret8 = strip_prefix_suffix_prefix(s);
            if (ret8)
                return ret8->first;

            // this is slow and unused so comment
            // auto ret9 = strip_2_prefixes_then_suffix(s);
            // if (ret9)
            //	return &ret9->second;
        }
        auto ret10 = check_compound(s);
        if (ret10)
            return ret10->first;

        return "";
    }

    string findWordUppercase(std::basic_string<CHAR_T> &s) {
        auto &loc = internal_locale;

        auto res = my_check_word(s);
        if (!res.empty())
            return res;

        // handle prefixes separated by apostrophe for Catalan, French and
        // Italian, e.g. SANT'ELIA -> Sant'+Elia
        auto apos = s.find('\'');
        if (apos != s.npos && apos != s.size() - 1) {
            // apostrophe is at beginning of word or dividing the word
            auto part1 = s.substr(0, apos + 1);
            auto part2 = s.substr(apos + 1);
            part1 = boost::locale::to_lower(part1, loc);
            part2 = boost::locale::to_title(part2, loc);
            auto t = part1 + part2;
            res = my_check_word(t);
            if (!res.empty())
                return res;
            part1 = boost::locale::to_title(part1, loc);
            t = part1 + part2;
            res = my_check_word(t);
            if (!res.empty())
                return res;
        }

        // handle sharp s for German
        if (checksharps && s.find(LITERAL(CHAR_T, "SS")) != s.npos) {
            auto t = boost::locale::to_lower(s, loc);
            res = my_spell_sharps(t);
            if (!res.empty())
                t = boost::locale::to_title(s, loc);
            res = my_spell_sharps(t);
            if (!res.empty())
                return res;
        }
        auto t = boost::locale::to_title(s, loc);
        res = my_check_word(t);
        if (!res.empty()) // TODO && !res->contains(keepcase_flag))
            return res;

        t = boost::locale::to_lower(s, loc);
        res = my_check_word(t);
        if (!res.empty()) // TODO && !res->contains(keepcase_flag))
            return res;

        return "";
        // return nullptr;
    }

    string getBaseName(const string &word) {
        auto static thread_local wide_word = std::wstring();
        auto static thread_local narrow_word = string();
        auto ok_enc =
            external_to_internal_encoding2(word, wide_word, narrow_word);

        const nuspell::Casing casing_type =
            nuspell::classify_casing(wide_word, internal_locale);

        switch (casing_type) {
        case Casing::SMALL:
        case Casing::CAMEL:
        case Casing::PASCAL:
            return "c?";
            // res = check_word(s);
            break;
        case Casing::ALL_CAPITAL:
            return findWordUppercase(wide_word);
            break;
        case Casing::INIT_CAPITAL:
            return "k?";
            // res = spell_casing_title(s);
            break;
        }

        /*for(auto e :  res[0].second) {
                cout << e << endl;
        }*/

        return "";
    }
    auto parse_personal_dict(istream &in, const std::locale &external_locale) {
        auto word = string();
        auto wide_word = std::wstring();
        while (getline(in, word)) {
            auto ok = utf8_to_wide(word, wide_word);
            ok &= to_narrow(wide_word, word, external_locale);
            if (!ok)
                continue;
            personal.insert(word);
        }
        return in.eof();
    }
    auto parse_personal_dict(std::string name,
                             const std::locale &external_locale) {
#ifdef _WIN32
        auto const PATH_SEPS = "\\/";
#else
        auto const PATH_SEPS = '/';
#endif
        auto file = std::ifstream();
        auto path_sep_idx = name.find_last_of(PATH_SEPS);
        if (path_sep_idx != name.npos)
            name.erase(0, path_sep_idx + 1);
        name.insert(0, ".nuspell_");
        auto home = getenv("HOME");
        if (home) {
            name.insert(0, "/");
            name.insert(0, home);
        }
        file.open(name);
        if (file.is_open())
            return parse_personal_dict(file, external_locale);
        return true;
    }
};

namespace std {
ostream &operator<<(ostream &out, const locale &loc) {
    if (has_facet<boost::locale::info>(loc)) {
        auto &f = use_facet<boost::locale::info>(loc);
        out << "name=" << f.name() << ", lang=" << f.language()
            << ", country=" << f.country() << ", enc=" << f.encoding();
    } else {
        out << loc.name();
    }
    return out;
}
} // namespace std

class Finder {
  private:
    My_Dictionary<wchar_t> dic;

  public:
    Finder() {
        // May speed up I/O. After this, don't use C printf, scanf etc.
        std::ios_base::sync_with_stdio(false);

        boost::locale::generator gen;
        auto loc = std::locale();
        try {
            loc = gen(""); // loc = gen("en_US." + args.encoding);
            nuspell::install_ctype_facets_inplace(loc);
        } catch (const boost::locale::conv::invalid_charset_error &e) {
            cerr << e.what() << '\n';
            throw exception("TODO");
        }
        cin.imbue(loc);
        cout.imbue(loc);

        clog << "INFO: I/O  locale " << loc << '\n';

        /*
        auto f = Finder::search_all_dirs_for_dicts();
            if (args.mode == LIST_DICTIONARIES_MODE) {
                list_dictionaries(f);
        }*/

        string dictionary;
        {
            // infer dictionary from locale
            auto &info = use_facet<boost::locale::info>(loc);
            dictionary = info.language();
            auto c = info.country();
            if (!c.empty()) {
                dictionary += '_';
                dictionary += c;
            }
        }

        string filename =
            "C:\\Alkan\\grammar\\dict\\Dictionaries-"
            "master\\German"; // TODO:
                              // f.get_dictionary_path(dictionary);
        if (filename.empty()) {
            cerr << "Dictionary " << dictionary << " not found\n";
            throw exception("Dictionary not found");
        }
        clog << "INFO: Pointed dictionary " << filename << ".{dic,aff}\n";

        try {
            dic = Dictionary::load_from_path(filename);
            dic.parse_personal_dict(dictionary, loc);

        } catch (const std::ios_base::failure &e) {
            cerr << e.what() << '\n';
            throw exception("todo");
        }
        dic.imbue(loc);
    }

    string getBaseName(const string &word) {

        bool correct = dic.spell(word);

        if (correct) {
            string base = dic.getBaseName(word);
            if (base.empty())
                return "";
            else
                return base;
        } else {
            return "";
        }
        /*else {
        auto suggestions = List_Strings();
        dic.suggest(word, suggestions);

    if (suggestions.empty()) {
        cout << "# " << word << endl; // ' ' << pos << '\n';

    } else {
        cout << "& " << word << ' '
             << suggestions.size() //<< ' ' << pos
             << ": ";
        cout << suggestions[0];
        for_each(begin(suggestions) + 1, end(suggestions),
                 [&](auto &sug) { cout << ", " << sug; });
    }
}*/
    }
};
string findWord(string in) {

    /*
if (false) {
            cout << "Bitte Wörter eingeben:" << endl;
    loop_function(cin, cout, dic);
} else {
    for (auto &file_name : args.files) {
        ifstream in(file_name.c_str());
        if (!in.is_open()) {
            cerr << "Can't open " << file_name << '\n';
            return 1;
        }
        in.imbue(loc);
        loop_function(in, cout, dic);
    }
}
*/
    return in;
}