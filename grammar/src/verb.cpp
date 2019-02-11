#include "verb.hpp"

#include <iostream>
using std::cout, std::endl;

string Verb::indPres(const Verbspec::Person person, bool plural,
                     const Dictionary &dict, bool forceArtificial) const {

    switch (person) {
    case Verbspec::First:
        if (plural) {
            if (!forceArtificial && !base.present.first.plural.empty())
                return base.present.first.plural[0];

            // TODO
            return "";
        } else {
            return indPres1Sg();
        }

    case Verbspec::Second:
        // TODO
        return "";

    case Verbspec::Third:
        // TODO
        return "";

    case Verbspec::Polite:
        // TODO
        return "";

    case Verbspec::Majestic:
        // TODO
        return "";

    default:
        cout << "Error: TODO: person not supported" << endl;
        return "";
    }
}

bool Verb::testIndPres(const Verbspec::Person person, bool plural,
                       const Dictionary &dict, const stringLower &test) const {

    switch (person) {
    case Verbspec::First:
        if (plural) {
            if (base.present.first.plural.empty())
                break;
            return isAnyOf<stringLower>(test, base.present.first.plural);
        }
        if (base.present.first.singular.empty())
            break;
        return isAnyOf<stringLower>(test, base.present.first.singular);

    case Verbspec::Second:
        if (plural) {
            if (base.present.second.plural.empty())
                break;
            return isAnyOf<stringLower>(test, base.present.first.plural);
        }
        if (base.present.second.singular.empty())
            break;
        return isAnyOf<stringLower>(test, base.present.first.singular);

    case Verbspec::Third:
        if (plural) {
            if (base.present.third.plural.empty())
                break;
            return isAnyOf<stringLower>(test, base.present.first.plural);
        }
        if (base.present.third.singular.empty())
            break;
        return isAnyOf<stringLower>(test, base.present.first.singular);
    default:
        // Fallthrough
        break;
    }
    // TODO
    // cout << "Error: You must never reach this." << endl;
    return false;
}

string Verb::get(const Verbspec &spec, const Dictionary &dict,
                 bool forceArtificial) const {

    if (spec.person == Verbspec::AnyPerson ||
        spec.number == Verbspec::AnyNumber) {
        cout << "Error: can't use 'any' here." << endl;
        return "";
    }

    switch (spec.mode) {
    case Verbspec::Indicative:
        switch (spec.tempus) {
        case Verbspec::Present:
            return indPres(spec.person, spec.number == Verbspec::Plural, dict,
                           forceArtificial);
        default:
            cout << "Error: TODO: unknown tempus" << endl;
            break;
        }
        break;
    default:
        cout << "Error: TODO: unknown mode" << endl;
        break;
    }
    return "";
}

bool Verb::test(const Verbspec &spec, const Dictionary &dict,
                const stringLower &test) const {

    if (spec.person == Verbspec::AnyPerson ||
        spec.number == Verbspec::AnyNumber) {
        cout << "Error: can't use 'any' here." << endl;
        return "";
    }

    switch (spec.mode) {
    case Verbspec::Indicative:
        switch (spec.tempus) {
        case Verbspec::Present: {
            bool res = testIndPres(spec.person, spec.number == Verbspec::Plural,
                                   dict, test);
        
            return res;
        }
        default:
            cout << "Error: TODO: unknown tempus" << endl;
            break;
        }
        break;
    default:
        cout << "Error: TODO: unknown mode" << endl;
        break;
    }
    return true;
}