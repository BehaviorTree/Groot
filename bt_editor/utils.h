#ifndef NODE_UTILS_H
#define NODE_UTILS_H

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include "bt_editor_base.h"

std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene);

std::vector<QtNodes::Node *> getChildren(const QtNodes::FlowScene &scene,
                                         const QtNodes::Node &parent_node);

BehaviorTree BuildBehaviorTreeFromScene(const QtNodes::FlowScene* scene);

void BuildSceneFromBehaviorTree(QtNodes::FlowScene *scene , BehaviorTree& abstract_tree);

void NodeReorder(QtNodes::FlowScene &scene, BehaviorTree &abstract_tree );

QString getCategory(const QtNodes::NodeDataModel* model);

#endif // NODE_UTILS_H
