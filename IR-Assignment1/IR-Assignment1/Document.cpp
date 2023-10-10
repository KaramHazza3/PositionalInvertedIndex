#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

class Document {
private:
    string path;
    int id;
    vector<int> positions;

public:
    Document(string path, int id) {
        this->path = path;
        this->id = id;
    }

        vector<int>& getPositions() {
        return positions;
        }
        string getPath() {
        return path;
        }
        int getId() {
            return id;
        }
        void setPath(string path) {
            this->path = path;
        }
        void setId(int id) {
            this->id = id;
        }


        void print() const {
            cout << "Document ID: " << id << ", Path: " << path << ", Positions: {";
            for (size_t i = 0; i < positions.size(); i++) {
                cout << positions[i];
                if (i < positions.size() - 1) {
                    cout << ", ";
                }
            }
            cout << "}" << endl;
        }

        json toJson() const {
            json j;
            j["docId"] = id;
            j["positions"] = positions;
            return j;
        }

};

