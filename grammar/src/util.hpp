#pragma once

#include <ostream>
#include <string>
#include <vector>
using std::ostream, std::wostream;
using std::string, std::wstring, std::to_string;
using std::vector;

//#include <customOperator.h>

std::wstring widen(string s);

void fixUTF8(std::string &str);


enum Color { blue, red, green, yellow, black, greenblue, purple, grey };

const char *colorToString(const Color color);

static inline ostream &operator<<(ostream &out, const Color color) {
    out << colorToString(color);
    return out;
}

static inline wostream &operator<<(wostream &out, const Color color) {
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
static inline int codePoint(const char &c) {
    if ((c & 0xf8) == 0xf0)
        return 4;
    else if ((c & 0xf0) == 0xe0)
        return 3;
    else if ((c & 0xe0) == 0xc0)
        return 2;
    return 1;
}

template <typename T, typename K> inline bool has(const T &t, const K &k) {
    return std::find(t.begin(), t.end(), k) != t.end();
}

template <> inline bool has(const string &t, const string &k) {
    return t.find(k) != string::npos;
}

static inline bool has(const string &t, const char *k) { return has(t, string(k)); }

static inline bool only(const vector<string> &vec, const string &comp) {
    if (vec.size() != 1)
        return false;
    return vec[0] == comp;
}

static inline string removeSub(const string &in, const string &what) {
    string cp(in);
    size_t pos = std::string::npos;
    while ((pos = cp.find(what)) != std::string::npos)
        cp.erase(pos, what.length());
    return cp;
}

static inline bool startsWith(const string &who, const string &prefix) {
    return !who.compare(0, prefix.size(), prefix);
}

static inline bool endsWith(const string &who, const string &suffix) {
    if (suffix.size() > who.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), who.rbegin());
}

static inline bool tryEat(string &t, const string &prefix) {
    if (startsWith(t, prefix)) {
        t = t.substr(prefix.length());
        return true;
    }
    return false;
}
static inline bool tryEatB(string &t, const string &suffix) {
    if (endsWith(t, suffix)) {
        t = t.substr(0, t.length() - suffix.length());
        return true;
    }
    return false;
}

// trim from start (in place)
void ltrim(std::string &s);

// trim from end (in place)
void rtrim(string &s);

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

template <typename K, typename T, typename F>
static inline vector<K> vecMap(const vector<T> &vec, F f) {
    vector<K> out;
    out.resize(vec.size());
    for (size_t i = 0; i < vec.size(); i++)
        out[i] = f(vec[i]);
    return out;
}

static inline bool isAnyOf(const string &x, const vector<string> &of) {
    for (auto o : of)
        if (o == x)
            return true;
    return false;
}

static inline bool hasAnyOf(const string &x, const vector<string> &of) {
    for (auto o : of)
        if (has(x, o))
            return true;
    return false;
}

static inline bool endsWithAny(const string &who, const vector<string> &of) {
    for (auto o : of)
        if (endsWith(who, o))
            return true;
    return false;
}

static inline bool startsWithAny(const string &who, const vector<string> &of) {
    for (auto o : of)
        if (startsWith(who, o))
            return true;
    return false;
}

static inline void replaceHere(string &str, const string &what, const string &with) {
    if (what.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(what, start_pos)) != std::string::npos) {
        str.replace(start_pos, what.length(), with);
        start_pos += with.length();
    }
}

static inline string umlautify(string in) {
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

static inline size_t count(const string &who, const vector<char> &infix) {
    size_t c = 0;
    for (auto i : infix) {
        c += std::count(who.begin(), who.end(), i);
    }
    return c;
}

static inline size_t count(const string &who, const vector<string> &infix) {
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
static inline size_t countInOrder(const string &who, const vector<string> &infix) {
    size_t c = 0;
    for (size_t i = 0; i < who.size(); i++) {
        for (auto inf : infix) {
            if (inf.size() <= who.size() - i) {
                if (who.substr(i, inf.size()) == inf) {
                    c++;
                    i += inf.size() -
                         1; // The one is incremented in the loop header
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
const extern thread_local LOCALE _LOCALE;

string toLower(const string &s);