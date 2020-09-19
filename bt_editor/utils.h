#ifndef NODE_UTILS_H
#define NODE_UTILS_H

#include <QDomDocument>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/NodeStyle>

#include "bt_editor_base.h"
#include <behaviortree_cpp_v3/flatbuffers/BT_logger_generated.h>
#include <behaviortree_cpp_v3/flatbuffers/bt_flatbuffer_helper.h>

QtNodes::Node* findRoot(const QtNodes::FlowScene &scene);

std::vector<QtNodes::Node *> getChildren(const QtNodes::FlowScene &scene,
                                         const QtNodes::Node &parent_node,
                                         bool ordered);

AbsBehaviorTree BuildTreeFromScene(const QtNodes::FlowScene *scene,
                                   QtNodes::Node *root_node = nullptr);

std::pair<AbsBehaviorTree, std::unordered_map<int, int> >
BuildTreeFromFlatbuffers(const Serialization::BehaviorTree* bt );

AbsBehaviorTree BuildTreeFromXML(const QDomElement &bt_root, const NodeModels &models);

void NodeReorder(QtNodes::FlowScene &scene, AbsBehaviorTree &abstract_tree );

std::pair<QtNodes::NodeStyle, QtNodes::ConnectionStyle>
getStyleFromStatus(NodeStatus status, NodeStatus prev_status);

QtNodes::Node* GetParentNode(QtNodes::Node* node);

std::set<QString> GetModelsToRemove(QWidget* parent,
                                    NodeModels& prev_models,
                                    const NodeModels& new_models);

BT::NodeType convert( Serialization::NodeType type);

BT::NodeStatus convert(Serialization::NodeStatus type);

BT::PortDirection convert(Serialization::PortDirection direction);


#endif // NODE_UTILS_H
