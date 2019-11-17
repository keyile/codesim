#pragma once

#include "CostModel.h"
#include "node/NodeIndexer.h"
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// Distance Algorithm
//------------------------------------------------------------------------------

typedef std::pair<Integer, Integer> IntPair;

template<class Data>
class TreeEditDistance {
protected:
    NodeIndexer<Data>* it1;
    NodeIndexer<Data>* it2;
    Integer size1;
    Integer size2;
    const CostModel<Data>* costModel;

    void init(Node<Data>* t1, Node<Data>* t2) {
        it1 = new NodeIndexer<Data>(t1, costModel);
        it2 = new NodeIndexer<Data>(t2, costModel);
        size1 = it1->getSize();
        size2 = it2->getSize();
    }

public:
    TreeEditDistance(CostModel<Data>* costModel) : costModel(costModel) {
        it1 = nullptr;
        it2 = nullptr;
        size1 = -1;
        size2 = -1;
    }

    ~TreeEditDistance() {
        delete it1;
        delete it2;
    }

    virtual float computeEditDistance(Node<Data>* t1, Node<Data>* t2) = 0;
};

} // namespace capted
