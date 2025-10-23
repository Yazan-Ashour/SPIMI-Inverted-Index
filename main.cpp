#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <cctype>
#include <chrono>
#include "json.hpp"
#include "porter2_stemmer.h"

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

// STOP WORDS - LOWERCASE
static const set<string> STOP_WORDS = {
    "the", "and", "is", "in", "to", "of", "that", "it", "for", "as", "with", 
        "was", "this", "but", "be", "on", "by", "not", "he", "she", "or", "are", 
        "at", "from", "his", "her", "they", "an", "will", "would", "which", "we"
};

// BLOCK TERM LIMIT - WHEN DICTIONARY REACHES THIS NUMBER OF UNIQUE TERMS, FLUSH TO DISK
static const size_t BLOCK_TERM_LIMIT = 2500;

// CLEAN A WORD: KEEP ONLY ALPHABETIC CHARS, CONVERT TO LOWERCASE
string cleanWord(const string &word) {
    string out;
    out.reserve(word.size());
    for (char c : word) {
        if (isalpha(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

// TOKENIZE TEXT: RETURN VECTOR OF (CLEANED_WORD, POSITION)
vector<pair<string,int>> tokenizeWithPositions(const string &text) {
    vector<pair<string,int>> tokens;
    string token;
    stringstream ss(text);
    int pos = 0;
    while (ss >> token) {
        string cleaned = cleanWord(token);
            if (cleaned.size() > 2 && STOP_WORDS.find(cleaned) == STOP_WORDS.end()) {
                Porter2Stemmer::stem(cleaned); 
                tokens.emplace_back(cleaned, pos);
            }
        pos++;
    }
    return tokens;
}

// WRITE A SINGLE SPIMI BLOCK TO DISK (ONE TERM PER LINE, JSON FORMAT)
void writeBlockToDisk(const map<string, map<int, vector<int>>> &blockIndex, int blockNumber) {
    string filename = "spimi_block_" + to_string(blockNumber) + ".jsonl";
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "ERROR OPENING BLOCK FILE FOR WRITING: " << filename << endl;
        return;
    }

    // WRITE EACH TERM AS A SINGLE JSON OBJECT LINE
    for (const auto &termEntry : blockIndex) {
        const string &term = termEntry.first;
        const auto &postings = termEntry.second;

        json j;
        // FIRST ELEMENT: NUMBER OF DOCUMENTS CONTAINING THE TERM
        j[term].push_back((int)postings.size());

        // FOLLOWING ELEMENTS: OBJECTS MAPPING docID -> [positions]
        for (const auto &docEntry : postings) {
            int docID = docEntry.first;
            const vector<int> &positions = docEntry.second;
            // ENSURE POSITIONS ARE SORTED AND UNIQUE
            vector<int> sortedPositions = positions;
            sort(sortedPositions.begin(), sortedPositions.end());
            sortedPositions.erase(unique(sortedPositions.begin(), sortedPositions.end()), sortedPositions.end());

            json postingObj;
            postingObj[to_string(docID)] = sortedPositions;
            j[term].push_back(postingObj);
        }

        out << j.dump() << "\n";
    }

    out.close();
    cout << "WRITTEN SPIMI BLOCK TO DISK: " << filename << " (TERMS: " << blockIndex.size() << ")\n";
}

// MERGE ALL BLOCK FILES (spimi_block_*.jsonl) INTO A SINGLE IN-MEMORY INDEX
map<string, map<int, vector<int>>> mergeBlocksToIndex(const vector<string> &blockFiles) {
    map<string, map<int, vector<int>>> mergedIndex;

    for (const string &blockFile : blockFiles) {
        ifstream in(blockFile);
        if (!in.is_open()) {
            cerr << "ERROR OPENING BLOCK FILE FOR READING: " << blockFile << endl;
            continue;
        }

        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            json j = json::parse(line);
            for (auto &el : j.items()) {
                string term = el.key();
                json arr = el.value();

                for (size_t i = 1; i < arr.size(); ++i) {
                    json postingObj = arr[i];
                    for (auto &p : postingObj.items()) {
                        int docID = stoi(p.key());
                        vector<int> positions = p.value().get<vector<int>>();

                        // MERGE POSITIONS INTO mergedIndex[term][docID]
                        auto &vecRef = mergedIndex[term][docID];
                        vecRef.insert(vecRef.end(), positions.begin(), positions.end());
                    }
                }
            }
        }

        in.close();
        cout << "MERGED BLOCK FILE: " << blockFile << "\n";
    }

    // POST-PROCESS: SORT AND DEDUP POSITIONS FOR EACH posting
    for (auto &termEntry : mergedIndex) {
        for (auto &docEntry : termEntry.second) {
            auto &vec = docEntry.second;
            sort(vec.begin(), vec.end());
            vec.erase(unique(vec.begin(), vec.end()), vec.end());
        }
    }

    cout << "MERGE COMPLETED. TOTAL TERMS IN MERGED INDEX: " << mergedIndex.size() << "\n";
    return mergedIndex;
}

// WRITE FINAL pos_inverted_index.json (ONE TERM PER LINE, MATCHING ASSIGNMENT FORMAT)
void writeFinalIndexToFile(const map<string, map<int, vector<int>>> &index, const string &outFilename) {
    ofstream out(outFilename);
    if (!out.is_open()) {
        cerr << "ERROR OPENING FINAL INDEX FILE FOR WRITING: " << outFilename << endl;
        return;
    }

    for (const auto &termEntry : index) {
        const string &term = termEntry.first;
        const auto &postings = termEntry.second;

        json j;
        j[term].push_back((int)postings.size());
        for (const auto &docEntry : postings) {
            int docID = docEntry.first;
            const vector<int> &positions = docEntry.second;
            json postingObj;
            postingObj[to_string(docID)] = positions;
            j[term].push_back(postingObj);
        }

        out << j.dump() << "\n";
    }

    out.close();
    cout << "FINAL INDEX WRITTEN TO: " << outFilename << "\n";
}

// CHECK IF QUERY PHRASE OCCURS SEQUENTIALLY IN DOCUMENT (USING THE IN-MEMORY INDEX)
bool phraseExistsInDoc(const vector<pair<string,int>> &queryTokens,
                       const map<string, map<int, vector<int>>> &index,
                       int docId) {
    if (queryTokens.empty()) return false;
    const string &firstWord = queryTokens[0].first;
    // FIRST WORD MUST EXIST IN DOC
    auto itFirstTerm = index.find(firstWord);
    if (itFirstTerm == index.end()) return false;
    auto itDoc = itFirstTerm->second.find(docId);
    if (itDoc == itFirstTerm->second.end()) return false;
    vector<int> prevPositions = itDoc->second;

    // FOR EACH NEXT WORD, KEEP ONLY POSITIONS THAT ARE +1 OF PREVIOUS
    for (size_t i = 1; i < queryTokens.size(); ++i) {
        const string &currWord = queryTokens[i].first;
        auto itTerm = index.find(currWord);
        if (itTerm == index.end()) return false;
        auto itTermDoc = itTerm->second.find(docId);
        if (itTermDoc == itTerm->second.end()) return false;
        const vector<int> &currPositions = itTermDoc->second;

        vector<int> nextPositions;
        for (int p : prevPositions) {
            if (binary_search(currPositions.begin(), currPositions.end(), p + 1)) {
                nextPositions.push_back(p + 1);
            }
        }

        if (nextPositions.empty()) return false;
        prevPositions = move(nextPositions);
    }

    return true;
}

// UTILITY: LIST SPIMI BLOCK FILES IN CWD
vector<string> findSpimiBlockFiles() {
    vector<string> files;
    for (const auto &entry : fs::directory_iterator(fs::current_path())) {
        if (!entry.is_regular_file()) continue;
        string fname = entry.path().filename().string();
        if (fname.rfind("spimi_block_", 0) == 0 && fname.find(".jsonl") != string::npos) {
            files.push_back(entry.path().string());
        }
    }
    sort(files.begin(), files.end());
    return files;
}

int main() {
    cout << "SPIMI POSITIONAL INVERTED INDEX - STARTING\n";

    // INDEX IN A SINGLE BLOCK (CURRENT BLOCK)
    map<string, map<int, vector<int>>> currentBlock;
    vector<string> blockFiles; // TRACK WRITTEN BLOCK FILES
    int blockCount = 0;

    // DOC ID TO PATH MAPPING
    map<int, string> docIdToPath;
    int docCounter = 1;

    string folderPath = "./docs";

    // READ DOCUMENTS AND BUILD BLOCKS
    cout << "READING DOCUMENTS FROM: " << folderPath << "\n";
    for (const auto &entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;

        string path = entry.path().string();
        docIdToPath[docCounter] = path;

        ifstream inFile(path);
        if (!inFile.is_open()) {
            cerr << "ERROR OPENING DOCUMENT: " << path << "\n";
            docCounter++;
            continue;
        }
        stringstream buffer;
        buffer << inFile.rdbuf();
        string content = buffer.str();
        inFile.close();

        // TOKENIZE WITH POSITIONS
        auto tokens = tokenizeWithPositions(content);
        for (const auto &tp : tokens) {
            const string &term = tp.first;
            int pos = tp.second;
            currentBlock[term][docCounter].push_back(pos);
        }

        // IF BLOCK TERM LIMIT REACHED -> FLUSH BLOCK
        if (currentBlock.size() >= BLOCK_TERM_LIMIT) {
            blockCount++;
            writeBlockToDisk(currentBlock, blockCount);
            blockFiles.push_back("spimi_block_" + to_string(blockCount) + ".jsonl");
            currentBlock.clear();
        }

        docCounter++;
    }

    // FLUSH REMAINING BLOCK
    if (!currentBlock.empty()) {
        blockCount++;
        writeBlockToDisk(currentBlock, blockCount);
        blockFiles.push_back("spimi_block_" + to_string(blockCount) + ".jsonl");
        currentBlock.clear();
    }

    cout << "ALL BLOCKS WRITTEN. NUMBER OF BLOCKS: " << blockFiles.size() << "\n";

    // MERGE BLOCKS
    cout << "MERGING BLOCKS INTO FINAL INDEX (IN MEMORY)\n";
    auto mergedIndex = mergeBlocksToIndex(blockFiles);

    // WRITE FINAL INDEX FILE
    string finalIndexFile = "pos_inverted_index.json";
    writeFinalIndexToFile(mergedIndex, finalIndexFile);

    // WRITE DOCID -> PATH MAPPING CSV
    string csvFileName = "docId_filePath_mapping.csv";
    ofstream csvOut(csvFileName);
    csvOut << "docID,relative_path\n";
    for (const auto &p : docIdToPath) {
        csvOut << p.first << "," << p.second << "\n";
    }
    csvOut.close();
    cout << "DOCID TO FILEPATH MAPPING WRITTEN: " << csvFileName << "\n";

    cout << "INDEXING COMPLETED \n";

    // PHRASE QUERY
    cout << "\nENTER A PHRASE TO SEARCH: ";
    string queryLine;
    getline(cin, queryLine);

    auto queryTokens = tokenizeWithPositions(queryLine);

    if (queryTokens.empty()) {
        cout << "NOTHING? IS THAT WHAT YOU ARE HOPING TO FIND.\n";
        return 0;
    }

    set<int> matchingDocs;
    const string &firstWord = queryTokens[0].first;
    auto it = mergedIndex.find(firstWord);
    if (it != mergedIndex.end()) {
        for (const auto &docPair : it->second) {
            int docId = docPair.first;
            if (phraseExistsInDoc(queryTokens, mergedIndex, docId)) {
                matchingDocs.insert(docId);
            }
        }
    }

    if (matchingDocs.empty()) {
        cout << "NO DOCUMENT FOUND FOR THIS PHRASE.\n";
    } else {
        cout << "\nPHRASE LOCATED IN:\n";
        for (int id : matchingDocs) {
            cout << "- " << docIdToPath[id] << "\n";
        }
    }

    cout << "SPIMI INDEX PROGRAM FINISHED\n";
}

//YAZAN ASHOUR
