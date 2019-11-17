#pragma once

#include "node/Node.h"

namespace capted {

//------------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------------

template<class Data>
class InputParser {
public:
    InputParser() { }
    virtual Node<Data>* getRoot() = 0;
};

} // namespace capted
