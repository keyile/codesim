#pragma once

#include "node/Node.h"

namespace capted {

//------------------------------------------------------------------------------
// Cost Model
//------------------------------------------------------------------------------

template<class Data>
class CostModel {
public:
    virtual float deleteCost(Node<Data>* n) const = 0;
    virtual float insertCost(Node<Data>* n) const = 0;
    virtual float renameCost(Node<Data>* n1, Node<Data>* n2) const = 0;
};

} // namespace capted
