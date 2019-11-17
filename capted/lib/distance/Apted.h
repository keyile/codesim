#pragma once

#include <cmath>
#include <vector>
#include <stack>
#include "TreeEditDistance.h"
#include "util/debug.h"
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

template<typename T>
static inline void fillArray(std::vector<T> &array, T val) {
    for (size_t i = 0; i < array.size(); i++) {
        array[i] = val;
    }
}

template<typename T>
static inline T signum(T val) {
    if (val < 0) {
        return (T) -1;
    } else if (val > 0) {
        return (T) 1;
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------------
// Distance Algorithm (apted)
//------------------------------------------------------------------------------

template<class Data>
class Apted : public TreeEditDistance<Data> {
private:
    static const Integer LEFT = 0;
    static const Integer RIGHT = 1;
    static const Integer INNER = 2;

    std::vector<std::vector<float>> delta;

    std::vector<float> q;
    std::vector<Integer> fn;
    std::vector<Integer> ft;
    long counter = 0;

    void updateFnArray(Integer lnForNode, Integer node, Integer currentSubtreePreL) {
        if (lnForNode >= currentSubtreePreL) {
            fn[node] = fn[lnForNode];
            fn[lnForNode] = node;
        } else {
            fn[node] = fn[fn.size() - 1];
            fn[fn.size() - 1] = node;
        }
    }

    void updateFtArray(Integer lnForNode, Integer node) {
        ft[node] = lnForNode;
        if(fn[node] > -1) {
            ft[fn[node]] = node;
        }
    }

    Integer getStrategyPathType(Integer pathIDWithPathIDOffset, Integer pathIDOffset, NodeIndexer<Data>* it, Integer currentRootNodePreL, Integer currentSubtreeSize) {
        if (signum(pathIDWithPathIDOffset) == -1) {
            return LEFT;
        }
        Integer pathID = std::abs(pathIDWithPathIDOffset) - 1;
        if (pathID >= pathIDOffset) {
            pathID = pathID - pathIDOffset;
        }
        if (pathID == (currentRootNodePreL + currentSubtreeSize) - 1) {
            return RIGHT;
        }
        return INNER;
    }

    //--------------------------------------------------------------------------

    float spfA(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2, Integer pathID, Integer pathType, bool treesSwapped) {
        std::vector<Node<Data>*> &it2nodes = it2->preL_to_node;
        Node<Data>* lFNode;
        std::vector<Integer> &it1sizes = it1->sizes;
        std::vector<Integer> &it2sizes = it2->sizes;
        std::vector<Integer> &it1parents = it1->parents;
        std::vector<Integer> &it2parents = it2->parents;
        std::vector<Integer> &it1preL_to_preR = it1->preL_to_preR;
        std::vector<Integer> &it2preL_to_preR = it2->preL_to_preR;
        std::vector<Integer> &it1preR_to_preL = it1->preR_to_preL;
        std::vector<Integer> &it2preR_to_preL = it2->preR_to_preL;
        Integer currentSubtreePreL1 = it1->getCurrentNode();
        Integer currentSubtreePreL2 = it2->getCurrentNode();

        // Variables to incrementally sum up the forest sizes.
        Integer currentForestSize1 = 0;
        Integer currentForestSize2 = 0;
        Integer tmpForestSize1 = 0;

        // Variables to incrementally sum up the forest cost.
        float currentForestCost1 = 0;
        float currentForestCost2 = 0;
        float tmpForestCost1 = 0;

        Integer subtreeSize2 = it2->sizes[currentSubtreePreL2];
        Integer subtreeSize1 = it1->sizes[currentSubtreePreL1];
        std::vector<std::vector<float>> t(subtreeSize2 + 1);
        for (size_t i = 0; i < t.size(); i++) {
            t[i].resize(subtreeSize2 + 1);
        }
        std::vector<std::vector<float>> s(subtreeSize1 + 1);
        for (size_t i = 0; i < s.size(); i++) {
            s[i].resize(subtreeSize2 + 1);
        }

        float minCost = -1;

        // sp1, sp2 and sp3 correspond to three elements of the minimum in the
        // recursive formula [1, Figure 12].
        float sp1 = 0;
        float sp2 = 0;
        float sp3 = 0;
        Integer startPathNode = -1;
        Integer endPathNode = pathID;
        Integer it1PreLoff = endPathNode;
        Integer it2PreLoff = currentSubtreePreL2;
        Integer it1PreRoff = it1preL_to_preR[endPathNode];
        Integer it2PreRoff = it2preL_to_preR[it2PreLoff];
        // variable declarations which were inside the loops
        Integer rFlast,lFlast,endPathNode_in_preR,startPathNode_in_preR,parent_of_endPathNode,parent_of_endPathNode_in_preR,
        lFfirst,rFfirst,rGlast,rGfirst,lGfirst,rG_in_preL,rGminus1_in_preL,parent_of_rG_in_preL,lGlast,lF_in_preR,lFSubtreeSize,
        lGminus1_in_preR,parent_of_lG,parent_of_lG_in_preR,rF_in_preL,rFSubtreeSize,
        rGfirst_in_preL;

        bool leftPart,rightPart,fForestIsTree,lFIsConsecutiveNodeOfCurrentPathNode,lFIsLeftSiblingOfCurrentPathNode,
        rFIsConsecutiveNodeOfCurrentPathNode,rFIsRightSiblingOfCurrentPathNode;
        std::vector<float>* sp1spointer;
        std::vector<float>* sp2spointer;
        std::vector<float>* sp3spointer;
        std::vector<float>* sp3deltapointer;
        std::vector<float>* swritepointer;
        std::vector<float>* sp1tpointer;
        std::vector<float>* sp3tpointer;

        // These variables store the id of the source (which array) of looking up
        // elements of the minimum in the recursive formula [1, Figures 12,13].
        Integer sp1source,sp3source;

        // Loop A [1, Algorithm 3] - walk up the path.
        while (endPathNode >= currentSubtreePreL1) {
            it1PreLoff = endPathNode;
            it1PreRoff = it1preL_to_preR[endPathNode];
            rFlast = -1;
            lFlast = -1;
            endPathNode_in_preR = it1preL_to_preR[endPathNode];
            startPathNode_in_preR = startPathNode == -1 ? 0x7fffffff : it1preL_to_preR[startPathNode];
            parent_of_endPathNode = it1parents[endPathNode];
            parent_of_endPathNode_in_preR = parent_of_endPathNode == -1 ? 0x7fffffff : it1preL_to_preR[parent_of_endPathNode];

            if (startPathNode - endPathNode > 1) {
                leftPart = true;
            } else {
                leftPart = false;
            }

            if (startPathNode >= 0 && startPathNode_in_preR - endPathNode_in_preR > 1) {
                rightPart = true;
            } else {
                rightPart = false;
            }

            // Deal with nodes to the left of the path.
            if (pathType == 1 || (pathType == 2 && leftPart)) {
                if (startPathNode == -1) {
                    rFfirst = endPathNode_in_preR;
                    lFfirst = endPathNode;
                } else {
                    rFfirst = startPathNode_in_preR;
                    lFfirst = startPathNode - 1;
                }

                if (!rightPart) {
                    rFlast = endPathNode_in_preR;
                }

                rGlast = it2preL_to_preR[currentSubtreePreL2];
                rGfirst = (rGlast + subtreeSize2) - 1;
                lFlast = rightPart ? endPathNode + 1 : endPathNode;
                fn[fn.size() - 1] = -1;

                for (Integer i = currentSubtreePreL2; i < currentSubtreePreL2 + subtreeSize2; i++) {
                    fn[i] = -1;
                    ft[i] = -1;
                }

                // Store the current size and cost of forest in F.
                tmpForestSize1 = currentForestSize1;
                tmpForestCost1 = currentForestCost1;

                // Loop B [1, Algoritm 3] - for all nodes in G (right-hand input tree).
                for (Integer rG = rGfirst; rG >= rGlast; rG--) {
                    lGfirst = it2preR_to_preL[rG];
                    rG_in_preL = it2preR_to_preL[rG];
                    rGminus1_in_preL = rG <= it2preL_to_preR[currentSubtreePreL2] ? 0x7fffffff : it2preR_to_preL[rG - 1];
                    parent_of_rG_in_preL = it2parents[rG_in_preL];
                    // This if statement decides on the last lG node for Loop D [1, Algorithm 3];
                    if (pathType == 1) {
                        if (lGfirst == currentSubtreePreL2 || rGminus1_in_preL != parent_of_rG_in_preL) {
                            lGlast = lGfirst;
                        } else {
                            lGlast = it2parents[lGfirst]+1;
                        }
                    } else {
                        lGlast = lGfirst == currentSubtreePreL2 ? lGfirst : currentSubtreePreL2+1;
                    }

                    updateFnArray(it2->preL_to_ln[lGfirst], lGfirst, currentSubtreePreL2);
                    updateFtArray(it2->preL_to_ln[lGfirst], lGfirst);
                    Integer rF = rFfirst;

                    // Reset size and cost of the forest in F.
                    currentForestSize1 = tmpForestSize1;
                    currentForestCost1 = tmpForestCost1;

                    // Loop C [1, Algorithm 3] - for all nodes to the left of the path node.
                    for (Integer lF = lFfirst; lF >= lFlast; lF--) {
                        // This if statement fixes rF node.
                        if (lF == lFlast && !rightPart) {
                            rF = rFlast;
                        }

                        lFNode = it1->preL_to_node[lF];
                        // Increment size and cost of F forest by node lF.
                        currentForestSize1++;
                        currentForestCost1 += (treesSwapped ? this->costModel->insertCost(lFNode) : this->costModel->deleteCost(lFNode)); // USE COST MODEL - sum up deletion cost of a forest.
                        // Reset size and cost of forest in G to subtree G_lGfirst.
                        currentForestSize2 = it2sizes[lGfirst];
                        currentForestCost2 = (treesSwapped ? it2->preL_to_sumDelCost[lGfirst] : it2->preL_to_sumInsCost[lGfirst]); // USE COST MODEL - reset to subtree insertion cost.
                        lF_in_preR = it1preL_to_preR[lF];
                        fForestIsTree = lF_in_preR == rF;
                        lFSubtreeSize = it1sizes[lF];
                        lFIsConsecutiveNodeOfCurrentPathNode = startPathNode - lF == 1;
                        lFIsLeftSiblingOfCurrentPathNode = lF + lFSubtreeSize == startPathNode;
                        sp1spointer = &(s[(lF + 1) - it1PreLoff]);
                        sp2spointer = &(s[lF - it1PreLoff]);
                        sp3spointer = &(s[0]);
                        sp3deltapointer = treesSwapped ? nullptr : &(delta[lF]);
                        swritepointer = &(s[lF - it1PreLoff]);
                        sp1source = 1; // Search sp1 value in s array by default.
                        sp3source = 1; // Search second part of sp3 value in s array by default.

                        if (fForestIsTree) { // F_{lF,rF} is a tree.
                            if (lFSubtreeSize == 1) { // F_{lF,rF} is a single node.
                                sp1source = 3;
                            } else if (lFIsConsecutiveNodeOfCurrentPathNode) { // F_{lF,rF}-lF is the path node subtree.
                                sp1source = 2;
                            }
                            sp3 = 0;
                            sp3source = 2;
                        } else {
                            if (lFIsConsecutiveNodeOfCurrentPathNode) {
                                sp1source = 2;
                            }

                            sp3 = currentForestCost1 - (treesSwapped ? it1->preL_to_sumInsCost[lF] : it1->preL_to_sumDelCost[lF]); // USE COST MODEL - Delete F_{lF,rF}-F_lF.

                            if (lFIsLeftSiblingOfCurrentPathNode) {
                                sp3source = 3;
                            }
                        }

                        if (sp3source == 1) {
                            sp3spointer = &(s[(lF + lFSubtreeSize) - it1PreLoff]);
                        }

                        // Go to first lG.
                        Integer lG = lGfirst;

                        // currentForestSize2++;
                        // sp1, sp2, sp3 -- Done here for the first node in Loop D. It differs for consecutive nodes.
                        // sp1 -- START
                        switch(sp1source) {
                            case 1: sp1 = (*sp1spointer)[lG - it2PreLoff]; break;
                            case 2: sp1 = t[lG - it2PreLoff][rG - it2PreRoff]; break;
                            case 3: sp1 = currentForestCost2; break; // USE COST MODEL - Insert G_{lG,rG}.
                        }
                        sp1 += (treesSwapped ? this->costModel->insertCost(lFNode) : this->costModel->deleteCost(lFNode));// USE COST MODEL - Delete lF, leftmost root node in F_{lF,rF}.
                        // sp1 -- END
                        minCost = sp1; // Start with sp1 as minimal value.

                        // sp2 -- START
                        if (currentForestSize2 == 1) { // G_{lG,rG} is a single node.
                            sp2 = currentForestCost1; // USE COST MODEL - Delete F_{lF,rF}.
                        } else { // G_{lG,rG} is a tree.
                            sp2 = q[lF];
                        }
                        sp2 += (treesSwapped ? this->costModel->deleteCost(it2nodes[lG]) : this->costModel->insertCost(it2nodes[lG]));// USE COST MODEL - Insert lG, leftmost root node in G_{lG,rG}.
                        if (sp2 < minCost) { // Check if sp2 is minimal value.
                            minCost = sp2;
                        }
                        // sp2 -- END

                        // sp3 -- START
                        if (sp3 < minCost) {
                            sp3 += treesSwapped ? delta[lG][lF] : (*sp3deltapointer)[lG];
                            if (sp3 < minCost) {
                                sp3 += (treesSwapped ? this->costModel->renameCost(it2nodes[lG], lFNode) : this->costModel->renameCost(lFNode, it2nodes[lG])); // USE COST MODEL - Rename the leftmost root nodes in F_{lF,rF} and G_{lG,rG}.
                                if(sp3 < minCost) {
                                    minCost = sp3;
                                }
                            }
                        }
                        // sp3 -- END

                        (*swritepointer)[lG - it2PreLoff] = minCost;

                        // Go to next lG.
                        lG = ft[lG];
                        counter++;

                        // Loop D [1, Algorithm 3] - for all nodes to the left of rG.
                        while (lG >= lGlast) {
                            // Increment size and cost of G forest by node lG.
                            currentForestSize2++;
                            currentForestCost2 += (treesSwapped ? this->costModel->deleteCost(it2nodes[lG]) : this->costModel->insertCost(it2nodes[lG]));
                            switch(sp1source) {
                                case 1: sp1 = (*sp1spointer)[lG - it2PreLoff] + (treesSwapped ? this->costModel->insertCost(lFNode) : this->costModel->deleteCost(lFNode)); break; // USE COST MODEL - Delete lF, leftmost root node in F_{lF,rF}.
                                case 2: sp1 = t[lG - it2PreLoff][rG - it2PreRoff] + (treesSwapped ? this->costModel->insertCost(lFNode) : this->costModel->deleteCost(lFNode)); break; // USE COST MODEL - Delete lF, leftmost root node in F_{lF,rF}.
                                case 3: sp1 = currentForestCost2 + (treesSwapped ? this->costModel->insertCost(lFNode) : this->costModel->deleteCost(lFNode)); break; // USE COST MODEL - Insert G_{lG,rG} and elete lF, leftmost root node in F_{lF,rF}.
                            }

                            sp2 = (*sp2spointer)[fn[lG] - it2PreLoff] + (treesSwapped ? this->costModel->deleteCost(it2nodes[lG]) : this->costModel->insertCost(it2nodes[lG])); // USE COST MODEL - Insert lG, leftmost root node in G_{lG,rG}.
                            minCost = sp1;
                            if(sp2 < minCost) {
                                minCost = sp2;
                            }

                            sp3 = treesSwapped ? delta[lG][lF] : (*sp3deltapointer)[lG];
                            if (sp3 < minCost) {
                                switch(sp3source) {
                                    case 1: sp3 += (*sp3spointer)[fn[(lG + it2sizes[lG]) - 1] - it2PreLoff]; break;
                                    case 2: sp3 += currentForestCost2 - (treesSwapped ? it2->preL_to_sumDelCost[lG] : it2->preL_to_sumInsCost[lG]); break; // USE COST MODEL - Insert G_{lG,rG}-G_lG.
                                    case 3: sp3 += t[fn[(lG + it2sizes[lG]) - 1] - it2PreLoff][rG - it2PreRoff]; break;
                                }

                                if (sp3 < minCost) {
                                    sp3 += (treesSwapped ? this->costModel->renameCost(it2nodes[lG], lFNode) : this->costModel->renameCost(lFNode, it2nodes[lG])); // USE COST MODEL - Rename the leftmost root nodes in F_{lF,rF} and G_{lG,rG}.
                                    if (sp3 < minCost) {
                                        minCost = sp3;
                                    }
                                }
                            }
                            (*swritepointer)[lG - it2PreLoff] = minCost;
                            lG = ft[lG];
                            counter++;
                        }
                    }

                    if (rGminus1_in_preL == parent_of_rG_in_preL) {
                        if (!rightPart) {
                            if (leftPart) {
                                if (treesSwapped) {
                                    delta[parent_of_rG_in_preL][endPathNode] = s[(lFlast + 1) - it1PreLoff][(rGminus1_in_preL + 1) - it2PreLoff];
                                } else {
                                    delta[endPathNode][parent_of_rG_in_preL] = s[(lFlast + 1) - it1PreLoff][(rGminus1_in_preL + 1) - it2PreLoff];
                                }
                            }
                            if (endPathNode > 0 && endPathNode == parent_of_endPathNode + 1 && endPathNode_in_preR == parent_of_endPathNode_in_preR + 1) {
                                if (treesSwapped) {
                                    delta[parent_of_rG_in_preL][parent_of_endPathNode] = s[lFlast - it1PreLoff][(rGminus1_in_preL + 1) - it2PreLoff];
                                } else {
                                    delta[parent_of_endPathNode][parent_of_rG_in_preL] = s[lFlast - it1PreLoff][(rGminus1_in_preL + 1) - it2PreLoff];
                                }
                            }
                        }

                        for (Integer lF = lFfirst; lF >= lFlast; lF--) {
                            q[lF] = s[lF - it1PreLoff][(parent_of_rG_in_preL + 1) - it2PreLoff];
                        }
                    }

                    // TODO: first pointers can be precomputed
                    for (Integer lG = lGfirst; lG >= lGlast; lG = ft[lG]) {
                        t[lG - it2PreLoff][rG - it2PreRoff] = s[lFlast - it1PreLoff][lG - it2PreLoff];
                    }
                }
            }

            // Deal with nodes to the right of the path.
            if (pathType == 0 || (pathType == 2 && rightPart) || (pathType == 2 && !leftPart && !rightPart)) {
                if (startPathNode == -1) {
                    lFfirst = endPathNode;
                    rFfirst = it1preL_to_preR[endPathNode];
                } else {
                    rFfirst = it1preL_to_preR[startPathNode] - 1;
                    lFfirst = endPathNode + 1;
                }

                lFlast = endPathNode;
                lGlast = currentSubtreePreL2;
                lGfirst = (lGlast + subtreeSize2) - 1;
                rFlast = it1preL_to_preR[endPathNode];
                fn[fn.size() - 1] = -1;

                for (Integer i = currentSubtreePreL2; i < currentSubtreePreL2 + subtreeSize2; i++){
                    fn[i] = -1;
                    ft[i] = -1;
                }

                // Store size and cost of the current forest in F.
                tmpForestSize1 = currentForestSize1;
                tmpForestCost1 = currentForestCost1;

                // Loop B' [1, Algorithm 3] - for all nodes in G.
                for (Integer lG = lGfirst; lG >= lGlast; lG--) {
                    rGfirst = it2preL_to_preR[lG];
                    updateFnArray(it2->preR_to_ln[rGfirst], rGfirst, it2preL_to_preR[currentSubtreePreL2]);
                    updateFtArray(it2->preR_to_ln[rGfirst], rGfirst);
                    Integer lF = lFfirst;
                    lGminus1_in_preR = lG <= currentSubtreePreL2 ? 0x7fffffff : it2preL_to_preR[lG - 1];
                    parent_of_lG = it2parents[lG];
                    parent_of_lG_in_preR = parent_of_lG == -1 ? -1 : it2preL_to_preR[parent_of_lG];

                    // Reset size and cost of forest if F.
                    currentForestSize1 = tmpForestSize1;
                    currentForestCost1 = tmpForestCost1;

                    if (pathType == 0) {
                        if (lG == currentSubtreePreL2) {
                            rGlast = rGfirst;
                        } else if (it2->children[parent_of_lG][0] != lG) {
                            rGlast = rGfirst;
                        } else {
                            rGlast = it2preL_to_preR[parent_of_lG]+1;
                        }
                    } else {
                        rGlast = rGfirst == it2preL_to_preR[currentSubtreePreL2] ? rGfirst : it2preL_to_preR[currentSubtreePreL2];
                    }

                    // Loop C' [1, Algorithm 3] - for all nodes to the right of the path node.
                    for (Integer rF = rFfirst; rF >= rFlast; rF--) {
                        if (rF == rFlast) {
                            lF = lFlast;
                        }
                        rF_in_preL = it1preR_to_preL[rF];

                        // Increment size and cost of F forest by node rF.
                        currentForestSize1++;
                        currentForestCost1 += (treesSwapped ? this->costModel->insertCost(it1->preL_to_node[rF_in_preL]) : this->costModel->deleteCost(it1->preL_to_node[rF_in_preL])); // USE COST MODEL - sum up deletion cost of a forest.

                        // Reset size and cost of G forest to G_lG.
                        currentForestSize2 = it2sizes[lG];
                        currentForestCost2 = (treesSwapped ? it2->preL_to_sumDelCost[lG] : it2->preL_to_sumInsCost[lG]); // USE COST MODEL - reset to subtree insertion cost.
                        rFSubtreeSize = it1sizes[rF_in_preL];

                        if (startPathNode > 0) {
                            rFIsConsecutiveNodeOfCurrentPathNode = startPathNode_in_preR - rF == 1;
                            rFIsRightSiblingOfCurrentPathNode = rF + rFSubtreeSize == startPathNode_in_preR;
                        } else {
                            rFIsConsecutiveNodeOfCurrentPathNode = false;
                            rFIsRightSiblingOfCurrentPathNode = false;
                        }

                        fForestIsTree = rF_in_preL == lF;
                        Node<Data>* rFNode = it1->preL_to_node[rF_in_preL];
                        sp1spointer = &(s[(rF + 1) - it1PreRoff]);
                        sp2spointer = &(s[rF - it1PreRoff]);
                        sp3spointer = &(s[0]);
                        sp3deltapointer = treesSwapped ? nullptr : &(delta[rF_in_preL]);
                        swritepointer = &(s[rF - it1PreRoff]);
                        sp1tpointer = &(t[lG - it2PreLoff]);
                        sp3tpointer = &(t[lG - it2PreLoff]);
                        sp1source = 1;
                        sp3source = 1;

                        if (fForestIsTree) {
                            if (rFSubtreeSize == 1) {
                                sp1source = 3;
                            } else if (rFIsConsecutiveNodeOfCurrentPathNode) {
                                sp1source = 2;
                            }
                            sp3 = 0;
                            sp3source = 2;
                        } else {
                            if (rFIsConsecutiveNodeOfCurrentPathNode) {
                                sp1source = 2;
                            }
                            sp3 = currentForestCost1 - (treesSwapped ? it1->preL_to_sumInsCost[rF_in_preL] : it1->preL_to_sumDelCost[rF_in_preL]); // USE COST MODEL - Delete F_{lF,rF}-F_rF.
                            if (rFIsRightSiblingOfCurrentPathNode) {
                                sp3source = 3;
                            }
                        }

                        if (sp3source == 1) {
                            sp3spointer = &(s[(rF + rFSubtreeSize) - it1PreRoff]);
                        }

                        if (currentForestSize2 == 1) {
                            sp2 = currentForestCost1;// USE COST MODEL - Delete F_{lF,rF}.
                        } else {
                            sp2 = q[rF];
                        }

                        Integer rG = rGfirst;
                        rGfirst_in_preL = it2preR_to_preL[rGfirst];
                        currentForestSize2++;

                        switch (sp1source) {
                            case 1: sp1 = (*sp1spointer)[rG - it2PreRoff]; break;
                            case 2: sp1 = (*sp1tpointer)[rG - it2PreRoff]; break;
                            case 3: sp1 = currentForestCost2; break; // USE COST MODEL - Insert G_{lG,rG}.
                        }

                        sp1 += (treesSwapped ? this->costModel->insertCost(rFNode) : this->costModel->deleteCost(rFNode)); // USE COST MODEL - Delete rF.
                        minCost = sp1;

                        sp2 += (treesSwapped ? this->costModel->deleteCost(it2nodes[rGfirst_in_preL]) : this->costModel->insertCost(it2nodes[rGfirst_in_preL])); // USE COST MODEL - Insert rG.
                        if (sp2 < minCost) {
                            minCost = sp2;
                        }

                        if (sp3 < minCost) {
                            sp3 += treesSwapped ? delta[rGfirst_in_preL][rF_in_preL] : (*sp3deltapointer)[rGfirst_in_preL];
                            if (sp3 < minCost) {
                                sp3 += (treesSwapped ? this->costModel->renameCost(it2nodes[rGfirst_in_preL], rFNode) : this->costModel->renameCost(rFNode, it2nodes[rGfirst_in_preL]));
                                if (sp3 < minCost) {
                                    minCost = sp3;
                                }
                            }
                        }

                        (*swritepointer)[rG - it2PreRoff] = minCost;
                        rG = ft[rG];
                        counter++;

                        // Loop D' [1, Algorithm 3] - for all nodes to the right of lG;
                        while (rG >= rGlast) {
                            rG_in_preL = it2preR_to_preL[rG];
                            // Increment size and cost of G forest by node rG.
                            currentForestSize2++;
                            currentForestCost2 += (treesSwapped ? this->costModel->deleteCost(it2nodes[rG_in_preL]) : this->costModel->insertCost(it2nodes[rG_in_preL]));
                            switch (sp1source) {
                                case 1: sp1 = (*sp1spointer)[rG - it2PreRoff] + (treesSwapped ? this->costModel->insertCost(rFNode) : this->costModel->deleteCost(rFNode)); break; // USE COST MODEL - Delete rF.
                                case 2: sp1 = (*sp1tpointer)[rG - it2PreRoff] + (treesSwapped ? this->costModel->insertCost(rFNode) : this->costModel->deleteCost(rFNode)); break; // USE COST MODEL - Delete rF.
                                case 3: sp1 = currentForestCost2 + (treesSwapped ? this->costModel->insertCost(rFNode) : this->costModel->deleteCost(rFNode)); break; // USE COST MODEL - Insert G_{lG,rG} and delete rF.
                            }
                            sp2 = (*sp2spointer)[fn[rG] - it2PreRoff] + (treesSwapped ? this->costModel->deleteCost(it2nodes[rG_in_preL]) : this->costModel->insertCost(it2nodes[rG_in_preL])); // USE COST MODEL - Insert rG.
                            minCost = sp1;
                            if (sp2 < minCost) {
                                minCost = sp2;
                            }
                            sp3 = treesSwapped ? delta[rG_in_preL][rF_in_preL] : (*sp3deltapointer)[rG_in_preL];
                            if (sp3 < minCost) {
                                switch (sp3source) {
                                    case 1: sp3 += (*sp3spointer)[fn[(rG + it2sizes[rG_in_preL]) - 1] - it2PreRoff]; break;
                                    case 2: sp3 += currentForestCost2 - (treesSwapped ? it2->preL_to_sumDelCost[rG_in_preL] : it2->preL_to_sumInsCost[rG_in_preL]); break; // USE COST MODEL - Insert G_{lG,rG}-G_rG.
                                    case 3: sp3 += (*sp3tpointer)[fn[(rG + it2sizes[rG_in_preL]) - 1] - it2PreRoff]; break;
                                }
                                if (sp3 < minCost) {
                                    sp3 += (treesSwapped ? this->costModel->renameCost(it2nodes[rG_in_preL], rFNode) : this->costModel->renameCost(rFNode, it2nodes[rG_in_preL])); // USE COST MODEL - Rename rF to rG.
                                    if (sp3 < minCost) {
                                        minCost = sp3;
                                    }
                                }
                            }
                            (*swritepointer)[rG - it2PreRoff] = minCost;
                            rG = ft[rG];
                            counter++;
                        }
                    }

                    if (lG > currentSubtreePreL2 && lG - 1 == parent_of_lG) {
                        if (rightPart) {
                            if (treesSwapped) {
                                delta[parent_of_lG][endPathNode] = s[(rFlast + 1) - it1PreRoff][(lGminus1_in_preR + 1) - it2PreRoff];
                            } else {
                                delta[endPathNode][parent_of_lG] = s[(rFlast + 1) - it1PreRoff][(lGminus1_in_preR + 1) - it2PreRoff];
                            }
                        }

                        if (endPathNode > 0 && endPathNode == parent_of_endPathNode + 1 && endPathNode_in_preR == parent_of_endPathNode_in_preR + 1) {
                            if (treesSwapped) {
                                delta[parent_of_lG][parent_of_endPathNode] = s[rFlast - it1PreRoff][(lGminus1_in_preR + 1) - it2PreRoff];
                            } else {
                                delta[parent_of_endPathNode][parent_of_lG] = s[rFlast - it1PreRoff][(lGminus1_in_preR + 1) - it2PreRoff];
                            }
                        }

                        for (Integer rF = rFfirst; rF >= rFlast; rF--) {
                            q[rF] = s[rF - it1PreRoff][(parent_of_lG_in_preR + 1) - it2PreRoff];
                        }
                    }

                    // TODO: first pointers can be precomputed
                    for (Integer rG = rGfirst; rG >= rGlast; rG = ft[rG]) {
                        t[lG - it2PreLoff][rG - it2PreRoff] = s[rFlast - it1PreRoff][rG - it2PreRoff];
                    }
                }
            }

            // Walk up the path by one node.
            startPathNode = endPathNode;
            endPathNode = it1parents[endPathNode];
        }

        return minCost;
    }

    //--------------------------------------------------------------------------

    float spfL(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2, bool treesSwapped) {
        // Initialise the array to store the keyroot nodes in the right-hand input subtree.
        std::vector<Integer> keyRoots(it2->sizes[it2->getCurrentNode()], -1);

        // Get the leftmost leaf node of the right-hand input subtree.
        Integer pathID = it2->preL_to_lld(it2->getCurrentNode());

        // Calculate the keyroot nodes in the right-hand input subtree.
        // firstKeyRoot is the index in keyRoots of the first keyroot node that
        // we have to process. We need this index because keyRoots array is larger
        // than the number of keyroot nodes.
        Integer firstKeyRoot = computeKeyRoots(it2, it2->getCurrentNode(), pathID, keyRoots, 0);

        // Initialise an array to store intermediate distances for subforest pairs.
        std::vector<std::vector<float>> forestdist(it1->sizes[it1->getCurrentNode()] + 1);
        for (size_t i = 0; i < forestdist.size(); i++) {
            forestdist[i].resize(it2->sizes[it2->getCurrentNode()] + 1);
        }

        // Compute the distances between pairs of keyroot nodes. In the left-hand
        // input subtree only the root is the keyroot. Thus, we compute the distance
        // between the left-hand input subtree and all keyroot nodes in the
        // right-hand input subtree.
        for (Integer i = firstKeyRoot-1; i >= 0; i--) {
            treeEditDist(it1, it2, it1->getCurrentNode(), keyRoots[i], forestdist, treesSwapped);
        }

        return forestdist[it1->sizes[it1->getCurrentNode()]][it2->sizes[it2->getCurrentNode()]];
    }

    Integer computeKeyRoots(NodeIndexer<Data>* it2, Integer subtreeRootNode, Integer pathID, std::vector<Integer> &keyRoots, Integer index) {
        // The subtreeRootNode is a keyroot node. Add it to keyRoots.
        keyRoots[index] = subtreeRootNode;

        // Increment the index to know where to store the next keyroot node.
        index++;

        // Walk up the left path starting with the leftmost leaf of subtreeRootNode,
        // until the child of subtreeRootNode.
        Integer pathNode = pathID;

        while (pathNode > subtreeRootNode) {
            Integer parent = it2->parents[pathNode];
            // For each sibling to the right of pathNode, execute this method recursively.
            // Each right sibling of pathNode is a keyroot node.
            for (Integer child : it2->children[parent]) {
                // Execute computeKeyRoots recursively for the new subtree rooted at child and child's leftmost leaf node.
                if (child != pathNode) {
                    index = computeKeyRoots(it2, child, it2->preL_to_lld(child), keyRoots, index);
                }
            }
            // Walk up.
            pathNode = parent;
        }

        return index;
    }

    void treeEditDist(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2, Integer it1subtree, Integer it2subtree, std::vector<std::vector<float>> &forestdist, bool treesSwapped) {
        // Translate input subtree root nodes to left-to-right postorder.
        Integer i = it1->preL_to_postL[it1subtree];
        Integer j = it2->preL_to_postL[it2subtree];

        // We need to offset the node ids for accessing forestdist array which has
        // indices from 0 to subtree size. However, the subtree node indices do not
        // necessarily start with 0.
        // Whenever the original left-to-right postorder id has to be accessed, use
        // i+ioff and j+joff.
        Integer ioff = it1->postL_to_lld[i] - 1;
        Integer joff = it2->postL_to_lld[j] - 1;

        // Variables holding costs of each minimum element.
        float da = 0;
        float db = 0;
        float dc = 0;

        // Initialize forestdist array with deletion and insertion costs of each
        // relevant subforest.
        forestdist[0][0] = 0;
        for (Integer i1 = 1; i1 <= i - ioff; i1++) {
            forestdist[i1][0] = forestdist[i1 - 1][0] + (treesSwapped ? this->costModel->insertCost(it1->postL_to_node(i1 + ioff)) : this->costModel->deleteCost(it1->postL_to_node(i1 + ioff))); // USE COST MODEL - delete i1.
        }
        for (Integer j1 = 1; j1 <= j - joff; j1++) {
            forestdist[0][j1] = forestdist[0][j1 - 1] + (treesSwapped ? this->costModel->deleteCost(it2->postL_to_node(j1 + joff)) : this->costModel->insertCost(it2->postL_to_node(j1 + joff))); // USE COST MODEL - insert j1.
        }

        // Fill in the remaining costs.
        for (Integer i1 = 1; i1 <= i - ioff; i1++) {
            for (Integer j1 = 1; j1 <= j - joff; j1++) {
                // Increment the number of subproblems.
                counter++;

                // Calculate partial distance values for this subproblem.
                float u = (treesSwapped ? this->costModel->renameCost(it2->postL_to_node(j1 + joff), it1->postL_to_node(i1 + ioff)) : this->costModel->renameCost(it1->postL_to_node(i1 + ioff), it2->postL_to_node(j1 + joff))); // USE COST MODEL - rename i1 to j1.
                da = forestdist[i1 - 1][j1] + (treesSwapped ? this->costModel->insertCost(it1->postL_to_node(i1 + ioff)) : this->costModel->deleteCost(it1->postL_to_node(i1 + ioff))); // USE COST MODEL - delete i1.
                db = forestdist[i1][j1 - 1] + (treesSwapped ? this->costModel->deleteCost(it2->postL_to_node(j1 + joff)) : this->costModel->insertCost(it2->postL_to_node(j1 + joff))); // USE COST MODEL - insert j1.

                // If current subforests are subtrees.
                if (it1->postL_to_lld[i1 + ioff] == it1->postL_to_lld[i] && it2->postL_to_lld[j1 + joff] == it2->postL_to_lld[j]) {
                    dc = forestdist[i1 - 1][j1 - 1] + u;
                    // Store the relevant distance value in delta array.
                    if (treesSwapped) {
                        delta[it2->postL_to_preL[j1 + joff]][it1->postL_to_preL[i1 + ioff]] = forestdist[i1 - 1][j1 - 1];
                    } else {
                        delta[it1->postL_to_preL[i1 + ioff]][it2->postL_to_preL[j1 + joff]] = forestdist[i1 - 1][j1 - 1];
                    }
                } else {
                    dc = forestdist[it1->postL_to_lld[i1 + ioff] - 1 - ioff][it2->postL_to_lld[j1 + joff] - 1 - joff] 
                         + (treesSwapped ? delta[it2->postL_to_preL[j1 + joff]][it1->postL_to_preL[i1 + ioff]] : delta[it1->postL_to_preL[i1 + ioff]][it2->postL_to_preL[j1 + joff]])
                         + u;
                }

                // Calculate final minimum.
                forestdist[i1][j1] = da >= db ? db >= dc ? dc : db : da >= dc ? dc : da;
            }
        }
    }

    //--------------------------------------------------------------------------

    float spfR(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2, bool treesSwapped) {
        // Initialise the array to store the keyroot nodes in the right-hand input subtree.
        std::vector<Integer> revKeyRoots(it2->sizes[it2->getCurrentNode()], -1);

        // Get the rightmost leaf node of the right-hand input subtree.
        Integer pathID = it2->preL_to_rld(it2->getCurrentNode());

        // Calculate the keyroot nodes in the right-hand input subtree.
        // firstKeyRoot is the index in keyRoots of the first keyroot node that
        // we have to process. We need this index because keyRoots array is larger
        // than the number of keyroot nodes.
        Integer firstKeyRoot = computeRevKeyRoots(it2, it2->getCurrentNode(), pathID, revKeyRoots, 0);

        // Initialise an array to store intermediate distances for subforest pairs.
        std::vector<std::vector<float>> forestdist(it1->sizes[it1->getCurrentNode()] + 1);
        for (size_t i = 0; i < forestdist.size(); i++) {
            forestdist[i].resize(it2->sizes[it2->getCurrentNode()] + 1);
        }

        // Compute the distances between pairs of keyroot nodes. In the left-hand
        // input subtree only the root is the keyroot. Thus, we compute the distance
        // between the left-hand input subtree and all keyroot nodes in the
        // right-hand input subtree.
        for (Integer i = firstKeyRoot - 1; i >= 0; i--) {
            revTreeEditDist(it1, it2, it1->getCurrentNode(), revKeyRoots[i], forestdist, treesSwapped);
        }

        // Return the distance between the input subtrees.
        return forestdist[it1->sizes[it1->getCurrentNode()]][it2->sizes[it2->getCurrentNode()]];
    }

    Integer computeRevKeyRoots(NodeIndexer<Data>* it2, Integer subtreeRootNode, Integer pathID, std::vector<Integer> &revKeyRoots, Integer index) {
        // The subtreeRootNode is a keyroot node. Add it to keyRoots.
        revKeyRoots[index] = subtreeRootNode;

        // Increment the index to know where to store the next keyroot node.
        index++;

        // Walk up the right path starting with the rightmost leaf of
        // subtreeRootNode, until the child of subtreeRootNode.
        Integer pathNode = pathID;

        while (pathNode > subtreeRootNode) {
            Integer parent = it2->parents[pathNode];
            // For each sibling to the left of pathNode, execute this method recursively.
            // Each left sibling of pathNode is a keyroot node.
            for (Integer child : it2->children[parent]) {
                // Execute computeRevKeyRoots recursively for the new subtree rooted at child and child's rightmost leaf node.
                if (child != pathNode) {
                    index = computeRevKeyRoots(it2, child, it2->preL_to_rld(child), revKeyRoots, index);
                }
            }
            // Walk up.
            pathNode = parent;
        }

        return index;
    }

    void revTreeEditDist(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2, Integer it1subtree, Integer it2subtree, std::vector<std::vector<float>> &forestdist, bool treesSwapped) {
        // Translate input subtree root nodes to right-to-left postorder.
        Integer i = it1->preL_to_postR[it1subtree];
        Integer j = it2->preL_to_postR[it2subtree];

        // We need to offset the node ids for accessing forestdist array which has
        // indices from 0 to subtree size. However, the subtree node indices do not
        // necessarily start with 0.
        // Whenever the original right-to-left postorder id has to be accessed, use
        // i+ioff and j+joff.
        Integer ioff = it1->postR_to_rld[i] - 1;
        Integer joff = it2->postR_to_rld[j] - 1;

        // Variables holding costs of each minimum element.
        float da = 0;
        float db = 0;
        float dc = 0;

        // Initialize forestdist array with deletion and insertion costs of each
        // relevant subforest.
        forestdist[0][0] = 0;
        for (Integer i1 = 1; i1 <= i - ioff; i1++) {
            forestdist[i1][0] = forestdist[i1 - 1][0] + (treesSwapped ? this->costModel->insertCost(it1->postR_to_node(i1 + ioff)) : this->costModel->deleteCost(it1->postR_to_node(i1 + ioff))); // USE COST MODEL - delete i1.
        }
        for (Integer j1 = 1; j1 <= j - joff; j1++) {
            forestdist[0][j1] = forestdist[0][j1 - 1] + (treesSwapped ? this->costModel->deleteCost(it2->postR_to_node(j1 + joff)) : this->costModel->insertCost(it2->postR_to_node(j1 + joff))); // USE COST MODEL - insert j1.
        }

        // Fill in the remaining costs.
        for (Integer i1 = 1; i1 <= i - ioff; i1++) {
            for (Integer j1 = 1; j1 <= j - joff; j1++) {
                // Increment the number of subproblems.
                counter++;

                // Calculate partial distance values for this subproblem.
                float u = (treesSwapped ? this->costModel->renameCost(it2->postR_to_node(j1 + joff), it1->postR_to_node(i1 + ioff)) : this->costModel->renameCost(it1->postR_to_node(i1 + ioff), it2->postR_to_node(j1 + joff))); // USE COST MODEL - rename i1 to j1.
                da = forestdist[i1 - 1][j1] + (treesSwapped ? this->costModel->insertCost(it1->postR_to_node(i1 + ioff)) : this->costModel->deleteCost(it1->postR_to_node(i1 + ioff))); // USE COST MODEL - delete i1.
                db = forestdist[i1][j1 - 1] + (treesSwapped ? this->costModel->deleteCost(it2->postR_to_node(j1 + joff)) : this->costModel->insertCost(it2->postR_to_node(j1 + joff))); // USE COST MODEL - insert j1.
                
                // If current subforests are subtrees.
                if (it1->postR_to_rld[i1 + ioff] == it1->postR_to_rld[i] && it2->postR_to_rld[j1 + joff] == it2->postR_to_rld[j]) {
                    dc = forestdist[i1 - 1][j1 - 1] + u;
                    // Store the relevant distance value in delta array.
                    if (treesSwapped) {
                        delta[it2->postR_to_preL[j1+joff]][it1->postR_to_preL[i1+ioff]] = forestdist[i1 - 1][j1 - 1];
                    } else {
                        delta[it1->postR_to_preL[i1+ioff]][it2->postR_to_preL[j1+joff]] = forestdist[i1 - 1][j1 - 1];
                    }
                } else {
                    dc = forestdist[it1->postR_to_rld[i1 + ioff] - 1 - ioff][it2->postR_to_rld[j1 + joff] - 1 - joff] +
                    (treesSwapped ? delta[it2->postR_to_preL[j1 + joff]][it1->postR_to_preL[i1 + ioff]] : delta[it1->postR_to_preL[i1 + ioff]][it2->postR_to_preL[j1 + joff]]) + u;
                }
                
                // Calculate final minimum.
                forestdist[i1][j1] = da >= db ? db >= dc ? dc : db : da >= dc ? dc : da;
            }
        }
    }

    //--------------------------------------------------------------------------

    float spf1 (NodeIndexer<Data>* ni1, Integer subtreeRootNode1, NodeIndexer<Data>* ni2, Integer subtreeRootNode2) {
        Integer subtreeSize1 = ni1->sizes[subtreeRootNode1];
        Integer subtreeSize2 = ni2->sizes[subtreeRootNode2];

        if (subtreeSize1 == 1 && subtreeSize2 == 1) {
            Node<Data>* n1 = ni1->preL_to_node[subtreeRootNode1];
            Node<Data>* n2 = ni2->preL_to_node[subtreeRootNode2];
            float maxCost = this->costModel->deleteCost(n1) + this->costModel->insertCost(n2);
            float renCost = this->costModel->renameCost(n1, n2);
            return renCost < maxCost ? renCost : maxCost;
        }

        if (subtreeSize1 == 1) {
            Node<Data>* n1 = ni1->preL_to_node[subtreeRootNode1];
            Node<Data>* n2 = nullptr;
            float cost = ni2->preL_to_sumInsCost[subtreeRootNode2];
            float maxCost = cost + this->costModel->deleteCost(n1);
            float minRenMinusIns = cost;
            float nodeRenMinusIns = 0;
            for (Integer i = subtreeRootNode2; i < subtreeRootNode2 + subtreeSize2; i++) {
                n2 = ni2->preL_to_node[i];
                nodeRenMinusIns = this->costModel->renameCost(n1, n2) - this->costModel->insertCost(n2);
                if (nodeRenMinusIns < minRenMinusIns) {
                    minRenMinusIns = nodeRenMinusIns;
                }
            }

            cost += minRenMinusIns;
            return cost < maxCost ? cost : maxCost;
        }

        if (subtreeSize2 == 1) {
            Node<Data>* n1 = nullptr;
            Node<Data>* n2 = ni2->preL_to_node[subtreeRootNode2];

            float cost = ni1->preL_to_sumDelCost[subtreeRootNode1];
            float maxCost = cost + this->costModel->insertCost(n2);
            float minRenMinusDel = cost;
            float nodeRenMinusDel = 0;

            for (Integer i = subtreeRootNode1; i < subtreeRootNode1 + subtreeSize1; i++) {
                n1 = ni1->preL_to_node[i];
                nodeRenMinusDel = this->costModel->renameCost(n1, n2) - this->costModel->deleteCost(n1);

                if (nodeRenMinusDel < minRenMinusDel) {
                    minRenMinusDel = nodeRenMinusDel;
                }
            }

            cost += minRenMinusDel;
            return cost < maxCost ? cost : maxCost;
        }

        return -1;
    }

    //--------------------------------------------------------------------------

    void computeOptStrategy_postL() {
        Integer size1 = this->it1->getSize();
        Integer size2 = this->it2->getSize();

        assert(delta.size() == 0);
        delta.resize(size1);
        for (size_t i = 0; i < delta.size(); i++) {
            delta[i].resize(size2);
        }

        std::vector<std::vector<float>> cost1_L(size1);
        std::vector<std::vector<float>> cost1_R(size1);
        std::vector<std::vector<float>> cost1_I(size1);
        std::vector<float> cost2_L(size2);
        std::vector<float> cost2_R(size2);
        std::vector<float> cost2_I(size2);
        std::vector<Integer> cost2_path(size2);
        std::vector<float> leafRow(size2);
        Integer pathIDOffset = size1;
        float minCost = 0x7fffffffffffffffL;
        Integer strategyPath = -1;

        std::vector<Integer> &pre2size1 = this->it1->sizes;
        std::vector<Integer> &pre2size2 = this->it2->sizes;
        std::vector<Integer> &pre2descSum1 = this->it1->preL_to_desc_sum;
        std::vector<Integer> &pre2descSum2 = this->it2->preL_to_desc_sum;
        std::vector<Integer> &pre2krSum1 = this->it1->preL_to_kr_sum;
        std::vector<Integer> &pre2krSum2 = this->it2->preL_to_kr_sum;
        std::vector<Integer> &pre2revkrSum1 = this->it1->preL_to_rev_kr_sum;
        std::vector<Integer> &pre2revkrSum2 = this->it2->preL_to_rev_kr_sum;
        std::vector<Integer> &preL_to_preR_1 = this->it1->preL_to_preR;
        std::vector<Integer> &preL_to_preR_2 = this->it2->preL_to_preR;
        std::vector<Integer> &preR_to_preL_1 = this->it1->preR_to_preL;
        std::vector<Integer> &preR_to_preL_2 = this->it2->preR_to_preL;
        std::vector<Integer> &pre2parent1 = this->it1->parents;
        std::vector<Integer> &pre2parent2 = this->it2->parents;
        std::vector<bool> &nodeType_L_1 = this->it1->nodeType_L;
        std::vector<bool> &nodeType_L_2 = this->it2->nodeType_L;
        std::vector<bool> &nodeType_R_1 = this->it1->nodeType_R;
        std::vector<bool> &nodeType_R_2 = this->it2->nodeType_R;

        std::vector<Integer> &preL_to_postL_1 = this->it1->preL_to_postL;
        std::vector<Integer> &preL_to_postL_2 = this->it2->preL_to_postL;
        std::vector<Integer> &postL_to_preL_1 = this->it1->postL_to_preL;
        std::vector<Integer> &postL_to_preL_2 = this->it2->postL_to_preL;

        Integer size_w,
            size_v,
            parent_w_preL,
            parent_v_preL,
            parent_w_postL = -1,
            parent_v_postL = -1;
        Integer leftPath_v,
            rightPath_v;

        std::vector<float> cost_Lpointer_v,
                           cost_Rpointer_v,
                           cost_Ipointer_v;
        std::vector<float> cost_Lpointer_parent_v, 
                           cost_Rpointer_parent_v,
                           cost_Ipointer_parent_v;
        std::vector<float> strategypointer_parent_v;

        Integer krSum_v, revkrSum_v, descSum_v;
        bool is_v_leaf;

        Integer v_in_preL;
        Integer w_in_preL;

        std::stack<std::vector<float>> rowsToReuse_L;
        std::stack<std::vector<float>> rowsToReuse_R;
        std::stack<std::vector<float>> rowsToReuse_I;

        for(Integer v = 0; v < size1; v++) {
            v_in_preL = postL_to_preL_1[v];

            is_v_leaf = this->it1->isLeaf(v_in_preL);
            parent_v_preL = pre2parent1[v_in_preL];

            if (parent_v_preL != -1) {
                parent_v_postL = preL_to_postL_1[parent_v_preL];
            }

            size_v = pre2size1[v_in_preL];
            leftPath_v = -(preR_to_preL_1[preL_to_preR_1[v_in_preL] + size_v - 1] + 1);// this is the left path's ID which is the leftmost leaf node: l-r_preorder(r-l_preorder(v) + |Fv| - 1)
            rightPath_v = v_in_preL + size_v - 1 + 1; // this is the right path's ID which is the rightmost leaf node: l-r_preorder(v) + |Fv| - 1
            krSum_v = pre2krSum1[v_in_preL];
            revkrSum_v = pre2revkrSum1[v_in_preL];
            descSum_v = pre2descSum1[v_in_preL];

            if (is_v_leaf) {
                cost1_L[v] = leafRow;
                cost1_R[v] = leafRow;
                cost1_I[v] = leafRow;
                for(Integer i = 0; i < size2; i++) {
                    delta[v_in_preL][postL_to_preL_2[i]] = v_in_preL;
                }
            }

            cost_Lpointer_v = cost1_L[v];
            cost_Rpointer_v = cost1_R[v];
            cost_Ipointer_v = cost1_I[v];

            if (parent_v_preL != -1 && cost1_L[parent_v_postL].size() == 0) {
                if (rowsToReuse_L.empty()) {
                    cost1_L[parent_v_postL] = std::vector<float>(size2);
                    cost1_R[parent_v_postL] = std::vector<float>(size2);
                    cost1_I[parent_v_postL] = std::vector<float>(size2);
                } else {
                    cost1_L[parent_v_postL] = rowsToReuse_L.top();
                    rowsToReuse_L.pop();

                    cost1_R[parent_v_postL] = rowsToReuse_R.top();
                    rowsToReuse_R.pop();

                    cost1_I[parent_v_postL] = rowsToReuse_I.top();
                    rowsToReuse_I.pop();
                }
            }

            if (parent_v_preL != -1) {
                cost_Lpointer_parent_v = cost1_L[parent_v_postL];
                cost_Rpointer_parent_v = cost1_R[parent_v_postL];
                cost_Ipointer_parent_v = cost1_I[parent_v_postL];
                strategypointer_parent_v = delta[parent_v_preL];
            }

            fillArray(cost2_L, 0.0f);
            fillArray(cost2_R, 0.0f);
            fillArray(cost2_I, 0.0f);
            fillArray(cost2_path, (Integer)0);

            for(Integer w = 0; w < size2; w++) {
                w_in_preL = postL_to_preL_2[w];

                parent_w_preL = pre2parent2[w_in_preL];
                if (parent_w_preL != -1) {
                    parent_w_postL = preL_to_postL_2[parent_w_preL];
                }

                size_w = pre2size2[w_in_preL];
                if (this->it2->isLeaf(w_in_preL)) {
                    cost2_L[w] = 0L;
                    cost2_R[w] = 0L;
                    cost2_I[w] = 0L;
                    cost2_path[w] = w_in_preL;
                }

                minCost = 0x7fffffffffffffffL;
                strategyPath = -1;
                float tmpCost = 0x7fffffffffffffffL;

                if (size_v <= 1 || size_w <= 1) { // USE NEW SINGLE_PATH FUNCTIONS FOR SMALL SUBTREES
                    minCost = std::max(size_v, size_w);
                } else {
                    tmpCost = (float) size_v * (float) pre2krSum2[w_in_preL] + cost_Lpointer_v[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = leftPath_v;
                    }
                    tmpCost = (float) size_v * (float) pre2revkrSum2[w_in_preL] + cost_Rpointer_v[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = rightPath_v;
                    }
                    tmpCost = (float) size_v * (float) pre2descSum2[w_in_preL] + cost_Ipointer_v[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = (Integer)delta[v_in_preL][w_in_preL] + 1;
                    }
                    tmpCost = (float) size_w * (float) krSum_v + cost2_L[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = -(preR_to_preL_2[preL_to_preR_2[w_in_preL] + size_w - 1] + pathIDOffset + 1);
                    }
                    tmpCost = (float) size_w * (float) revkrSum_v + cost2_R[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = w_in_preL + size_w - 1 + pathIDOffset + 1;
                    }
                    tmpCost = (float) size_w * (float) descSum_v + cost2_I[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = cost2_path[w] + pathIDOffset + 1;
                    }
                }

                if (parent_v_preL != -1) {
                    cost_Rpointer_parent_v[w] += minCost;
                    tmpCost = -minCost + cost1_I[v][w];
                    if (tmpCost < cost1_I[parent_v_postL][w]) {
                        cost_Ipointer_parent_v[w] = tmpCost;
                        strategypointer_parent_v[w_in_preL] = delta[v_in_preL][w_in_preL];
                    }
                    if (nodeType_R_1[v_in_preL]) {
                        cost_Ipointer_parent_v[w] += cost_Rpointer_parent_v[w];
                        cost_Rpointer_parent_v[w] += cost_Rpointer_v[w] - minCost;
                    }
                    if (nodeType_L_1[v_in_preL]) {
                        cost_Lpointer_parent_v[w] += cost_Lpointer_v[w];
                    } else {
                        cost_Lpointer_parent_v[w] += minCost;
                    }
                }
                if (parent_w_preL != -1) {
                    cost2_R[parent_w_postL] += minCost;
                    tmpCost = -minCost + cost2_I[w];
                    if (tmpCost < cost2_I[parent_w_postL]) {
                        cost2_I[parent_w_postL] = tmpCost;
                        cost2_path[parent_w_postL] = cost2_path[w];
                    }
                    if (nodeType_R_2[w_in_preL]) {
                        cost2_I[parent_w_postL] += cost2_R[parent_w_postL];
                        cost2_R[parent_w_postL] += cost2_R[w] - minCost;
                    }
                    if (nodeType_L_2[w_in_preL]) {
                        cost2_L[parent_w_postL] += cost2_L[w];
                    } else {
                        cost2_L[parent_w_postL] += minCost;
                    }
                }
                delta[v_in_preL][w_in_preL] = strategyPath;
            }

            if (!this->it1->isLeaf(v_in_preL)) {
                fillArray(cost1_L[v], 0.0f);
                fillArray(cost1_R[v], 0.0f);
                fillArray(cost1_I[v], 0.0f);
                rowsToReuse_L.push(cost1_L[v]);
                rowsToReuse_R.push(cost1_R[v]);
                rowsToReuse_I.push(cost1_I[v]);
            }
        }
    }

    void computeOptStrategy_postR() {
        Integer size1 = this->it1->getSize();
        Integer size2 = this->it2->getSize();

        assert(delta.size() == 0);
        delta.resize(size1);
        for (size_t i = 0; i < delta.size(); i++) {
            delta[i].resize(size2);
        }

        std::vector<std::vector<float>> cost1_L(size1);
        std::vector<std::vector<float>> cost1_R(size1);
        std::vector<std::vector<float>> cost1_I(size1);
        std::vector<float> cost2_L(size2);
        std::vector<float> cost2_R(size2);
        std::vector<float> cost2_I(size2);
        std::vector<Integer> cost2_path(size2);
        std::vector<float> leafRow(size2);
        Integer pathIDOffset = size1;
        float minCost = 0x7fffffffffffffffL;
        Integer strategyPath = -1;

        std::vector<Integer> &pre2size1 = this->it1->sizes;
        std::vector<Integer> &pre2size2 = this->it2->sizes;
        std::vector<Integer> &pre2descSum1 = this->it1->preL_to_desc_sum;
        std::vector<Integer> &pre2descSum2 = this->it2->preL_to_desc_sum;
        std::vector<Integer> &pre2krSum1 = this->it1->preL_to_kr_sum;
        std::vector<Integer> &pre2krSum2 = this->it2->preL_to_kr_sum;
        std::vector<Integer> &pre2revkrSum1 = this->it1->preL_to_rev_kr_sum;
        std::vector<Integer> &pre2revkrSum2 = this->it2->preL_to_rev_kr_sum;
        std::vector<Integer> &preL_to_preR_1 = this->it1->preL_to_preR;
        std::vector<Integer> &preL_to_preR_2 = this->it2->preL_to_preR;
        std::vector<Integer> &preR_to_preL_1 = this->it1->preR_to_preL;
        std::vector<Integer> &preR_to_preL_2 = this->it2->preR_to_preL;
        std::vector<Integer> &pre2parent1 = this->it1->parents;
        std::vector<Integer> &pre2parent2 = this->it2->parents;
        std::vector<bool> &nodeType_L_1 = this->it1->nodeType_L;
        std::vector<bool> &nodeType_L_2 = this->it2->nodeType_L;
        std::vector<bool> &nodeType_R_1 = this->it1->nodeType_R;
        std::vector<bool> &nodeType_R_2 = this->it2->nodeType_R;

        Integer size_v,
            size_w,
            parent_v,
            parent_w;
        Integer leftPath_v,
            rightPath_v;
        std::vector<float> cost_Lpointer_v,
                           cost_Rpointer_v,
                           cost_Ipointer_v;
        std::vector<float> cost_Lpointer_parent_v, 
                           cost_Rpointer_parent_v,
                           cost_Ipointer_parent_v;
        std::vector<float> strategypointer_parent_v;
        Integer krSum_v, 
            revkrSum_v,
            descSum_v;
        bool is_v_leaf;

        std::stack<std::vector<float>> rowsToReuse_L;
        std::stack<std::vector<float>> rowsToReuse_R;
        std::stack<std::vector<float>> rowsToReuse_I;

        for(Integer v = size1 - 1; v >= 0; v--) {
            is_v_leaf = this->it1->isLeaf(v);
            parent_v = pre2parent1[v];

            size_v = pre2size1[v];
            leftPath_v = -(preR_to_preL_1[preL_to_preR_1[v] + pre2size1[v] - 1] + 1);// this is the left path's ID which is the leftmost leaf node: l-r_preorder(r-l_preorder(v) + |Fv| - 1)
            rightPath_v = v + pre2size1[v] - 1 + 1; // this is the right path's ID which is the rightmost leaf node: l-r_preorder(v) + |Fv| - 1
            krSum_v = pre2krSum1[v];
            revkrSum_v = pre2revkrSum1[v];
            descSum_v = pre2descSum1[v];

            if (is_v_leaf) {
                cost1_L[v] = leafRow;
                cost1_R[v] = leafRow;
                cost1_I[v] = leafRow;
                for (Integer i = 0; i < size2; i++) {
                    delta[v][i] = v;
                }
            }

            cost_Lpointer_v = cost1_L[v];
            cost_Rpointer_v = cost1_R[v];
            cost_Ipointer_v = cost1_I[v];

            if (parent_v != -1 && cost1_L[parent_v].size() == 0) {
                if (rowsToReuse_L.empty()) {
                    cost1_L[parent_v] = std::vector<float>(size2);
                    cost1_R[parent_v] = std::vector<float>(size2);
                    cost1_I[parent_v] = std::vector<float>(size2);
                } else {
                    cost1_L[parent_v] = rowsToReuse_L.top();
                    rowsToReuse_L.pop();

                    cost1_R[parent_v] = rowsToReuse_R.top();
                    rowsToReuse_R.pop();

                    cost1_I[parent_v] = rowsToReuse_I.top();
                    rowsToReuse_I.pop();
                }
            }

            if (parent_v != -1) {
                cost_Lpointer_parent_v = cost1_L[parent_v];
                cost_Rpointer_parent_v = cost1_R[parent_v];
                cost_Ipointer_parent_v = cost1_I[parent_v];
                strategypointer_parent_v = delta[parent_v];
            }

            fillArray(cost2_L, 0.0f);
            fillArray(cost2_R, 0.0f);
            fillArray(cost2_I, 0.0f);
            fillArray(cost2_path, (Integer)0);

            for (Integer w = size2 - 1; w >= 0; w--) {
                size_w = pre2size2[w];
                if (this->it2->isLeaf(w)) {
                    cost2_L[w] = 0.0f;
                    cost2_R[w] = 0.0f;
                    cost2_I[w] = 0.0f;
                    cost2_path[w] = w;
                }

                minCost = 0x7fffffffffffffffL;
                strategyPath = -1;
                float tmpCost = 0x7fffffffffffffffL;

                if (size_v <= 1 || size_w <= 1) { // USE NEW SINGLE_PATH FUNCTIONS FOR SMALL SUBTREES
                    minCost = std::max(size_v, size_w);
                } else {
                    tmpCost = (float) size_v * (float) pre2krSum2[w] + cost_Lpointer_v[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = leftPath_v;
                    }
                    tmpCost = (float) size_v * (float) pre2revkrSum2[w] + cost_Rpointer_v[w];
                    if (tmpCost < minCost){
                        minCost = tmpCost;
                        strategyPath = rightPath_v;
                    }
                    tmpCost = (float) size_v * (float) pre2descSum2[w] + cost_Ipointer_v[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = (Integer)delta[v][w] + 1;
                    }
                    tmpCost = (float) size_w * (float) krSum_v + cost2_L[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = -(preR_to_preL_2[preL_to_preR_2[w] + size_w - 1] + pathIDOffset + 1);
                    }
                    tmpCost = (float) size_w * (float) revkrSum_v + cost2_R[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = w + size_w - 1 + pathIDOffset + 1;
                    }
                    tmpCost = (float) size_w * (float) descSum_v + cost2_I[w];
                    if (tmpCost < minCost) {
                        minCost = tmpCost;
                        strategyPath = cost2_path[w] + pathIDOffset + 1;
                    }
                }

                if (parent_v != -1) {
                    cost_Lpointer_parent_v[w] += minCost;
                    tmpCost = -minCost + cost1_I[v][w];
                    if (tmpCost < cost1_I[parent_v][w]) {
                        cost_Ipointer_parent_v[w] = tmpCost;
                        strategypointer_parent_v[w] = delta[v][w];
                    }
                    if (nodeType_L_1[v]) {
                        cost_Ipointer_parent_v[w] += cost_Lpointer_parent_v[w];
                        cost_Lpointer_parent_v[w] += cost_Lpointer_v[w] - minCost;
                    }
                    if (nodeType_R_1[v]) {
                        cost_Rpointer_parent_v[w] += cost_Rpointer_v[w];
                    } else {
                        cost_Rpointer_parent_v[w] += minCost;
                    }
                }
                parent_w = pre2parent2[w];
                if (parent_w != -1) {
                    cost2_L[parent_w] += minCost;
                    tmpCost = -minCost + cost2_I[w];
                    if (tmpCost < cost2_I[parent_w]) {
                        cost2_I[parent_w] = tmpCost;
                        cost2_path[parent_w] = cost2_path[w];
                    }
                    if (nodeType_L_2[w]) {
                        cost2_I[parent_w] += cost2_L[parent_w];
                        cost2_L[parent_w] += cost2_L[w] - minCost;
                    }
                    if (nodeType_R_2[w]) {
                        cost2_R[parent_w] += cost2_R[w];
                    } else {
                        cost2_R[parent_w] += minCost;
                    }
                }
                delta[v][w] = strategyPath;
            }

            if (!this->it1->isLeaf(v)) {
                fillArray(cost1_L[v], 0.0f);
                fillArray(cost1_R[v], 0.0f);
                fillArray(cost1_I[v], 0.0f);
                rowsToReuse_L.push(cost1_L[v]);
                rowsToReuse_R.push(cost1_R[v]);
                rowsToReuse_I.push(cost1_I[v]);
            }
        }
    }

    void tedInit() {
        // Reset the subproblems counter.
        counter = 0L;

        // Initialize arrays.
        Integer maxSize = std::max(this->size1, this->size2) + 1;

        // TODO: Move q initialisation to spfA.
        q.resize(maxSize);

        // TODO: Do not use fn and ft arrays [1, Section 8.4].
        fn.resize(maxSize + 1);
        ft.resize(maxSize + 1);

        // Compute subtree distances without the root nodes when one of subtrees
        // is a single node.
        Integer sizeX = -1;
        Integer sizeY = -1;
        // Integer parentX = -1;
        // Integer parentY = -1;

        // Loop over the nodes in reversed left-to-right preorder.
        for(Integer x = 0; x < this->size1; x++) {
            sizeX = this->it1->sizes[x];
            // parentX = this->it1->parents[x];

            for(Integer y = 0; y < this->size2; y++) {
                sizeY = this->it2->sizes[y];
                // parentY = this->it2->parents[y];

                // Set values in delta based on the sums of deletion and insertion
                // costs. Substract the costs for root nodes.
                // In this method we don't have to verify the order of the input trees
                // because it is equal to the original.
                if (sizeX == 1 && sizeY == 1) {
                    delta[x][y] = 0.0f;
                } else if (sizeX == 1) {
                    delta[x][y] = this->it2->preL_to_sumInsCost[y] - this->costModel->insertCost(this->it2->preL_to_node[y]); // USE COST MODEL.
                } else if (sizeY == 1) {
                    delta[x][y] = this->it1->preL_to_sumDelCost[x] - this->costModel->deleteCost(this->it1->preL_to_node[x]); // USE COST MODEL.
                }
            }
        }
    }

    //--------------------------------------------------------------------------

    float gted(NodeIndexer<Data>* it1, NodeIndexer<Data>* it2) {
        Integer currentSubtree1 = it1->getCurrentNode();
        Integer currentSubtree2 = it2->getCurrentNode();
        Integer subtreeSize1 = it1->sizes[currentSubtree1];
        Integer subtreeSize2 = it2->sizes[currentSubtree2];

        // Use spf1.
        if ((subtreeSize1 == 1 || subtreeSize2 == 1)) {
            return spf1(it1, currentSubtree1, it2, currentSubtree2);
        }

        Integer strategyPathID = (Integer)delta[currentSubtree1][currentSubtree2];

        Integer strategyPathType = -1;
        Integer currentPathNode = std::abs(strategyPathID) - 1;
        Integer pathIDOffset = it1->getSize();

        Integer parent = -1;
        if(currentPathNode < pathIDOffset) {
            strategyPathType = getStrategyPathType(strategyPathID, pathIDOffset, it1, currentSubtree1, subtreeSize1);
            while((parent = it1->parents[currentPathNode]) >= currentSubtree1) {
                std::vector<Integer> ai;
                Integer k = (ai = it1->children[parent]).size();
                for(Integer i = 0; i < k; i++) {
                    Integer child = ai[i];
                    if(child != currentPathNode) {
                        it1->setCurrentNode(child);
                        gted(it1, it2);
                    }
                }
                currentPathNode = parent;
            }
            // TODO: Move this property away from node indexer and pass directly to spfs.
            it1->setCurrentNode(currentSubtree1);

            // Pass to spfs a bool that says says if the order of input subtrees
            // has been swapped compared to the order of the initial input trees.
            // Used for accessing delta array and deciding on the edit operation
            // [1, Section 3.4].
            if (strategyPathType == 0) {
                return spfL(it1, it2, false);
            }
            if (strategyPathType == 1) {
                return spfR(it1, it2, false);
            }
            return spfA(it1, it2, std::abs(strategyPathID) - 1, strategyPathType, false);
        }

        currentPathNode -= pathIDOffset;
        strategyPathType = getStrategyPathType(strategyPathID, pathIDOffset, it2, currentSubtree2, subtreeSize2);
        while((parent = it2->parents[currentPathNode]) >= currentSubtree2) {
            std::vector<Integer> ai1;
            Integer l = (ai1 = it2->children[parent]).size();
            for(Integer j = 0; j < l; j++) {
                Integer child = ai1[j];
                if(child != currentPathNode) {
                    it2->setCurrentNode(child);
                    gted(it1, it2);
                }
            }
            currentPathNode = parent;
        }
        // TODO: Move this property away from node indexer and pass directly to spfs.
        it2->setCurrentNode(currentSubtree2);

        // Pass to spfs a bool that says says if the order of input subtrees
        // has been swapped compared to the order of the initial input trees. Used
        // for accessing delta array and deciding on the edit operation
        // [1, Section 3.4].
        if (strategyPathType == 0) {
            return spfL(it2, it1, true);
        }
        if (strategyPathType == 1) {
            return spfR(it2, it1, true);
        }

        return spfA(it2, it1, std::abs(strategyPathID) - pathIDOffset - 1, strategyPathType, true);
    }

public:
    Apted(CostModel<Data>* costModel) : TreeEditDistance<Data>(costModel) {
        // nop
    }

    virtual float computeEditDistance(Node<Data>* t1, Node<Data>* t2) override {
        // Index the nodes of both input trees.
        this->init(t1, t2);

        // Determine the optimal strategy for the distance computation.
        // Use the heuristic from [2, Section 5.3].
        if (this->it1->lchl < this->it1->rchl) {
            computeOptStrategy_postL();
        } else {
            computeOptStrategy_postR();
        }

        // Initialise structures for distance computation.
        tedInit();

        // Compute the distance.
        return gted(this->it1, this->it2);
    }
};

} // namespace capted
