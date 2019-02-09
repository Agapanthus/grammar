#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "util.hpp"

using std::cout, std::endl;
using std::vector, std::string, std::istream, std::ostream, std::shared_ptr,
    std::make_shared, std::tuple;

namespace punctuation {

template <char delimiter> class WordDelimitedBy : public std::string {};

template <char del> vector<string> split(const string &str) {
    std::istringstream iss(str);
    return std::vector<std::string>(
        std::istream_iterator<WordDelimitedBy<del>>{iss},
        std::istream_iterator<WordDelimitedBy<del>>());
}



enum class NodeTypes { word, punctuation, sentence, closing, opening, none };

const vector<string> pairsOpen = {"\"", "»", "(", "[", "{"};
const vector<string> pairsClose = {"\"", "«", ")", "]", "}"};
const vector<string> end = {".", "!", "?", ";"};
const vector<string> breaks = {",", "+",  ":", "_", "#", "*",
                               "~", "\"", "&", "%", "$", "§",
                               "/", "°",  "<", ">", "|", "^"}; // "-",

class Node;
ostream &operator<<(ostream &out, const shared_ptr<Node> &s);

class Node {
  private:
    const string content;
    vector<shared_ptr<Node>> nodes;

  public:
    const NodeTypes type;
    Node(const NodeTypes type, const string &content)
        : type(type), content(content) {}

    const vector<shared_ptr<Node>> &getChildren() const { return nodes; }

    const string &getContent() const { return content; }

  private:
    void pushWord(const string &word) {
        if (word.length() > 0)
            nodes.push_back(make_shared<Node>(NodeTypes::word, word));
    }
    void pushPunctuation(const string &word) {
        if (word.length() > 0)
            nodes.push_back(make_shared<Node>(NodeTypes::punctuation, word));
    }
    void pushClosing(const string &word) {
        if (word.length() > 0)
            nodes.push_back(make_shared<Node>(NodeTypes::closing, word));
    }
    void pushOpening(const string &word) {
        nodes.push_back(make_shared<Node>(NodeTypes::opening, word));
    }
    void pushSentence(shared_ptr<Node> s) {
        if (s->getChildren().size() > 0)
            nodes.push_back(s);
    }

    int isAny(const string &me, const size_t offset, const vector<string> of) {
        for (size_t ind = 0; ind < of.size(); ind++) {
            const string &o = of[ind];
            if (me.length() - offset >= o.length()) {
                bool equal = true;
                for (size_t i = 0; i < o.length(); i++) {
                    if (o[i] != me[offset + i]) {
                        equal = false;
                        break;
                    }
                }
                if (equal)
                    return ind;
            }
        }
        return -1;
    }

  public:
    tuple<int, string> parse(istream &src, string left, bool isToplevel) {
        if (nodes.size() != 0)
            throw Exception("Don't run this multiple times!");
        string word;
        while (true) {
            if (left.length() > 0) {
                word = left;
                left = "";
            } else {
                if (src.eof())
                    throw Exception("Unexpected end of input");
                src >> word;
            }
            size_t start = 0;
            int index;
            for (size_t i = 0; i < word.length(); i++) {

                if ((index = isAny(word, i, pairsClose)) >= 0) {
                    if (!isToplevel) {
                        // pushClosing(pairsClose[index]);
                        // start = i + pairsClose[index].length();
                        pushWord(word.substr(start, i - start));
                        start = i;
                        return tuple<int, string>(index, word.substr(start));
                    }
                }

                if ((index = isAny(word, i, pairsOpen)) >= 0) {
                    pushOpening(pairsOpen[index]);
                    start = i + pairsOpen[index].length();
                    string lLeft = word.substr(start);
                    int resp = 0;
                    while (true) {
                        shared_ptr<Node> s =
                            make_shared<Node>(NodeTypes::sentence, "");
                        auto [iResp, iLeft] = s->parse(src, lLeft, false);
                        pushSentence(s);
                        lLeft = iLeft;
                        resp = iResp;
                        if (resp >= 0)
                            break;
                    }
                    if (resp != index) {
                        throw Exception("unmatched: " + pairsClose[resp] +
                                        " (" + std::to_string(resp) + ")" +
                                        " != " + pairsOpen[index] + " (" +
                                        std::to_string(index) + ")");
                    }
                    int closeIndex;
                    if ((closeIndex = isAny(lLeft, i, pairsClose)) < 0)
                        throw Exception("Expected pairsClose");
                    pushClosing(pairsClose[closeIndex]);

                    left += lLeft.substr(pairsClose[index].length());
                    start = word.length();
                    break;
                }

                if ((index = isAny(word, i, end)) >= 0) {
                    pushWord(word.substr(start, i - start));
                    pushPunctuation(end[index]);
                    start = i + end[index].length();
                    return tuple<int, string>(-1, word.substr(start));
                }

                if ((index = isAny(word, i, breaks)) >= 0) {
                    pushWord(word.substr(start, i - start));
                    pushPunctuation(breaks[index]);
                    start = i + breaks[index].length();
                }
            }
            pushWord(word.substr(start));
        }
    }
};

void recursiveOut(ostream &out, const shared_ptr<Node> &s, string prefix) {
    const vector<shared_ptr<Node>> nodes = s->getChildren();
    NodeTypes prev = NodeTypes::none;

    for (shared_ptr<Node> c : nodes) {

        switch (c->type) {
        case NodeTypes::word:
            if (prev != NodeTypes::none)
                out << " ";
            out << "\033[1;32m" << c->getContent() << "\033[0m";
            break;
        case NodeTypes::punctuation:
            out << "\033[1;31m" << c->getContent() << "\033[0m";
            break;
        case NodeTypes::opening:
        case NodeTypes::closing:
            out << "\033[1;33m" << c->getContent() << "\033[0m";
            break;
        case NodeTypes::sentence:
            out << "\033[1;34m(\033[0m";
            recursiveOut(out, c, "  " + prefix);
            out << "\033[1;34m)\033[0m";
            break;
        default:
            out << "?";
            break;
        }
        prev = c->type;
    }
}

/*
std::wostream &operator<<(std::wostream &out, string s) {
    out << widen(s);
    return out;
}*/


void recursiveWOut(std::wostream &out, const shared_ptr<Node> &s,
                   std::wstring prefix) {
    const vector<shared_ptr<Node>> nodes = s->getChildren();
    NodeTypes prev = NodeTypes::none;

    for (shared_ptr<Node> c : nodes) {

        switch (c->type) {
        case NodeTypes::word:
            if (prev != NodeTypes::none)
                out << L" ";
            out << green << widen(c->getContent()) << green;
            break;
        case NodeTypes::punctuation:
            out << red << widen(c->getContent()) << red;
            break;
        case NodeTypes::opening:
        case NodeTypes::closing:
            out << yellow << widen(c->getContent()) << yellow;
            break;
        case NodeTypes::sentence:
            out << blue << L"(" << black;
            recursiveWOut(out, c, L"  " + prefix);
            out << blue << L")" << black;
            break;
        default:
            out << L"?";
            break;
        }
        prev = c->type;
    }
}

std::wostream &operator<<(std::wostream &out, const shared_ptr<Node> &s) {
    recursiveWOut(out, s, L"");
    return out;
}

ostream &operator<<(ostream &out, const shared_ptr<Node> &s) {
    recursiveOut(out, s, "");
    return out;
}

class pLexer {

  private:
    istream &src;
    string left = "";

  public:
    pLexer(istream &src) : src(src) {}

    shared_ptr<Node> nextSentence() {
        shared_ptr<Node> s = make_shared<Node>(NodeTypes::sentence, "");
        if (src.eof())
            return s;
        auto [res, iLeft] = s->parse(src, left, true);
        if (res != -1)
            throw Exception("expected negative res");
        left = iLeft;
        return s;
    }
};

} // namespace punctuation