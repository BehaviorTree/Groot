#ifndef XMLPARSERS_HPP
#define XMLPARSERS_HPP

#include "tinyXML2/tinyxml2.h"

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>

struct TreeNodeModel
{
  enum class NodeType  { ACTION, DECORATOR, CONTROL, SUBTREE, UNDEFINED } node_type;
  enum class ParamType { INT, DOUBLE, TEXT, COMBO, UNDEFINED };
  QString ID;
  std::map<QString, ParamType> params;
};

typedef std::vector<TreeNodeModel> TreeNodeModels;

void ParseBehaviorTreeXML(const tinyxml2::XMLElement* bt_root, QtNodes::FlowScene* scene, QtNodes::Node& qt_root );

TreeNodeModels ReadTreeNodesModel(QtNodes::DataModelRegistry& registry, const tinyxml2::XMLElement* root);



inline
TreeNodeModel::ParamType getParamTypeFromString(const QString& str)
{
  if( str == "Int")    return TreeNodeModel::ParamType::INT;
  if( str == "Double") return TreeNodeModel::ParamType::DOUBLE;
  if( str == "Combo")  return TreeNodeModel::ParamType::COMBO;
  if( str == "Combo")  return TreeNodeModel::ParamType::TEXT;
  return TreeNodeModel::ParamType::UNDEFINED;
};

inline
TreeNodeModel::NodeType getNodeTypeFromString(const QString& str)
{
  if( str == "Action")    return TreeNodeModel::NodeType::ACTION;
  if( str == "Decorator") return TreeNodeModel::NodeType::DECORATOR;
  if( str == "SubTree")   return TreeNodeModel::NodeType::SUBTREE;
  if( str == "Control")   return TreeNodeModel::NodeType::CONTROL;
  return TreeNodeModel::NodeType::UNDEFINED;
};


inline
const char* toStr(TreeNodeModel::NodeType type)
{
  if( type == TreeNodeModel::NodeType::ACTION ) return   "Action";
  if( type == TreeNodeModel::NodeType::DECORATOR) return "Decorator";
  if( type == TreeNodeModel::NodeType::SUBTREE) return   "SubTree";
  if( type == TreeNodeModel::NodeType::CONTROL) return   "Control";
  return nullptr;
};

inline
const char* toStr(TreeNodeModel::ParamType type)
{
  if( type == TreeNodeModel::ParamType::TEXT)   return "Text";
  if( type == TreeNodeModel::ParamType::INT )   return "Int";
  if( type == TreeNodeModel::ParamType::DOUBLE) return "Double";
  if( type == TreeNodeModel::ParamType::COMBO)  return "Combo";
  return nullptr;
};

//--------------------------------------------------------

const QString gTestXML = R"(

<root main_tree_to_execute = "MainTree" >

    <BehaviorTree ID="MainTree">
        <Fallback name="root_Fallback">

            <Sequence name="door_open_sequence">
                <Action ID="IsDoorOpen" />
                <Action ID="PassThroughDoor" />
            </Sequence>

            <Sequence name="door_closed_sequence">
                <Decorator ID="Negation">
                     <Action ID="IsDoorOpen" />
                </Decorator>
                <Decorator ID="RetryUntilSuccesful" num_attempts="4">
                     <Action ID="OpenDoor"/>
                </Decorator>
                <Action ID="PassThroughDoor" />
                <Action ID="CloseDoor" />
            </Sequence>

            <Action ID="PassThroughWindow" />

        </Fallback>
    </BehaviorTree>

    <!-- TreeNodesModel is used only by the Graphic interface -->
    <TreeNodesModel>
        <Action ID="IsDoorOpen" />
        <Action ID="PassThroughDoor" />
        <Action ID="CloseDoor" />
        <Action ID="OpenDoor" />
        <Action ID="PassThroughWindow" />
        <Decorator ID="Negation" />
        <Decorator ID="RetryUntilSuccesful">
            <Parameter label="num_attempts" type="Int" />
        </Decorator>
        <Decorator ID="Repeat">
            <Parameter label="num_cycles" type="Int" />
        </Decorator>
    </TreeNodesModel>
</root>
        )";


#endif // XMLPARSERS_HPP
