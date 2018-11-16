#ifndef XMLPARSERS_HPP
#define XMLPARSERS_HPP

#include "tinyXML2/tinyxml2.h"
#include "bt_editor_base.h"

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>


TreeNodeModels ReadTreeNodesModel(const tinyxml2::XMLElement* root);

void RecursivelyCreateXml(const QtNodes::FlowScene &scene,
                          tinyxml2::XMLDocument& doc,
                          tinyxml2::XMLElement* parent_element,
                          const QtNodes::Node* node);

bool VerifyXML(tinyxml2::XMLDocument& doc,
               const std::vector<QString> &registered_ID,
               std::vector<QString> &error_messages);

TreeNodeModel buildTreeNodeModel(const tinyxml2::XMLElement* node,
                                 bool is_tree_node_model);


#endif // XMLPARSERS_HPP
