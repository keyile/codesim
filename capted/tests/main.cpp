#include <iostream>
#include <iomanip>
#include <fstream>
#include "includes/json.hpp"
#include "Capted.h"

using namespace capted;
using std::cout;
using std::endl;
using std::string;
using json = nlohmann::json;

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

void testEditDistance() {
    std::ifstream testFile("./tests/correctness_test_cases.json");
    json testCases;
    testFile >> testCases;

    std::vector<int> testsToRun = {};
    std::vector<int> testsToSkip = {}; // {64, 71} are really slow without APTED algorithm

    for (json test : testCases) {
        int id = test["testID"];
        float realDist = test["d"];
        string t1 = test["t1"];
        string t2 = test["t2"];

        if (std::find(testsToSkip.begin(), testsToSkip.end(), id) != testsToSkip.end()) {
            // Skip slow tests
            continue;
        }

        if (testsToRun.size() > 0 && std::find(testsToRun.begin(), testsToRun.end(), id) == testsToRun.end()) {
            // Run specific tests?
            continue;
        }

        StringCostModel costModel;
        Apted<StringNodeData> algorithm(&costModel);
        BracketStringInputParser p1(t1);
        BracketStringInputParser p2(t2);
        Node<StringNodeData>* n1 = p1.getRoot();
        Node<StringNodeData>* n2 = p2.getRoot();

        float compDist = algorithm.computeEditDistance(n1, n2);
        cout << std::setw(3) << id << " " << (realDist == compDist ? "✓" : "FAIL") << endl;

        delete n1;
        delete n2;
    }
}

void testLargeEditDistance() {
    std::ifstream testFile("./tests/large_test_case.json");
    json testCases;
    testFile >> testCases;

    for (json test : testCases) {
        int id = test["testID"];
        string t1 = test["t1"];
        string t2 = test["t2"];

        StringCostModel costModel;
        Apted<StringNodeData> algorithm(&costModel);
        BracketStringInputParser p1(t1);
        BracketStringInputParser p2(t2);
        Node<StringNodeData>* n1 = p1.getRoot();
        Node<StringNodeData>* n2 = p2.getRoot();

        float compDist = algorithm.computeEditDistance(n1, n2);
        cout << std::setw(3) << id << " " << compDist << endl;

        delete n1;
        delete n2;
    }
}

int main(int argc, char const *argv[]) {
    #ifdef CAPTED_LARGE_TREES
    testLargeEditDistance();
    #else
    testEditDistance();
    #endif
}
