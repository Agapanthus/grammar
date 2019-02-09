#include <assert.h>
#include <istream>
#include <locale>
#include <memory>
#include <streambuf>
#include <string>

using std::string, std::streambuf, std::istream, std::shared_ptr, std::endl;

template <class Streambuf, typename... Args> class iStrFil : public istream {
  private:
    Streambuf buf;
    shared_ptr<istream> in; // So the parent won't get deleted before me!

  public:
    iStrFil(Args &&... args, const shared_ptr<istream> in)
        : in(in), buf(in->rdbuf(), std::forward<Args>(args)...), istream(&buf) {
    }
};

// Skips everything before and including the given string
class SkipTo : public std::streambuf {

  private:
    char buf;
    const string s;
    std::streambuf *src;
    bool first = true;

  public:
    SkipTo(std::streambuf *src, const string &s) : s(s), src(src){};
    virtual ~SkipTo(){};

  protected:
    // virtual int overflow(int);
    // virtual int sync();
    // virtual streambuf *setbuf(char *p, int len);

    virtual int underflow() {
        int result(EOF);
        if (gptr() < egptr())
            result = *gptr();
        else if (src != NULL) {

            if (first) {
                first = false;
                const size_t size = s.length();
                char *contents = new char[size + 1];
                memset(contents, 0, size + 1);
                if (!src->sgetn(contents, size)) {
                    result = EOF;
                } else
                    while (true) {
                        if (string(contents) == s) {
                            result = src->sbumpc();
                            break;
                        }

                        for (size_t i = 0; i < size - 1; i++)
                            contents[i] = contents[i + 1];
                        contents[size - 1] = src->sbumpc();
                        if (contents[size - 1] == EOF) {
                            result = EOF;
                            break;
                        }
                    }
                delete[] contents;
            } else {
                result = src->sbumpc();
            }

            if (result != EOF) {
                assert(result >= 0 && result <= UCHAR_MAX);
                buf = result;
                setg(&buf, &buf, &buf + 1);
            }
        }
        return result;
    }
};

class LineNumberStreambuf : public std::streambuf {
    std::streambuf *mySource;
    std::istream *myOwner;
    bool myIsAtStartOfLine;
    int myLineNumber;

    char myBuffer;

  protected:
    int underflow() {
        int ch = mySource->sbumpc();
        if (ch != EOF) {
            myBuffer = ch;
            setg(&myBuffer, &myBuffer, &myBuffer + 1);
            if (myIsAtStartOfLine) {
                ++myLineNumber;
            }
            myIsAtStartOfLine = myBuffer == '\n';
        }
        return ch;
    }

  public:
    LineNumberStreambuf(std::streambuf *source)
        : mySource(source), myOwner(nullptr), myIsAtStartOfLine(true),
          myLineNumber(0) {}
    LineNumberStreambuf(std::istream &owner)
        : mySource(owner.rdbuf()), myOwner(&owner), myIsAtStartOfLine(true),
          myLineNumber(0) {
        myOwner->rdbuf(this);
    }
    ~LineNumberStreambuf() {
        if (myOwner != nullptr) {
            myOwner->rdbuf(mySource);
        }
    }
    int lineNumber() const { return myLineNumber; }
};

// Delete all instances of that char
class RemoveChar : public std::streambuf {

  private:
    char buf;
    const char s;
    std::streambuf *src;

  public:
    RemoveChar(std::streambuf *src, const char &s) : s(s), src(src){};
    virtual ~RemoveChar(){};

  protected:
    // virtual int overflow(int);
    // virtual int sync();
    // virtual streambuf *setbuf(char *p, int len);

    virtual int underflow() {
        int result(EOF);
        if (gptr() < egptr())
            result = *gptr();
        else if (src != NULL) {
            int ch(src->sbumpc());

            // Scan until you reach a character which is not s
            if (ch == s) {
                while (ch != EOF && ch == s)
                    ch = src->sbumpc();
            }
            result = ch;

            if (result != EOF) {
                assert(result >= 0 && result <= UCHAR_MAX);
                buf = result;
                setg(&buf, &buf, &buf + 1);
            }
        }
        return result;
    }
};