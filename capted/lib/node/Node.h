#pragma once

#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <cassert>
#include "util/int.h"

namespace capted {

//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

template<class Data>
class Node {
private:
    Data* data;
    Node<Data>* parent;
    std::list<Node<Data>*> children;

public:
    Node(Data* data) : data(data), parent(nullptr) {
        // nop
    }

    virtual ~Node() {
        delete data;

        for (Node<Data>* c : children) {
            delete c;
        }
    }

    //-------------------------------------------------------------------------
    // Helpful traversing functions
    //-------------------------------------------------------------------------

    Node<Data>* clone() {
        auto copy = new Node<Data>(cloneData(data));

        for (Node<Data>* child : children) {
            copy->addChild(child->clone());
        }

        return copy;
    }

    void detachFromParent() {
        bool madeChange = false;
        std::list<Node<Data>*> &siblings = parent->children;

        auto iter = siblings.begin();
        while (iter != siblings.end()) {
            if (*iter == this) {
                assert(!madeChange);
                iter = siblings.erase(iter);
                parent = nullptr;
                madeChange = true;
            } else {
                iter++;
            }
        }

        assert(madeChange);
    }

    void replaceChild(Node<Data>* child, Node<Data>* replacement) {
        bool madeChange = false;

        auto iter = children.begin();
        while (iter != children.end()) {
            if (*iter == child) {
                assert(!madeChange);
                assert(!replacement->parent);
                *iter = replacement;
                replacement->parent = this;
                madeChange = true;
            }

            iter++;
        }

        assert(madeChange);
    }

    void dfs(std::function<void(Node<Data>* currentNode, Integer depth)> callback, Integer depth = 0) {
        callback(this, depth);

        for (Node<Data>* child : children) {
            child->dfs(callback, depth + 1);
        }
    }

    //-------------------------------------------------------------------------
    // Getters and setters
    //-------------------------------------------------------------------------

    Data* getData() const {
        return data;
    }

    Integer getNodeCount() const {
        Integer sum = 1;

        for (Node<Data>* c : children) {
            sum += c->getNodeCount();
        }

        return sum;
    }

    Integer getNumChildren() const {
        return children.size();
    }

    std::list<Node<Data>*> &getChildren() {
        return children;
    }

    std::list<Node<Data>*> getChildren() const {
        return children;
    }

    std::vector<Node<Data>*> getChildrenAsVector() const {
        return std::vector<Node<Data>*>(children.begin(), children.end());
    }

    Node<Data>* getIthChild(Integer i) const {
        assert(i >= 0);
        assert(i < (Integer)children.size());

        auto it = children.begin();
        while (i > 0) {
            it++;
            i--;
        }

        return *it;
    }

    Node<Data>* getParent() {
        return parent;
    }

    void setParent(Node<Data>* parent) {
        assert(this->parent == nullptr);
        this->parent = parent;
    }

    void addChild(Node<Data>* child) {
        assert(child);
        assert(!child->parent);

        child->setParent(this);
        children.push_back(child);
    }

    typename std::list<Node<Data>*>::iterator insertChild(typename std::list<Node<Data>*>::iterator destIter, Node<Data>* child) {
        assert(child);
        assert(!child->parent);

        child->setParent(this);
        return children.insert(destIter, child);
    }

    typename std::list<Node<Data>*>::iterator getMyIter() const {
        return std::find(parent->getChildren().begin(), parent->getChildren().end(), this);
    }
};

} // namespace capted
