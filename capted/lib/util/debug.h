#pragma once

#include "StringNodeData.h"
#include "InputParser.h"
#include <iostream>

namespace capted {

//------------------------------------------------------------------------------
// arrayToString
//------------------------------------------------------------------------------

template<typename T>
std::string arrayToString(std::vector<T> &array) {
    std::stringstream ss;
    ss << "[";

    for (size_t i = 0; i < array.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }

        ss << array[i];
    }

    ss << "]";
    return ss.str();
}

//------------------------------------------------------------------------------
// arrayToString bool
//------------------------------------------------------------------------------

template<>
std::string arrayToString(std::vector<bool> &array);

//------------------------------------------------------------------------------
// arrayToString nested
//------------------------------------------------------------------------------

template<typename T>
std::string arrayToString(std::vector<typename std::vector<T>> &nestedArray);

//------------------------------------------------------------------------------
// arrayToString StringNodeData
//------------------------------------------------------------------------------

template<>
std::string arrayToString(std::vector<Node<StringNodeData>*> &array);

//------------------------------------------------------------------------------
// arrayToString std::pair
//------------------------------------------------------------------------------

template<typename T1, typename T2>
std::string arrayToString(std::vector<std::pair<T1, T2>> &mappings);

} // namespace capted
