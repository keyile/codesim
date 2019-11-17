#include "debug.h"

//------------------------------------------------------------------------------
// arrayToString bool
//------------------------------------------------------------------------------

template<>
std::string capted::arrayToString(std::vector<bool> &array) {
    std::stringstream ss;
    ss << "[";

    for (size_t i = 0; i < array.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }

        if (array[i]) {
            ss << "true";
        } else {
            ss << "false";
        }
    }

    ss << "]";
    return ss.str();
}

//------------------------------------------------------------------------------
// arrayToString nested
//------------------------------------------------------------------------------

template<typename T>
std::string capted::arrayToString(std::vector<typename std::vector<T>> &nestedArray) {
    std::stringstream ss;
    ss << "[";

    for (size_t i = 0; i < nestedArray.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }

        ss << arrayToString(nestedArray[i]);
    }

    ss << "]";
    return ss.str();
}

//------------------------------------------------------------------------------
// arrayToString StringNodeData
//------------------------------------------------------------------------------

template<>
std::string capted::arrayToString(std::vector<Node<StringNodeData>*> &array) {
    std::stringstream ss;
    ss << "[";

    for (size_t i = 0; i < array.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }

        ss << *array[i];
    }

    ss << "]";
    return ss.str();
}

//------------------------------------------------------------------------------
// arrayToString std::pair
//------------------------------------------------------------------------------

template<typename T1, typename T2>
std::string capted::arrayToString(std::vector<std::pair<T1, T2>> &mappings) {
    std::stringstream ss;
    ss << "[";

    for (size_t i = 0; i < mappings.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }

        ss << mappings[i].first << ":" << mappings[i].second;
    }

    ss << "]" << std::endl;
    return ss.str();
}
