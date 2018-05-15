#ifndef XMLPARSERS_HPP
#define XMLPARSERS_HPP

#include "tinyXML2/tinyxml2.h"

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>

enum class NodeType  { ACTION, DECORATOR, CONTROL, SUBTREE, UNDEFINED };
enum class ParamType { INT, DOUBLE, TEXT, COMBO, UNDEFINED };

struct TreeNodeModel
{
  NodeType node_type;
  std::map<QString, ParamType> params;
};

typedef std::map<QString,TreeNodeModel> TreeNodeModels;

void ParseBehaviorTreeXML(const tinyxml2::XMLElement* bt_root, QtNodes::FlowScene* scene, QtNodes::Node& qt_root );

void ReadTreeNodesModel(const tinyxml2::XMLElement* root,
                        QtNodes::DataModelRegistry& registry,
                        TreeNodeModels& models_list);

void RecursivelyCreateXml(const QtNodes::FlowScene &scene,
                          tinyxml2::XMLDocument& doc,
                          tinyxml2::XMLElement* parent_element,
                          const QtNodes::Node* node);



inline
ParamType getParamTypeFromString(const QString& str)
{
  if( str == "Int")    return ParamType::INT;
  if( str == "Double") return ParamType::DOUBLE;
  if( str == "Combo")  return ParamType::COMBO;
  if( str == "Combo")  return ParamType::TEXT;
  return ParamType::UNDEFINED;
};

inline
NodeType getNodeTypeFromString(const QString& str)
{
  if( str == "Action")    return NodeType::ACTION;
  if( str == "Decorator") return NodeType::DECORATOR;
  if( str == "SubTree")   return NodeType::SUBTREE;
  if( str == "Control")   return NodeType::CONTROL;
  return NodeType::UNDEFINED;
};


inline
const char* toStr(NodeType type)
{
  if( type == NodeType::ACTION )   return   "Action";
  if( type == NodeType::DECORATOR) return "Decorator";
  if( type == NodeType::SUBTREE)   return   "SubTree";
  if( type == NodeType::CONTROL)   return   "Control";
  return nullptr;
};

inline
const char* toStr(ParamType type)
{
  if( type == ParamType::TEXT)   return "Text";
  if( type == ParamType::INT )   return "Int";
  if( type == ParamType::DOUBLE) return "Double";
  if( type == ParamType::COMBO)  return "Combo";
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
