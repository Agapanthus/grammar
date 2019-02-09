#pragma once

#include <string>
using std::string, std::wstring;

#include <vector>
using std::vector;

#include <cctype>

#include <algorithm>

#include <ostream>
using std::ostream, std::wostream;

#include "utf8.h"

std::wstring widen(string s) {
    try {
        std::wstring wide_out;
        utf8::utf8to16(s.begin(), s.end(), back_inserter(wide_out));
        return wide_out;
    } catch (...) {
        cout << endl << "Error widen: " << s << endl;
    }
    return L"";
}

namespace TColors {
const char *darkBlue = "\033[34m";
const char *red = "\033[31m";
const char *green = "\033[32m";
const char *yellow = "\033[33m";
const char *black = "\033[0m";
const char *greenblue = "\033[36m";
const char *purple = "\033[35m";
const char *grey = "\033[30m";
} // namespace TColors

enum Color { blue, red, green, yellow, black, greenblue, purple, grey };

const char *colorToString(const Color color) {
    switch (color) {
    case blue:
        return TColors::darkBlue;
    case red:
        return TColors::red;
    case green:
        return TColors::green;
    case yellow:
        return TColors::yellow;
    case black:
        return TColors::black;
    case greenblue:
        return TColors::greenblue;
    case purple:
        return TColors::purple;
    case grey:
        return TColors::grey;
    }
    return "";
}

ostream &operator<<(ostream &out, const Color color) {
    out << colorToString(color);
    return out;
}

wostream &operator<<(wostream &out, const Color color) {
    out << widen(colorToString(color));
    return out;
}

class Exception {
  private:
    string msg;

  public:
    string what() { return msg; }
    Exception(const string &msg) : msg(msg) {}
};

/**
 * Returns the number of bytes the given character should have
 */
inline int codePoint(const char &c) {
    if ((c & 0xf8) == 0xf0)
        return 4;
    else if ((c & 0xf0) == 0xe0)
        return 3;
    else if ((c & 0xe0) == 0xc0)
        return 2;
    return 1;
}

template <typename T, typename K> bool has(const T &t, const K &k) {
    return std::find(t.begin(), t.end(), k) != t.end();
}

template <> bool has(const string &t, const string &k) {
    return t.find(k) != string::npos;
}

bool has(const string &t, const char *k) { return has(t, string(k)); }

inline bool only(const vector<string> &vec, const string &comp) {
    if (vec.size() != 1)
        return false;
    return vec[0] == comp;
}

inline string removeSub(const string &in, const string &what) {
    string cp(in);
    size_t pos = std::string::npos;
    while ((pos = cp.find(what)) != std::string::npos)
        cp.erase(pos, what.length());
    return cp;
}

inline bool startsWith(const string &who, const string &prefix) {
    return !who.compare(0, prefix.size(), prefix);
}

inline bool endsWith(const string &who, const string &suffix) {
    if (suffix.size() > who.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), who.rbegin());
}

bool tryEat(string &t, const string &prefix) {
    if (startsWith(t, prefix)) {
        t = t.substr(prefix.length());
        return true;
    }
    return false;
}
bool tryEatB(string &t, const string &suffix) {
    if (endsWith(t, suffix)) {
        t = t.substr(0, t.length() - suffix.length());
        return true;
    }
    return false;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

// trim from end (in place)
static inline void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

// trim from both ends (in place)
static inline void trimHere(string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from both ends (copy)
static inline string trim(const string &s) {
    string s2(s);
    trimHere(s2);
    return s2;
}

template <char delimiter> class WordDelimitedBy : public std::string {};

template <char del> vector<string> split(const string &str) {
    std::istringstream iss(str);
    return std::vector<std::string>(
        std::istream_iterator<WordDelimitedBy<del>>{iss},
        std::istream_iterator<WordDelimitedBy<del>>());
}

template <typename K, typename T, typename F>
vector<K> vecMap(const vector<T> &vec, F f) {
    vector<K> out;
    out.resize(vec.size());
    for (size_t i = 0; i < vec.size(); i++)
        out[i] = f(vec[i]);
    return out;
}

bool isAnyOf(const string &x, const vector<string> &of) {
    for (auto o : of)
        if (o == x)
            return true;
    return false;
}

bool hasAnyOf(const string &x, const vector<string> &of) {
    for (auto o : of)
        if (has(x, o))
            return true;
    return false;
}

inline bool endsWithAny(const string &who, const vector<string> &of) {
    for (auto o : of)
        if (endsWith(who, o))
            return true;
    return false;
}

inline bool startsWithAny(const string &who, const vector<string> &of) {
    for (auto o : of)
        if (startsWith(who, o))
            return true;
    return false;
}

inline void replaceHere(string &str, const string &what, const string &with) {
    if (what.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(what, start_pos)) != std::string::npos) {
        str.replace(start_pos, what.length(), with);
        start_pos += with.length();
    }
}

inline string umlautify(string in) {
    replaceHere(in, "a", "ä");
    replaceHere(in, "A", "Ä");
    replaceHere(in, "u", "ü");
    replaceHere(in, "U", "Ü");
    replaceHere(in, "o", "ö");
    replaceHere(in, "O", "Ö");
    replaceHere(in, "äü", "äu");
    replaceHere(in, "Äü", "Äu");
    replaceHere(in, "eü", "eu");
    replaceHere(in, "äi", "ai");
    return in;
}

inline size_t count(const string &who, const vector<char> &infix) {
    size_t c = 0;
    for (auto i : infix) {
        c += std::count(who.begin(), who.end(), i);
    }
    return c;
}

inline size_t count(const string &who, const vector<string> &infix) {
    size_t c = 0;
    for (auto i : infix) {
        size_t nPos = 0;
        while ((nPos = who.find(i, nPos)) != string::npos) {
            c++;
            nPos += i.size();
        }
    }
    return c;
}

// Counts all occurrences but every char in the string can only be used once to
// match any infix
inline size_t countInOrder(const string &who, const vector<string> &infix) {
    size_t c = 0;
    for (size_t i = 0; i < who.size(); i++) {
        for (auto inf : infix) {
            if (inf.size() <= who.size() - i) {
                if (who.substr(i, inf.size()) == inf) {
                    c++;
                    i += inf.size();
                    break;
                }
            }
        }
    }
    return c;
}

#include <clocale>

class LOCALE {
  public:
    LOCALE() { std::setlocale(LC_ALL, "de_DE.UTF-8"); }
};
const thread_local LOCALE _LOCALE;

string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++)
        s[i] = std::tolower(s[i]);
    return s;
}
