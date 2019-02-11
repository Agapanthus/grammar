#pragma once
#include "grammar.hpp"
#include "noun.hpp"

// TODO: Only for debugging
#include <iostream>
using std::cout, std::endl;

struct Verbspec {

    enum Mode {
        Stem,
        Indicative,
        Subjunctive, // Konjunktiv I
        Irrealis,    // Konjunktiv II
        Imperative,
        Infinitive,
        Inflective,
        Participle, // Present or Perfect
        Gerundive
    } mode;

    enum Diathesis { Active = 0, Passive = 1 } diathesis;

    enum Number { Singular = 0, Plural = 1, AnyNumber = 2 } number;

    enum Person {
        AnyPerson = 0,
        First = 1,
        Second = 2,
        Third = 3,
        Polite = 4,
        Majestic = 5,
    } person;

    enum Tempus {
        Present,
        Preterite,
        Perfect,
        Pluperfect,
        FutureI,
        FutureII,
        DoublePerfect,
        DoublePluperfect,
        DoubleFuture
    } tempus;

    Cases c; // For Gerundive

    Verbspec() = delete;
    Verbspec(Mode mode, Tempus tempus, Diathesis diathesis = Active,
             Person person = AnyPerson, Number number = AnyNumber)
        : mode(mode), tempus(tempus), diathesis(diathesis), person(person),
          number(number), c(Cases::Nominative) {}

    Verbspec(Tempus tempus, Diathesis diathesis = Active,
             Person person = AnyPerson, Number number = AnyNumber)
        : Verbspec(Indicative, tempus, diathesis, person, number) {}
};

class Verb {
  public:
    struct {
        vector<string> presentInfinitive;
        Person present;

        vector<string> participleII;
        vector<string> subjunctive_firstSingular;
        vector<string> irrealis_firstSingular;
        vector<string> preterite_firstSingular;
        vector<string> auxiliary;

    } base;
    Numeri imperative;

    ////////////////////////////////////////////////////////

    /* bool s1(const stringLower test) const {
         for (const stringLower g : base.present.first.singular) {
             if (g == test)
                 return true;
         }
         if (base.present.first.singular.empty()) {
             return s1() == test;
         }
         return false;
     }

     string s1(const bool forceArtificial = false) const {
         if (forceArtificial || base.present.first.singular.empty()) {
             // TODO! try creating it from infinitive
         }
         return base.present.first.singular[0];
     }

     bool s2(const stringLower test) const {
         for (const stringLower g : base.present.second.singular) {
             if (g == test)
                 return true;
         }
         if (base.present.second.singular.empty()) {
             return s2() == test;
         }
         return false;
     }

     string s2(const bool forceArtificial = false) const {
         if (forceArtificial || base.present.second.singular.empty()) {
             const string s1 = this->s1(false);
             if (endsWith(s1, "e")) {
                 return s1.substr(0, s1.size() - 1) + "st";
             } else {
                 cout << "Error: irregular s1: " << s1 << endl;
                 return s1;
             }
         }
         return base.present.second.singular[0];
     }*/

    string indPres1Sg() const {
        if (base.present.first.singular.empty()) {
            cout << "Error: First Person needed!"
                 << endl; // TODO: Try to get it from infinitive
            return "";
        }
        return base.present.first.singular[0];
    }

    string indPres(const Verbspec::Person person, bool plural,
                   const Dictionary &dict, bool forceArtificial = true) const;

    bool testIndPres(const Verbspec::Person person, bool plural,
                     const Dictionary &dict, const stringLower &test) const;

    string get(const Verbspec &spec, const Dictionary &dict,
               bool forceArtificial = true) const;

    bool test(const Verbspec &spec, const Dictionary &dict,
              const stringLower &test) const;

    ///////////////////////////////////////////////////////

    bool set(const Verbspec &spec, const string &word) {
        switch (spec.mode) {
        case Verbspec::Indicative:
            if (spec.tempus == Verbspec::Present) {
                if (spec.number == Verbspec::Singular ||
                    spec.number == Verbspec::AnyNumber) {
                    switch (spec.person) {
                    case Verbspec::First:
                        base.present.first.singular.push_back(word);
                        return true;
                    case Verbspec::Second:
                        base.present.second.singular.push_back(word);
                        return true;
                    case Verbspec::Third:
                        base.present.third.singular.push_back(word);
                        return true;

                    default:
                        return false;
                    }
                }
                if (spec.number == Verbspec::Plural ||
                    spec.number == Verbspec::AnyNumber) {
                    switch (spec.person) {
                    case Verbspec::First:
                        base.present.first.plural.push_back(word);
                        return true;
                    case Verbspec::Second:
                        base.present.second.plural.push_back(word);
                        return true;
                    case Verbspec::Third:
                        base.present.third.plural.push_back(word);
                        return true;
                    default:
                        return false;
                    }
                }
                return true;

            } else if (spec.tempus == Verbspec::Preterite) {
                if (spec.number != Verbspec::Singular)
                    return false;
                if (spec.person != Verbspec::First)
                    return false;
                base.preterite_firstSingular.push_back(word);
                return true;
            } else
                return false;

        case Verbspec::Subjunctive:
            if (spec.person != Verbspec::First)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            base.subjunctive_firstSingular.push_back(word);
            return true;

        case Verbspec::Irrealis:
            if (spec.person != Verbspec::First)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            base.irrealis_firstSingular.push_back(word);
            return true;

        case Verbspec::Participle:
            if (spec.tempus != Verbspec::Perfect)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            base.participleII.push_back(word);
            return true;

        case Verbspec::Infinitive:
            if (spec.tempus != Verbspec::Present)
                return false;
            base.presentInfinitive.push_back(word);
            return true;

        case Verbspec::Imperative:
            if ((spec.number == Verbspec::Singular ||
                 spec.number == Verbspec::AnyNumber)) {
                imperative.singular.push_back(word);
                return true;
            }
            if ((spec.number == Verbspec::Plural ||
                 spec.number == Verbspec::AnyNumber)) {
                imperative.plural.push_back(word);
                return true;
            }
            return false;

        default:
            return false;
        }
    }

    bool predefined(const Verbspec &spec) const {
        switch (spec.mode) {
        case Verbspec::Indicative:
            if (spec.tempus == Verbspec::Present) {
                if (spec.number == Verbspec::Singular ||
                    spec.number == Verbspec::AnyNumber) {
                    switch (spec.person) {
                    case Verbspec::First:
                        if (base.present.first.singular.empty())
                            return false;
                        break;
                    case Verbspec::Second:
                        if (base.present.second.singular.empty())
                            return false;
                        break;
                    case Verbspec::Third:
                        if (base.present.third.singular.empty())
                            return false;
                        break;
                    case Verbspec::AnyPerson:
                        if (base.present.first.singular.empty() ||
                            base.present.second.singular.empty() ||
                            base.present.third.singular.empty())
                            return false;
                        break;
                    default:
                        return false;
                    }
                }
                if (spec.number == Verbspec::Plural ||
                    spec.number == Verbspec::AnyNumber) {
                    switch (spec.person) {
                    case Verbspec::First:
                        if (base.present.first.plural.empty())
                            return false;
                        break;
                    case Verbspec::Second:
                        if (base.present.second.plural.empty())
                            return false;
                        break;
                    case Verbspec::Third:
                        if (base.present.third.plural.empty())
                            return false;
                        break;
                    case Verbspec::AnyPerson:
                        if (base.present.first.plural.empty() ||
                            base.present.second.plural.empty() ||
                            base.present.third.plural.empty())
                            return false;
                        break;
                    default:
                        return false;
                    }
                }
                return true;

            } else if (spec.tempus == Verbspec::Preterite) {
                if (spec.number != Verbspec::Singular)
                    return false;
                if (spec.person != Verbspec::First)
                    return false;
                return !base.preterite_firstSingular.empty();
            } else
                return false;

        case Verbspec::Subjunctive:
            if (spec.person != Verbspec::First)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            return !base.subjunctive_firstSingular.empty();

        case Verbspec::Irrealis:
            if (spec.person != Verbspec::First)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            return !base.irrealis_firstSingular.empty();

        case Verbspec::Participle:
            if (spec.tempus != Verbspec::Perfect)
                return false;
            if (spec.number != Verbspec::Singular)
                return false;
            return !base.participleII.empty();

        case Verbspec::Infinitive:
            if (spec.tempus != Verbspec::Present)
                return false;
            return !base.presentInfinitive.empty();

        case Verbspec::Imperative:
            if ((spec.number == Verbspec::Singular ||
                 spec.number == Verbspec::AnyNumber) &&
                imperative.singular.empty())
                return false;
            if ((spec.number == Verbspec::Plural ||
                 spec.number == Verbspec::AnyNumber) &&
                imperative.plural.empty())
                return false;
            return true;

        default:
            return false;
        }
    }

    ////////////////////////////////////////////////////////

    void serialize(ostream &out) const {
        serializeVector(out, base.presentInfinitive);
        serializeVector(out, base.participleII);
        serializeVector(out, base.irrealis_firstSingular);
        serializeVector(out, base.irrealis_firstSingular);
        serializeVector(out, base.preterite_firstSingular);
        serializeVector(out, base.auxiliary);
        base.present.serialize(out);
        imperative.serialize(out);
    }

    void deserialize(istream &in) {
        deserializeVector(in, base.presentInfinitive);
        deserializeVector(in, base.participleII);
        deserializeVector(in, base.irrealis_firstSingular);
        deserializeVector(in, base.irrealis_firstSingular);
        deserializeVector(in, base.preterite_firstSingular);
        deserializeVector(in, base.auxiliary);
        base.present.deserialize(in);
        imperative.deserialize(in);
    }

    void buildMap(map<string, vector<Word>> &dict) const {
        Word me({(Noun *)this, WordType::Verb});
        imperative.buildMap(me, dict);
        for (const auto &s : base.presentInfinitive)
            emplace(dict, s, me);
        for (const auto &s : base.participleII)
            emplace(dict, s, me);
        for (const auto &s : base.irrealis_firstSingular)
            emplace(dict, s, me);
        for (const auto &s : base.irrealis_firstSingular)
            emplace(dict, s, me);
        for (const auto &s : base.preterite_firstSingular)
            emplace(dict, s, me);
        for (const auto &s : base.auxiliary)
            emplace(dict, s, me);
        base.present.buildMap(me, dict);
    }
};
