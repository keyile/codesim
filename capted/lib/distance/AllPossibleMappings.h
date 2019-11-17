#pragma once

#include "TreeEditDistance.h"
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static inline bool removeMappingElement(std::vector<IntPair> &mapping, IntPair pair) {
    auto iter = std::remove_if(mapping.begin(), mapping.end(), [&pair](IntPair p) -> bool {
        return (p.first  == pair.first &&
                p.second == pair.second);
    });

    bool removedSomething = (iter != mapping.end());
    mapping.erase(iter, mapping.end());

    return removedSomething;
}

//------------------------------------------------------------------------------
// Distance Algorithm (simple - exponential cost)
//------------------------------------------------------------------------------

template<class Data>
class AllPossibleMappings : public TreeEditDistance<Data> {
private:
    std::vector<std::vector<IntPair>> generateAllOneToOneMappings() {
        // Start with an empty mapping - all nodes are deleted or inserted.
        std::vector<std::vector<IntPair>> mappings;
        mappings.push_back(std::vector<IntPair>());
        mappings[0].reserve(this->size1 + this->size2);

        // Add all deleted nodes.
        for (Integer n1 = 0; n1 < this->size1; n1++) {
            mappings[0].push_back(std::make_pair(n1, -1));
        }
        // Add all inserted nodes.
        for (Integer n2 = 0; n2 < this->size2; n2++) {
            mappings[0].push_back(std::make_pair(-1, n2));
        }

        // For each node in the source tree.
        for (Integer n1 = 0; n1 < this->size1; n1++) {
            // Duplicate all mappings and store in mappings_copy.
            std::vector<std::vector<IntPair>> mappings_copy = mappings;
            // For each node in the destination tree.
            for (Integer n2 = 0; n2 < this->size2; n2++) {
                // For each mapping (produced for all n1 values smaller than
                // current n1).
                for (std::vector<IntPair> m : mappings_copy) {
                    // Produce new mappings with the pair (n1, n2) by adding this
                    // pair to all mappings where it is valid to add.
                    bool element_add = true;
                    // Verify if (n1, n2) can be added to mapping m.
                    // All elements in m are checked with (n1, n2) for possible
                    // violation.
                    // One-to-one condition.
                    for (IntPair e : m) {
                        // n1 is not in any of previous mappings
                        if (e.first != -1 && e.second != -1 && e.second == n2) {
                            element_add = false;
                            break;
                        }
                    }
                    // New mappings must be produced by duplicating a previous
                    // mapping and extending it by (n1, n2).
                    if (element_add) {
                        std::vector<IntPair> m_copy = m;
                        m_copy.push_back(std::make_pair(n1, n2));
                        // If a pair (n1,n2) is added, (n1,-1) and (-1,n2) must be removed.
                        assert(removeMappingElement(m_copy, std::make_pair(n1, -1)));
                        assert(removeMappingElement(m_copy, std::make_pair(-1, n2)));
                        mappings.push_back(m_copy);
                    }
                }
            }
        }
        return mappings;
    }

    bool isTEDMapping(std::vector<IntPair> &mapping) const {
        // Validate each pair of pairs of mapped nodes in the mapping.
        for (IntPair e1 : mapping) {
            // Use only pairs of mapped nodes for validation.
            if (e1.first == -1 || e1.second == -1) {
                continue;
            }
            for (IntPair e2 : mapping) {
                // Use only pairs of mapped nodes for validation.
                if (e2.first == -1 || e2.second == -1) {
                    continue;
                }
                // If any of the conditions below doesn't hold, discard m.
                // Validate ancestor-descendant condition.
                bool a = e1.first  < e2.first  && this->it1->preL_to_preR[e1.first]  < this->it1->preL_to_preR[e2.first];
                bool b = e1.second < e2.second && this->it2->preL_to_preR[e1.second] < this->it2->preL_to_preR[e2.second];
                if ((a && !b) || (!a && b)) {
                    // Discard the mapping.
                    // If this condition doesn't hold, the next condition
                    // doesn't have to be verified any more and any other
                    // pair (e1, e2) doesn't have to be verified any more.
                    return false;
                }
                // Validate sibling-order condition.
                a = e1.first  < e2.first  && this->it1->preL_to_preR[e1.first]  > this->it1->preL_to_preR[e2.first];
                b = e1.second < e2.second && this->it2->preL_to_preR[e1.second] > this->it2->preL_to_preR[e2.second];
                if ((a && !b) || (!a && b)) {
                    // Discard the mapping.
                    return false;
                }
            }
        }
        return true;
    }

    void removeNonTEDMappings(std::vector<std::vector<IntPair>> &mappings) {
        mappings.erase(
            std::remove_if(mappings.begin(), mappings.end(), [&](std::vector<IntPair> &mapping) -> bool {
                return !isTEDMapping(mapping);
            }),
            mappings.end()
        );
    }

    float getMinCost(std::vector<std::vector<IntPair>> mappings) {
        // Initialize min_cost to the upper bound.
        float min_cost = this->size1 + this->size2;

        // Verify cost of each mapping.
        for (std::vector<IntPair> m : mappings) {
            float m_cost = 0;
            // Sum up edit costs for all elements in the mapping m.
            for (IntPair e : m) {
                // Add edit operation cost.
                if (e.first > -1 && e.second > -1) {
                    m_cost += this->costModel->renameCost(this->it1->preL_to_node[e.first], this->it2->preL_to_node[e.second]); // USE COST MODEL - rename e.first to e.second.
                } else if (e.first > -1) {
                    m_cost += this->costModel->deleteCost(this->it1->preL_to_node[e.first]); // USE COST MODEL - insert e.second.
                } else {
                    m_cost += this->costModel->insertCost(this->it2->preL_to_node[e.second]); // USE COST MODEL - delete e.first.
                }
                // Break as soon as the current min_cost is exceeded.
                // Only for early loop break.
                if (m_cost >= min_cost) {
                    break;
                }
            }
            // Store the minimal cost - compare m_cost and min_cost
            if (m_cost < min_cost) {
                min_cost = m_cost;
            }
        }
        return min_cost;
    }

public:
    AllPossibleMappings(CostModel<Data>* costModel) : TreeEditDistance<Data>(costModel) {
        // nop
    }

    virtual float computeEditDistance(Node<Data>* t1, Node<Data>* t2) override {
        this->init(t1, t2);
        std::vector<std::vector<IntPair>> mappings = generateAllOneToOneMappings();
        removeNonTEDMappings(mappings);
        return getMinCost(mappings);
    }
};

} // namespace capted
