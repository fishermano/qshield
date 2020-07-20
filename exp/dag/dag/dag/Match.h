#ifndef DAG_MATCH_H
#define DAG_MATCH_H

#include "DirectedAcyclicGraph.h"

#include <tuple>

namespace DAG{

  template <typename N>
  using Edge = std::tuple<const N*, const N*, bool>;
  template <typename N>
  using Edgeset = std::unordered_set<Edge<N>*>;

  template <typename N>
    class MatchVisitor : public BFSVisitor<N> {
    public:
      bool hasMatch(const DAG::Nodeset<N>& nodes, DAG::Edgeset<N>& edges);
    };

  template <typename N>
  bool MatchVisitor<N>::hasMatch(const DAG::Nodeset<N>& nodes, DAG::Edgeset<N>& edges) {

    std::queue<const N*> nodeQueue;

    // check whether all edges are ending with the same node, i.e., current operator
    int i = 0;
    int l = -1;
    for (auto edge : edges) {
      if (i == 0){
        l = std::get<1>(*edge)->value();
        i++;
        continue;
      }
      if (l != std::get<1>(*edge)->value()){
        return false;
      }
    }

    // traverse the DAG to find the node corresponding to current
    for (auto node : nodes){
      if(BFSVisitor<N>::m_visited.find(node) == BFSVisitor<N>::m_visited.end()){

        if(node->value() != l){
          node->accept(*this);
          nodeQueue.push(node);
          continue;
        }

        for (auto child : node->children()){
          for(auto edge : edges){
            if(std::get<0>(*edge)->value() == child->value()){
              std::get<2>(*edge) = true;
            }
          }
        }

        bool isEmpty = true;
        for (auto edge : edges){
          isEmpty = isEmpty && std::get<2>(*edge);
        }

        if (isEmpty){
          return true;
        }else{
          return false;
        }
      }
    }

    while (!nodeQueue.empty()){

      for (auto node : nodeQueue.front()->children()){
        if(BFSVisitor<N>::m_visited.find(node) == BFSVisitor<N>::m_visited.end()){
          if(node->value() != l){
            node->accept(*this);
            nodeQueue.push(node);
            continue;
          }

          for (auto child : node->children()){

            for(auto const& edge : edges){
              if(std::get<0>(*edge)->value() == child->value()){
                std::get<2>(*edge) = true;
              }
            }
          }


          bool isEmpty = true;
          for (auto edge : edges){
            isEmpty = isEmpty && std::get<2>(*edge);
          }

          if (isEmpty){
            return true;
          }else{
            return false;
          }

        }
      }

      nodeQueue.pop();
    }

    return false;

  }
}

#endif//DAG_MATCH_H
