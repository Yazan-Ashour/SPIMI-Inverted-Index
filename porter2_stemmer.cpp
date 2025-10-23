#include "porter2_stemmer.h"
#include <cctype>
#include <iostream>

namespace Porter2Stemmer
{
    using namespace std;
    using namespace internal;

    void stem(string& word)
    {
        if (word.size() <= 2)
            return;

        if (special(word))
            return;

        changeY(word);

        size_t startR1 = getStartR1(word);
        size_t startR2 = getStartR2(word, startR1);

        step0(word);
        if (step1A(word))
            return;

        step1B(word, startR1);
        step1C(word);
        step2(word, startR1);
        step3(word, startR1, startR2);
        step4(word, startR2);
        step5(word, startR1, startR2);

        // Final tidy-up
        for (auto& c : word)
            c = static_cast<char>(tolower(c));
    }

    void trim(string& word)
    {
        while (!word.empty() && (word.front() == '\'' || isspace(word.front())))
            word.erase(word.begin());
        while (!word.empty() && (word.back() == '\'' || isspace(word.back())))
            word.pop_back();
    }

    namespace internal
    {
        bool isVowel(char ch)
        {
            ch = static_cast<char>(tolower(ch));
            return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u';
        }

        bool isVowelY(char ch)
        {
            return isVowel(ch) || ch == 'y' || ch == 'Y';
        }

        bool endsWith(string_view word, string_view str)
        {
            return word.size() >= str.size() &&
                   word.compare(word.size() - str.size(), str.size(), str) == 0;
        }

        bool endsInDouble(const string& word)
        {
            if (word.size() < 2)
                return false;
            char c1 = word[word.size() - 1];
            char c2 = word[word.size() - 2];
            return c1 == c2 && (c1 == 'b' || c1 == 'd' || c1 == 'f' || c1 == 'g' ||
                                c1 == 'm' || c1 == 'n' || c1 == 'p' || c1 == 'r' ||
                                c1 == 't');
        }

        bool replaceIfExists(string& word, string_view suffix,
                             string_view replacement, size_t start)
        {
            if (word.size() >= suffix.size() &&
                word.compare(word.size() - suffix.size(), suffix.size(), suffix) == 0 &&
                word.size() - suffix.size() >= start)
            {
                word.replace(word.size() - suffix.size(), suffix.size(), replacement);
                return true;
            }
            return false;
        }

        bool containsVowel(const string& word, size_t start, size_t end)
        {
            for (size_t i = start; i < end && i < word.size(); ++i)
            {
                if (isVowel(word[i]))
                    return true;
            }
            return false;
        }

        size_t firstNonVowelAfterVowel(const string& word, size_t start)
        {
            for (size_t i = start + 1; i < word.size(); ++i)
            {
                if (!isVowel(word[i]) && isVowel(word[i - 1]))
                    return i + 1;
            }
            return word.size();
        }

        size_t getStartR1(const string& word)
        {
            if (word.rfind("gener", 0) == 0)
                return 5;
            if (word.rfind("commun", 0) == 0)
                return 6;
            if (word.rfind("arsen", 0) == 0)
                return 5;
            return firstNonVowelAfterVowel(word, 0);
        }

        size_t getStartR2(const string& word, size_t startR1)
        {
            return firstNonVowelAfterVowel(word, startR1);
        }

        void changeY(string& word)
        {
            if (!word.empty() && word[0] == 'y')
                word[0] = 'Y';
            for (size_t i = 1; i < word.size(); ++i)
            {
                if (word[i] == 'y' && isVowel(word[i - 1]))
                    word[i] = 'Y';
            }
        }

        bool special(string& word)
        {
            static const vector<pair<string, string>> exceptions = {
                {"skis", "ski"}, {"skies", "sky"}, {"dying", "die"},
                {"lying", "lie"}, {"tying", "tie"}, {"idly", "idl"},
                {"gently", "gentl"}, {"ugly", "ugli"}, {"early", "earli"},
                {"only", "onli"}, {"singly", "singl"}};

            for (auto& ex : exceptions)
            {
                if (word == ex.first)
                {
                    word = ex.second;
                    return true;
                }
            }
            return false;
        }

        void step0(string& word)
        {
            if (endsWith(word, "'s'"))
                word.erase(word.size() - 3);
            else if (endsWith(word, "'s"))
                word.erase(word.size() - 2);
            else if (endsWith(word, "'"))
                word.pop_back();
        }

        bool step1A(string& word)
        {
            if (endsWith(word, "sses"))
                word.replace(word.size() - 4, 4, "ss");
            else if (endsWith(word, "ies") || endsWith(word, "ied"))
            {
                if (word.size() > 4)
                    word.replace(word.size() - 3, 3, "i");
                else
                    word.replace(word.size() - 3, 3, "ie");
            }
            else if (endsWith(word, "us") || endsWith(word, "ss"))
            {
                // do nothing
            }
            else if (endsWith(word, "s"))
            {
                for (size_t i = 0; i + 2 < word.size(); ++i)
                {
                    if (isVowel(word[i]) && !isVowel(word[i + 1]))
                    {
                        word.erase(word.size() - 1);
                        break;
                    }
                }
            }
            return false;
        }

        void step1B(string& word, size_t startR1)
        {
            if (endsWith(word, "eedly"))
            {
                if (word.size() - 5 >= startR1)
                    word.replace(word.size() - 5, 5, "ee");
                return;
            }
            if (endsWith(word, "eed"))
            {
                if (word.size() - 3 >= startR1)
                    word.replace(word.size() - 3, 3, "ee");
                return;
            }

            vector<string> suffixes = {"ingly", "edly", "ing", "ed"};
            for (auto& suf : suffixes)
            {
                if (endsWith(word, suf) &&
                    containsVowel(word, 0, word.size() - suf.size()))
                {
                    word.erase(word.size() - suf.size());
                    if (endsWith(word, "at") || endsWith(word, "bl") || endsWith(word, "iz"))
                        word += "e";
                    else if (endsInDouble(word))
                        word.pop_back();
                    else if (isShort(word))
                        word += "e";
                    break;
                }
            }
        }

        void step1C(string& word)
        {
            if ((endsWith(word, "y") || endsWith(word, "Y")) &&
                word.size() > 2 && !isVowel(word[word.size() - 2]))
                word[word.size() - 1] = 'i';
        }

        void step2(string& word, size_t startR1)
        {
            replaceIfExists(word, "fulness", "ful", startR1);
            replaceIfExists(word, "ousness", "ous", startR1);
            replaceIfExists(word, "iveness", "ive", startR1);
            replaceIfExists(word, "ization", "ize", startR1);
            replaceIfExists(word, "biliti", "ble", startR1);
        }

        void step3(string& word, size_t startR1, size_t startR2)
        {
            replaceIfExists(word, "icate", "ic", startR1);
            replaceIfExists(word, "ative", "", startR2);
            replaceIfExists(word, "alize", "al", startR1);
        }

        void step4(string& word, size_t startR2)
        {
            static const vector<string> suffixes = {
                "ement", "ment", "able", "ible", "ance", "ence", "er", "ic", "al", "ism",
                "ion", "ou", "ant", "ent", "ism", "ate", "iti", "ous", "ive", "ize"};

            for (auto& suf : suffixes)
            {
                if (endsWith(word, suf) && word.size() - suf.size() >= startR2)
                {
                    if (suf == "ion")
                    {
                        char ch = word[word.size() - suf.size() - 1];
                        if (ch == 's' || ch == 't')
                            word.erase(word.size() - suf.size());
                    }
                    else
                        word.erase(word.size() - suf.size());
                    break;
                }
            }
        }

        void step5(string& word, size_t startR1, size_t startR2)
        {
            if (endsWith(word, "e"))
            {
                if (word.size() - 1 >= startR2)
                    word.erase(word.size() - 1);
                else if (word.size() - 1 >= startR1 && !isShort(word))
                    word.erase(word.size() - 1);
            }
            else if (endsWith(word, "l") && word.size() - 1 >= startR2 && word[word.size() - 2] == 'l')
            {
                word.pop_back();
            }
        }

        bool isShort(const string& word)
        {
            if (word.size() < 3)
                return false;
            char a = word[word.size() - 3];
            char b = word[word.size() - 2];
            char c = word[word.size() - 1];
            return !isVowel(a) && isVowel(b) && !isVowel(c) && c != 'w' && c != 'x' && c != 'Y';
        }
    } // namespace internal
} // namespace Porter2Stemmer
