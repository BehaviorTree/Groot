#ifndef NODE_UTILS_H
#define NODE_UTILS_H

#include <QDomDocument>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/NodeStyle>

#include "bt_editor_base.h"

QtNodes::Node* findRoot(const QtNodes::FlowScene &scene);

std::vector<QtNodes::Node *> getChildren(const QtNodes::FlowScene &scene,
                                         const QtNodes::Node &parent_node,
                                         bool ordered);

AbsBehaviorTree BuildTreeFromScene(const QtNodes::FlowScene *scene,
                                   QtNodes::Node *root_node = nullptr);

AbsBehaviorTree BuildTreeFromXML(const QDomElement &bt_root, const NodeModels &models);

void NodeReorder(QtNodes::FlowScene &scene, AbsBehaviorTree &abstract_tree );

std::pair<QtNodes::NodeStyle, QtNodes::ConnectionStyle>
getStyleFromStatus(NodeStatus status, NodeStatus prev_status);

QtNodes::Node* GetParentNode(QtNodes::Node* node);

std::set<QString> GetModelsToRemove(QWidget* parent,
                                    NodeModels& prev_models,
                                    const NodeModels& new_models);

#endif // NODE_UTILS_H
