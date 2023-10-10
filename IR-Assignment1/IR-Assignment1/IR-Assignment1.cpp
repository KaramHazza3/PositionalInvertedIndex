#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <list>
#include "Document.cpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

string folderPath = "C:/Users/karam/OneDrive/Desktop/files";
string invertedIndexPath = "C:/Users/karam/OneDrive/Desktop/pos_inverted_index.json";
string newDocPath = "C:/Users/karam/OneDrive/Desktop/new_file.txt";
string csvFilePath = "C:/Users/karam/OneDrive/Desktop/docId_filePath_mapping.csv";

void processDocument(const string& filePath, int docId, unordered_map<string, list<Document*>>& terms) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filePath << endl;
        return;
    }

    string line;
    int wordIndex = 1;

    while (getline(file, line)) {
        stringstream ss(line);
        string word;

        while (ss >> word) {
            if (word != "the" && word.length() >= 3) {
                Document* termDoc = new Document(filePath, docId);

                if (!terms.contains(word)) {
                    termDoc->getPositions().push_back(wordIndex++);
                    terms[word].push_back(termDoc);
                }
                else if (terms[word].back()->getId() == docId) {
                    terms[word].back()->getPositions().push_back(wordIndex++);
                }
                else {
                    termDoc->getPositions().push_back(wordIndex++);
                    terms[word].push_back(termDoc);
                }
            }
        }
    }
    file.close();
}

void createDocIdFilePathCSV(const string& folderPath) {
    try {
        ofstream csvFile(csvFilePath);
        csvFile << "docId, filePath" << endl;

        if (!csvFile.is_open()) {
            cerr << "Error: Failed to open the CSV file." << endl;
            return;
        }

        int docId = 1;
        for (const auto& entry : filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                csvFile << docId << "," << entry.path().string() << endl;
                docId++;
            }
        }
        csvFile.close();
        cout << "CSV file created successfully." << endl;
    }
    catch (const filesystem::filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

void writeJsonToFile(const unordered_map<string, list<Document*>>& terms,
    const string& filePath) {

    json result;

    for (const auto& pair : terms) {
        const string& term = pair.first;
        const list<Document*>& docs = pair.second;

        json termArray;

        termArray.push_back(docs.size());

        json docArray;

        for (Document* doc : docs) {
            json docObject;

            docObject["docId"] = doc->getId();
            docObject["positions"] = doc->getPositions();
            docArray.push_back(docObject);
        }

        termArray.push_back(docArray);

        result[term] = termArray;
    }

    string jsonStr = result.dump(4);

    ofstream jsonFile(filePath);
    jsonFile << jsonStr;
    jsonFile.close();

    cout << "JSON data has been written to " << filePath << endl;
}

void addNewDocument(const string& filePath, const string& invertedIndexPath,const string& csvFilePath, int docId) {
    ifstream doc(filePath);
    ifstream invertedIndex(invertedIndexPath);
    ofstream csvFile(csvFilePath, ios::app);

    if (!csvFile.is_open()) {
        cerr << "Error: Failed to open the CSV file." <<endl;
        return;
    } 

    csvFile << docId << "," << filePath << endl;
    csvFile.close();
    cout << "New line added to the CSV file." << endl;

    json jsonData;
    invertedIndex >> jsonData;

    string line;
    int wordIndex = 1;
    while (getline(doc, line)) {
        stringstream ss(line);
        string word;

        while (ss >> word) {
            if (word != "the" && word.length() >= 3) {
                Document* termDoc = new Document(filePath, docId);
                if (jsonData.contains(word)) {
                    if (jsonData[word].back().back().front() == docId) {
                        jsonData[word].back().back().back().push_back(wordIndex++);
                    }
                    else {
                        termDoc->getPositions().push_back(wordIndex++);
                        json jsonTermDoc = termDoc->toJson();
                        jsonData[word].back().push_back(jsonTermDoc);
                        jsonData[word].front() = jsonData[word].back().size();
                    }
                }
                else {
                    termDoc->getPositions().push_back(wordIndex++);
                    json jsonTermDoc = { 1 ,{termDoc->toJson()} };
                    jsonData[word] = jsonTermDoc;
                }

            }
        }
    }

    ofstream outputFile(invertedIndexPath);
    outputFile << setw(4) << jsonData << endl;
    outputFile.close();
    cout << "The inverted index has been updated" << endl;

}

void deleteDocument(int docId, const string& invertedIndexPath, const string& csvFilePath) {

    ifstream invertedIndexFile(invertedIndexPath);
    if (!invertedIndexFile.is_open()) {
        cerr << "Error: Failed to open the JSON file." << endl;
        return;
    }

    json invertedIndexData;
    invertedIndexFile >> invertedIndexData;
    invertedIndexFile.close();

    for (json::iterator it = invertedIndexData.begin(); it != invertedIndexData.end(); ++it) {
        json& termData = *it;
        json& docArray = termData[1];

        docArray.erase(remove_if(docArray.begin(), docArray.end(),
            [docId](const json& x) {return x["docId"] == docId; }), docArray.end());

        termData[0] = docArray.size();
    }
    ofstream updatedInvertedIndexFile(invertedIndexPath);
    updatedInvertedIndexFile << setw(4) << invertedIndexData << endl;
    updatedInvertedIndexFile.close();

    ifstream inputFile(csvFilePath);

    if (!inputFile.is_open()) {
        cerr << "Error: Failed to open the CSV file." << endl;
        return;
    }

    vector<string> lines;
    string line;
    while (getline(inputFile, line)) {
        lines.push_back(line);
    }

    inputFile.close();

    if (docId < 1 || docId > lines.size()) {
        cerr << "Error: Invalid line number to delete." << endl;
        return;
    }

    lines.erase(lines.begin() + docId);

    ofstream outputFile(csvFilePath);
    if (!outputFile.is_open()) {
        cerr << "Error: Failed to open the CSV file for writing." << endl;
        return;
    }

    for (const string& updatedLine : lines) {
        outputFile << updatedLine << std::endl;
    }

    outputFile.close();

    cout << "Line " << docId << " has been deleted from the CSV file." << endl;

}

vector<int> findCommonDocuments(const json& invertedIndexData, const string& word1, const string& word2) {
    vector<int> commonDocuments;

    if (invertedIndexData.find(word1) != invertedIndexData.end() && invertedIndexData.find(word2) != invertedIndexData.end()) {

        const json& positions1 = invertedIndexData[word1].back();
        const json& positions2 = invertedIndexData[word2].back();

        for (const json& pos1 : positions1) {
            for (const json& pos2 : positions2) {
                if (pos1["docId"] == pos2["docId"]) {
                    const json& positions1Array = pos1["positions"];
                    const json& positions2Array = pos2["positions"];

                    for (int pos1_val : positions1Array) {
                        for (int pos2_val : positions2Array) {
                            if (pos2_val == (pos1_val + 1)) {
                                commonDocuments.push_back(pos1["docId"]);
                                break;
                            }
                        }
                    }
                }
            }
        }
        return commonDocuments;
    }
}

vector<int> queryResult(const vector<string>& userInput, const string& invertedIndexPath) {
    ifstream invertedIndexFile(invertedIndexPath);
    vector<int> res;

    if (!invertedIndexFile.is_open()) {
        cerr << "Error: Failed to open the JSON file." << endl;
        return res;
    }

    json invertedIndexData;
    invertedIndexFile >> invertedIndexData;
    invertedIndexFile.close();
    
     string word1;
     for (int i = 0; i < userInput.size(); i++) {
         
         if (word1.size()==0) {
             word1 = userInput[i];
             if (invertedIndexData.find(word1) != invertedIndexData.end()) {
                 const json& docs = invertedIndexData[word1].back();
                 for (const json& doc : docs) {
                     res.push_back(doc["docId"]);
                 }
             }
         }
         else {

             const string& word2 = userInput[i];

             if (invertedIndexData.find(word1) != invertedIndexData.end() && invertedIndexData.find(word2) != invertedIndexData.end()) {
                 vector<int> commonDocuments = findCommonDocuments(invertedIndexData, word1, word2);
                 word1 = word2;

                 vector<int> tempRes;
                 set_intersection(res.begin(), res.end(), commonDocuments.begin(), commonDocuments.end(), back_inserter(tempRes));
                 res = tempRes;

             }
             else {
                 cerr << "Word(s) not found in the inverted index: " << word1 << " or " << word2 << endl;
                 return res;
             }
         }
     }
    return res;
}





int main() {
    unordered_map<string, list<Document*>> terms;

    createDocIdFilePathCSV(folderPath);

    int docId = 1;
    for (const auto& entry : filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            processDocument(entry.path().string(), docId, terms);
            docId++;
        }
    }
    writeJsonToFile(terms, invertedIndexPath);
    addNewDocument(newDocPath, invertedIndexPath, csvFilePath, docId);
    deleteDocument(3, invertedIndexPath, csvFilePath);

    string input;
    cout << "Enter your query: ";
    getline(cin, input);

    istringstream iss(input);
    vector<string> tokens;
    string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    vector<int> results = queryResult(tokens, invertedIndexPath);

    for (int i = 0; i < results.size(); ++i) {
        cout << results[i] << " ";
    }

    return 0;
}
