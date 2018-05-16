#ifndef NODE_UTILS_H
#define NODE_UTILS_H

#include <nodes/NodeData>
#include <nodes/FlowScene>


std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene);

std::vector<QtNodes::Node *> getChildren(const QtNodes::FlowScene &scene,
                                         const QtNodes::Node &parent_node);

void NodeReorder(QtNodes::FlowScene &scene);


QString getCategory(const QtNodes::NodeDataModel* model);

#endif // NODE_UTILS_H
