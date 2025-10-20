# 🧠 SPIMI Positional Inverted Index

This project implements the **Single-Pass In-Memory Indexing (SPIMI)** algorithm in **C++** to construct a **positional inverted index** from a given collection of text documents.  
The program processes documents, tokenizes text while ignoring stop words and short terms, and generates an index that supports **efficient phrase queries**.

---

## 📚 Project Overview

The system reads all text documents from a specified folder (e.g., `./docs`), tokenizes their contents, and builds the inverted index block by block in memory using the **SPIMI-Invert** algorithm.  
Once memory is full (simulated by a block term limit), each block is written to disk and later merged into a final index file `pos_inverted_index.json`.

The final index file stores each term in the following JSON format:

```json
{"happy": [4, {"1": [156, 985, 658]}, {"66": [30]}, {"90": [775, 89]}, {"27": [15, 120, 128]}]}
```

Where:
- `"happy"` → The term.
- `4` → Number of documents containing the term.
- Each `{docID : [positions]}` pair lists the positions of the term within that document.

---

## 🏗️ Files Generated

| File Name | Description |
|------------|-------------|
| **pos_inverted_index.json** | Final merged positional inverted index (one term per line). |
| **spimi_block_#.jsonl** | Intermediate SPIMI blocks created during indexing. |
| **docId_filePath_mapping.csv** | Mapping between each document ID and its relative file path. |
| **main.cpp** | The main implementation file. |
| **json.hpp** | JSON library used for structured output (nlohmann/json). |

---

## ⚙️ How It Works

### 1️⃣ Tokenization
- Each document is read and split into tokens (words).
- Words are cleaned, converted to lowercase, and filtered:
  - Stop words are ignored.
  - Words shorter than 3 characters are skipped.

### 2️⃣ SPIMI Index Construction
- Terms are added to a **dictionary** in memory.
- When memory (term count) reaches a threshold, the block is written to disk (`spimi_block_#.jsonl`).
- Each posting list stores document IDs and word positions.

### 3️⃣ Merging
- All SPIMI blocks are merged into one global index (`pos_inverted_index.json`).
- Each term appears once, containing all document postings.

### 4️⃣ Phrase Query Search
- After indexing, the user is prompted to enter a phrase.
- The system checks positional adjacency to determine if the phrase exists within documents.
- It returns the relative file paths of matching documents.

---

## 🧩 Example Output

```
INDEXING COMPLETED 
WRITTEN SPIMI BLOCK TO DISK: spimi_block_1.jsonl (TERMS: 97)
WRITTEN SPIMI BLOCK TO DISK: spimi_block_2.jsonl (TERMS: 77)
WRITTEN SPIMI BLOCK TO DISK: spimi_block_3.jsonl (TERMS: 502)
MERGED 3 SPIMI BLOCKS INTO FINAL INDEX: pos_inverted_index.json

ENTER A PHRASE TO SEARCH: happy day
PHRASE LOCATED IN:
- ./docs/doc1.txt
- ./docs/doc2.txt
```

---

## 🧮 Example JSON Entry

```json
{"university": [3, {"1": [5, 32, 76]}, {"4": [10]}, {"5": [27, 45]}]}
```

---

## 🗂️ Folder Structure

```
📁 PositionalInvertedIndex/
│
├── main.cpp
├── json.hpp
├── pos_inverted_index.json
├── docId_filePath_mapping.csv
├── spimi_block_1.jsonl
├── spimi_block_2.jsonl
├── spimi_block_3.jsonl
├── docs/
│   ├── doc1.txt
│   ├── doc2.txt
│   └── ...
└── .vscode/
```

---

## 🚀 How to Run

### 🧱 1. Compile
```bash
g++ main.cpp -o main
```

### ▶️ 2. Run
```bash
./main
```

### 📁 3. Ensure Folder Exists
Make sure you have a folder named `docs/` in the same directory, containing your text files.

---

## 🧠 Features Summary

✅ Implements **SPIMI-Invert Algorithm**  
✅ Creates **Positional Inverted Index**  
✅ Ignores Stop Words & Short Words (< 3 chars)  
✅ Saves **Intermediate SPIMI Blocks**  
✅ Merges Blocks into a Final Index  
✅ Supports **Phrase Query Search**  
✅ Well-Commented and Organized Code  

---

## ✍️ Author
**Yazan Ashour**  
📘 *Information Retrieval — Assignment 1*  
Faculty of Computer Science and Information Systems
