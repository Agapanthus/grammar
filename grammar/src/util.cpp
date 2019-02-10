#include "util.hpp"

#include "utf8.h"

#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

#include <iostream>
#include <algorithm>
#include <cctype>

using std::cout, std::endl;



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

void fixUTF8(std::string &str) {
    try {
        str += " "; // To avoid not_enough_room
        std::string temp;
        utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
        str = temp;
    } catch (... /*utf8::not_enough_room &e*/) {
        cout << endl << "Error sani: " << endl << str << endl;
    }
}

// trim from start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

// trim from end (in place)
void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}


const thread_local LOCALE _LOCALE;

string toLower(const string &s) {
    // for (size_t i = 0; i < s.size(); i++) s[i] = std::tolower(s[i]);
    // std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    icu::UnicodeString str(s.c_str(), "UTF-8");
    string target;
    str.toLower("de_DE").toUTF8String(target);
    return target;
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