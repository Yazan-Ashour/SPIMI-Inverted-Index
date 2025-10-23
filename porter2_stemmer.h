/**
 * @file porter2_stemmer.h
 * @author Sean Massung (modified by ChatGPT)
 * @date Original: September 2012, Modified: October 2025
 *
 * Standalone implementation of the Porter2 stemming algorithm for English.
 * Based on: http://snowball.tartarus.org/algorithms/english/stemmer.html
 *
 * Original code © 2012 Sean Massung
 * Modifications © 2025 for standalone C++17 compatibility
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */

#ifndef _PORTER2_STEMMER_H_
#define _PORTER2_STEMMER_H_

#include <string>
#include <string_view>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace Porter2Stemmer
{
    /**
     * Main stemming function. Modifies the word in-place.
     */
    void stem(std::string& word);

    /**
     * Removes leading and trailing apostrophes or spaces.
     */
    void trim(std::string& word);

    namespace internal
    {
        size_t firstNonVowelAfterVowel(const std::string& word, size_t start);

        size_t getStartR1(const std::string& word);

        size_t getStartR2(const std::string& word, size_t startR1);

        void changeY(std::string& word);

        void step0(std::string& word);

        bool step1A(std::string& word);

        void step1B(std::string& word, size_t startR1);

        void step1C(std::string& word);

        void step2(std::string& word, size_t startR1);

        void step3(std::string& word, size_t startR1, size_t startR2);

        void step4(std::string& word, size_t startR2);

        void step5(std::string& word, size_t startR1, size_t startR2);

        inline bool isShort(const std::string& word);

        bool special(std::string& word);

        bool isVowel(char ch);

        bool isVowelY(char ch);

        bool endsWith(std::string_view word, std::string_view str);

        bool endsInDouble(const std::string& word);

        bool replaceIfExists(std::string& word, std::string_view suffix,
                             std::string_view replacement, size_t start);

        bool isValidLIEnding(char ch);

        bool containsVowel(const std::string& word, size_t start, size_t end);
    } // namespace internal

} // namespace Porter2Stemmer

#endif