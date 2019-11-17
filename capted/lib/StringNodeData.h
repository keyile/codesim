#pragma once

#include <cassert>
#include <iostream>
#include <sstream>
#include "InputParser.h"
#include "CostModel.h"
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// String Node Data
//------------------------------------------------------------------------------

class StringNodeData {
private:
    std::string label;

public:
    friend std::ostream &operator<<(std::ostream &os, StringNodeData const &stringNode);

    StringNodeData(std::string label) : label(label) { }
    std::string getLabel() const { return label; }
};

inline std::ostream &operator<<(std::ostream &os, StringNodeData const &stringNode) {
    os << stringNode.getLabel();
    return os;
}

inline std::ostream &operator<<(std::ostream &os, Node<StringNodeData> const &node) {
    os << "{";
    os << *node.getData();
    for (Node<StringNodeData>* child : node.getChildren()) {
        os << *child;
    }
    os << "}";
    return os;
}

//------------------------------------------------------------------------------
// String Node Data Parser
//------------------------------------------------------------------------------

class BracketStringInputParser : public InputParser<StringNodeData> {
private:
    const std::string inputString;

    static std::string getRootLabel(std::string s) {
        // Find where my children starts, after my own opening brace
        std::size_t start = 1;
        std::size_t end = s.find_first_of('{', 1);

        // If I don't have children, my label ends at my own closing brace
        if (end == std::string::npos) {
            end = s.find_first_of('}', 1);
            assert(end != std::string::npos);
        }

        // -2 to exclude my opening/closing brace
        // +1 because 0-based counting
        return s.substr(start, (end - 2 + 1));
    }

    static std::vector<std::string> getChildrenString(std::string s) {
        std::vector<std::string> children;

        // Check if I have children
        std::size_t childrenStart = s.find_first_of('{', 1);
        if (childrenStart == std::string::npos) {
            return children;
        }

        Integer depth = 0;
        std::stringstream currentChild;
        for (size_t i = childrenStart; i < s.size(); i++) {
            switch (s[i]) {
                case '{': depth++; break;
                case '}': depth--; break;
            }

            if (depth == -1) {
                break;
            }

            currentChild << s[i];

            if (depth == 0) {
                children.push_back(currentChild.str());
                currentChild.str(std::string());
            }            
        }

        return children;
    }

public:
    BracketStringInputParser(std::string inputString) : inputString(inputString) {
        // nop
    }

    virtual Node<StringNodeData>* getRoot() override {
        std::string rootLabel = getRootLabel(inputString);
        std::vector<std::string> childrenString = getChildrenString(inputString);

        Node<StringNodeData>* node = new Node<StringNodeData>(new StringNodeData(rootLabel));
        for (std::string childString : childrenString) {
            BracketStringInputParser parser(childString);
            node->addChild(parser.getRoot());
        }

        return node;
    }
};

//------------------------------------------------------------------------------
// String Node Data Cost Model
//------------------------------------------------------------------------------

class StringCostModel : public CostModel<StringNodeData> {
public:
    virtual float deleteCost(Node<StringNodeData>* n) const override {
        return 1.0f;
    }

    virtual float insertCost(Node<StringNodeData>* n) const override {
        return 1.0f;
    }

    virtual float renameCost(Node<StringNodeData>* n1, Node<StringNodeData>* n2) const override {
        return (n1->getData()->getLabel() == n2->getData()->getLabel()) ? 0.0f : 1.0f;
    }
};

} // namespace capted
