#pragma once

#include <vector>
#include <iostream>
#include "util/debug.h"
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// Node Indexer
//------------------------------------------------------------------------------

/**
 * Indexes nodes of the input tree to the algorithm that is already parsed to
 * tree structure using {@link node.Node} class. Stores various indices on
 * nodes required for efficient computation of APTED [1,2]. Additionally, it
 * stores single-value properties of the tree.
 *
 * <p>For indexing we use four tree traversals that assign ids to the nodes:
 * <ul>
 * <li>left-to-right preorder [1],
 * <li>right-to-left preorder [1],
 * <li>left-to-right postorder [2],
 * <li>right-to-left postorder [2].
 * </ul>
 *
 * <p>See the source code for more algorithm-related comments.
 *
 * <p>References:
 * <ul>
 * <li>[1] M. Pawlik and N. Augsten. Efficient Computation of the Tree Edit
 *      Distance. ACM Transactions on Database Systems (TODS) 40(1). 2015.
 * <li>[2] M. Pawlik and N. Augsten. Tree edit distance: Robust and memory-
 *      efficient. Information Systems 56. 2016.
 * </ul>
 *
 * @param <D> type of node data.
 * @param <C> type of cost model.
 * @see node.Node
 * @see parser.InputParser
 */

template <class NodeData>
class AllPossibleMappings;

template <class NodeData>
class Apted;

template<class Data>
class NodeIndexer {
private:
    typedef Node<Data> N;

    friend AllPossibleMappings<Data>;
    friend Apted<Data>;

    const CostModel<Data>* costModel;
    const Integer treeSize;

    // Structure indices
    std::vector<Integer> sizes;
    std::vector<Integer> parents;
    std::vector<std::vector<Integer>> children;

    std::vector<Integer> postL_to_lld;
    std::vector<Integer> postR_to_rld;
    std::vector<Integer> preL_to_ln;
    std::vector<Integer> preR_to_ln;

    std::vector<N*> preL_to_node;
    std::vector<bool> nodeType_L;
    std::vector<bool> nodeType_R;

    // Traversal translation indices
    std::vector<Integer> preL_to_preR;
    std::vector<Integer> preR_to_preL;
    std::vector<Integer> preL_to_postL;
    std::vector<Integer> preL_to_postR;
    std::vector<Integer> postL_to_preL;
    std::vector<Integer> postR_to_preL;

    // Cost indices
    std::vector<Integer> preL_to_kr_sum;
    std::vector<Integer> preL_to_rev_kr_sum;
    std::vector<Integer> preL_to_desc_sum;
    std::vector<float> preL_to_sumDelCost;
    std::vector<float> preL_to_sumInsCost;

    // Temp variables
    Integer currentNode;
    Integer lchl;
    Integer rchl;
    Integer sizeTmp;
    Integer descSizesTmp;
    Integer krSizesSumTmp;
    Integer revkrSizesSumTmp;
    Integer preorderTmp;

    Integer indexNodes(N* node, Integer postorder) {
        // Initialise variables.
        Integer currentSize = 0;
        Integer childrenCount = 0;
        Integer descSizes = 0;
        Integer krSizesSum = 0;
        Integer revkrSizesSum = 0;
        Integer preorder = preorderTmp;
        Integer preorderR = 0;
        Integer currentPreorder = -1;

        // Store the preorder id of the current node to use it after the recursion.
        preorderTmp++;

        // Loop over children of a node.
        std::vector<N*> childNodes = node->getChildrenAsVector();
        for (size_t i = 0; i < childNodes.size(); i++) {
            childrenCount++;
            currentPreorder = preorderTmp;
            parents[currentPreorder] = preorder;

            // Execute method recursively for next child.
            postorder = indexNodes(childNodes[i], postorder);
            children[preorder].push_back(currentPreorder);

            currentSize += 1 + sizeTmp;
            descSizes += descSizesTmp;

            if(childrenCount > 1) {
                krSizesSum += krSizesSumTmp + sizeTmp + 1;
            } else {
                krSizesSum += krSizesSumTmp;
                nodeType_L[currentPreorder] = true;
            }

            if (i < childNodes.size() - 1) {
                revkrSizesSum += revkrSizesSumTmp + sizeTmp + 1;
            } else {
                revkrSizesSum += revkrSizesSumTmp;
                nodeType_R[currentPreorder] = true;
            }
        }

        postorder++;

        Integer currentDescSizes = descSizes + currentSize + 1;

        Integer temp_mul = (currentSize + 1) * (currentSize + 1 + 3);
        if (__builtin_mul_overflow((currentSize + 1), (currentSize + 1 + 3), &temp_mul)) {
            printf("Overflow in %s::%d\n", __FILE__, __LINE__);
            exit(1);
        }

        preL_to_desc_sum[preorder] = (temp_mul) / 2 - currentDescSizes;
        preL_to_kr_sum[preorder] = krSizesSum + currentSize + 1;
        preL_to_rev_kr_sum[preorder] = revkrSizesSum + currentSize + 1;

        // Store pointer to a node object corresponding to preorder.
        preL_to_node[preorder] = node;

        sizes[preorder] = currentSize + 1;
        preorderR = treeSize - 1 - postorder;
        preL_to_preR[preorder] = preorderR;
        preR_to_preL[preorderR] = preorder;

        descSizesTmp = currentDescSizes;
        sizeTmp = currentSize;
        krSizesSumTmp = krSizesSum;
        revkrSizesSumTmp = revkrSizesSum;

        postL_to_preL[postorder] = preorder;
        preL_to_postL[preorder] = postorder;
        preL_to_postR[preorder] = treeSize-1-preorder;
        postR_to_preL[treeSize-1-preorder] = preorder;

        return postorder;
    }

    void postTraversalIndexing() {
        Integer currentLeaf = -1;
        Integer nodeForSum = -1;
        Integer parentForSum = -1;

        for(Integer i = 0; i < treeSize; i++) {
            preL_to_ln[i] = currentLeaf;
            if(isLeaf(i)) {
                currentLeaf = i;
            }

            // This block stores leftmost leaf descendants for each node
            // indexed in postorder. Used for mapping computation.
            // Added by Victor.
            Integer postl = i; // Assume that the for loop iterates postorder.
            Integer preorder = postL_to_preL[i];
            if (sizes[preorder] == 1) {
                postL_to_lld[postl] = postl;
            } else {
                postL_to_lld[postl] = postL_to_lld[preL_to_postL[children[preorder][0]]];
            }
            // This block stores rightmost leaf descendants for each node
            // indexed in right-to-left postorder.
            // [TODO] Use postL_to_lld and postR_to_rld instead of APTED.getLLD
            //        and APTED.gerRLD methods, remove these method.
            //        Result: faster lookup of these values.
            Integer postr = i; // Assume that the for loop iterates reversed postorder.
            preorder = postR_to_preL[postr];
            if (sizes[preorder] == 1) {
                postR_to_rld[postr] = postr;
            } else {
                postR_to_rld[postr] = postR_to_rld[preL_to_postR[children[preorder][children[preorder].size() - 1]]];
            }
            // Count lchl and rchl.
            // [TODO] There are no values for parent node.
            if (sizes[i] == 1) {
                Integer parent = parents[i];
                if (parent > -1) {
                    if (parent+1 == i) {
                        lchl++;
                    } else if (preL_to_preR[parent]+1 == preL_to_preR[i]) {
                        rchl++;
                    }
                }
            }

            // Sum up costs of deleting and inserting entire subtrees.
            // Reverse the node index. Here, we need traverse nodes bottom-up.
            nodeForSum = treeSize - i - 1;
            parentForSum = parents[nodeForSum];
            // Update myself.
            preL_to_sumDelCost[nodeForSum] += costModel->deleteCost(preL_to_node[nodeForSum]);
            preL_to_sumInsCost[nodeForSum] += costModel->insertCost(preL_to_node[nodeForSum]);
            if (parentForSum > -1) {
                // Update my parent.
                preL_to_sumDelCost[parentForSum] += preL_to_sumDelCost[nodeForSum];
                preL_to_sumInsCost[parentForSum] += preL_to_sumInsCost[nodeForSum];
            }
        }

        currentLeaf = -1;
        // [TODO] Merge with the other loop. Assume different traversal.
        for(Integer i = 0; i < sizes[0]; i++) {
            preR_to_ln[i] = currentLeaf;
            if(isLeaf(preR_to_preL[i])) {
                currentLeaf = i;
            }
        }
    }

public:
    NodeIndexer(N* inputTree, const CostModel<Data>* costModel)
    : costModel(costModel)
    , treeSize(inputTree->getNodeCount()) {
        // Initialize tmp variables
        currentNode = 0;
        lchl = 0;
        rchl = 0;
        sizeTmp = 0;
        descSizesTmp = 0;
        krSizesSumTmp = 0;
        revkrSizesSumTmp = 0;
        preorderTmp = 0;

        // Initialize indices
        sizes.resize(treeSize, 0);
        children.resize(treeSize);
        parents.resize(treeSize, 0); parents[0] = -1; // Root has no parent

        postL_to_lld.resize(treeSize, 0);
        postR_to_rld.resize(treeSize, 0);
        preL_to_ln.resize(treeSize, 0);
        preR_to_ln.resize(treeSize, 0);

        preL_to_node.resize(treeSize, nullptr);
        nodeType_L.resize(treeSize, false);
        nodeType_R.resize(treeSize, false);

        preL_to_preR.resize(treeSize, 0);
        preR_to_preL.resize(treeSize, 0);
        preL_to_postL.resize(treeSize, 0);
        preL_to_postR.resize(treeSize, 0);
        postL_to_preL.resize(treeSize, 0);
        postR_to_preL.resize(treeSize, 0);

        preL_to_kr_sum.resize(treeSize, 0);
        preL_to_rev_kr_sum.resize(treeSize, 0);
        preL_to_desc_sum.resize(treeSize, 0);
        preL_to_sumDelCost.resize(treeSize, 0.0f);
        preL_to_sumInsCost.resize(treeSize, 0.0f);

        // Index
        indexNodes(inputTree, -1);
        postTraversalIndexing();
    }

    Integer getSize() {
        return treeSize;
    }

    Integer preL_to_lld(Integer preL) {
        return postL_to_preL[postL_to_lld[preL_to_postL[preL]]];
    }

    Integer preL_to_rld(Integer preL) {
        return postR_to_preL[postR_to_rld[preL_to_postR[preL]]];
    }

    Node<Data>* postL_to_node(Integer postL) {
        return preL_to_node[postL_to_preL[postL]];
    }

    Node<Data>* postR_to_node(Integer postR) {
        return preL_to_node[postR_to_preL[postR]];
    }

    bool isLeaf(Integer nodeId) {
        return sizes[nodeId] == 1;
    }

    Integer getCurrentNode() const {
        return currentNode;
    }

    void setCurrentNode(Integer preorder) {
        currentNode = preorder;
    }

    void dump() {
        std::cerr << std::string(80, '-') << std::endl;
        std::cerr << "sizes: "              << arrayToString(sizes)              << std::endl;
        std::cerr << "preL_to_preR: "       << arrayToString(preL_to_preR)       << std::endl;
        std::cerr << "preR_to_preL: "       << arrayToString(preR_to_preL)       << std::endl;
        std::cerr << "preL_to_postL: "      << arrayToString(preL_to_postL)      << std::endl;
        std::cerr << "postL_to_preL: "      << arrayToString(postL_to_preL)      << std::endl;
        std::cerr << "preL_to_postR: "      << arrayToString(preL_to_postR)      << std::endl;
        std::cerr << "postR_to_preL: "      << arrayToString(postR_to_preL)      << std::endl;
        std::cerr << "postL_to_lld: "       << arrayToString(postL_to_lld)       << std::endl;
        std::cerr << "postR_to_rld: "       << arrayToString(postR_to_rld)       << std::endl;
        std::cerr << "preL_to_node: "       << arrayToString(preL_to_node)       << std::endl;
        std::cerr << "preL_to_ln: "         << arrayToString(preL_to_ln)         << std::endl;
        std::cerr << "preR_to_ln: "         << arrayToString(preR_to_ln)         << std::endl;
        std::cerr << "preL_to_kr_sum: "     << arrayToString(preL_to_kr_sum)     << std::endl;
        std::cerr << "preL_to_rev_kr_sum: " << arrayToString(preL_to_rev_kr_sum) << std::endl;
        std::cerr << "preL_to_desc_sum: "   << arrayToString(preL_to_desc_sum)   << std::endl;
        std::cerr << "preL_to_sumDelCost: " << arrayToString(preL_to_sumDelCost) << std::endl;
        std::cerr << "preL_to_sumInsCost: " << arrayToString(preL_to_sumInsCost) << std::endl;
        std::cerr << "children: "           << arrayToString(children)           << std::endl;
        std::cerr << "nodeType_L: "         << arrayToString(nodeType_L)         << std::endl;
        std::cerr << "nodeType_R: "         << arrayToString(nodeType_R)         << std::endl;
        std::cerr << "parents: "            << arrayToString(parents)            << std::endl;
        std::cerr << std::string(80, '-') << std::endl;
    }
};

}
