#include "kompositum.hpp"
#include "dictionary.hpp"

bool isKStem(const stringLower &str,
             const Dictionary &dict) { // TODO: not very robust, isn't it?
    vector<Word> words = dict.find(str);
    for (const Word w : words) {
        switch (w.w) {
        case WordType::Noun:
            if (w.noun->type == NounType::Noun) {
                if (w.noun->ns(str)) {
                    return true;
                } else if (w.noun->np(dict, str)) {
                    return true;
                }
            }
            break;

        case WordType::Adjective:
            if (!w.adj->positive.empty()) // TODO: Also consider stems
                for (const stringLower pos : w.adj->positive)
                    if (pos == str)
                        return true;
            break;

        default:
            break;
        }
    }
    return false;
}

#include "grammarIO.hpp"

bool isKHead(const string &str, const Dictionary &dict) {
    vector<Word> words = dict.find(str);
    for (const Word w : words) {
        switch (w.w) {
        case WordType::Noun:
            if (w.noun->type == NounType::Noun)
                return true; // It's a Noun, perfect match. We want nothing
                             // more.
            break;
        default:
            break;
        }
    }
    return false;
}

vector<string> breakKompositum(const string &word, const Dictionary &dict) {
    if (word.size() < 4)
        return {word};
    for (size_t i = 3; i < word.size() - 3; i++) {
        const string start = word.substr(0, i);
        string end = word.substr(i, word.size() - i);
        bool allOk = false;
        vector<string> begin = vector({start});
        if (isKStem(start, dict)) {
            if (isKHead(end, dict)) {
                allOk = true;
            }

            // Fugenelement (there are also others, but I don't really care -
            // see https://de.wikipedia.org/wiki/Komposition_(Grammatik) )
            else if (word[i] == 's' &&
                     !isAnyOf(string("") + word[i - 1], vocals) &&
                     !isAnyOf(string("") + word[i + 1], vocals)) {
                end = word.substr(i + 1, word.size() - i - 1);
                if (isKHead(end, dict)) {
                    begin.push_back("s");
                    allOk = true;
                }
            }
        }
        if (allOk) {
            const vector<string> a = breakKompositum(end, dict);
            begin.insert(begin.end(), a.begin(), a.end());
            return begin;
        }
    }
    return {word};
}
