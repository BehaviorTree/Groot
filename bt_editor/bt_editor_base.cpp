#include "bt_editor_base.h"


AbstractTreeNode *AbsBehaviorTree::rootNode()
{
    if( _root_node_index < 0)
        return nullptr;
    else
        return &_nodes.at( _root_node_index );
}

const AbstractTreeNode *AbsBehaviorTree::rootNode() const
{
    if( _root_node_index < 0)
        return nullptr;
    else
        return &_nodes.at( _root_node_index );
}

void AbsBehaviorTree::pushBack(uint16_t UID, AbstractTreeNode node)
{
    node.index = _nodes.size();
    _UID_to_index.insert( {UID, node.index} );
    _nodes.push_back( std::move(node) );
}

int AbsBehaviorTree::UidToIndex(uint16_t uid) const
{
    return _UID_to_index.at(uid);
}

void AbsBehaviorTree::updateRootIndex()
{
    std::vector<bool> index_has_parent(_nodes.size(), false);

    for(const auto& node: _nodes)
    {
        for(int child_index: node.children_index)
        {
            index_has_parent[ child_index ] = true;
        }
    }
    int root_count = 0;
    for(size_t index = 0; index < index_has_parent.size(); index++)
    {
        if (! index_has_parent[index] )
        {
            root_count++;
            _root_node_index = index;
        }
    }
    if( root_count != 1)
    {
        throw std::logic_error("Malformed AbsBehaviorTree");
    }
}

ParamType getParamTypeFromString(const QString &str)
{
    if( str == "Text")  return ParamType::TEXT;
    if( str == "Int")    return ParamType::INT;
    if( str == "Double") return ParamType::DOUBLE;
    if( str == "Combo")  return ParamType::COMBO;
    return ParamType::UNDEFINED;
}

NodeType getNodeTypeFromString(const QString &str)
{
    if( str == "Action")    return NodeType::ACTION;
    if( str == "Decorator") return NodeType::DECORATOR;
    if( str == "Condition") return NodeType::CONDITION;
    if( str == "SubTree")   return NodeType::SUBTREE;
    if( str == "Control")   return NodeType::CONTROL;
    if( str == "Root")   return NodeType::ROOT;
    return NodeType::UNDEFINED;
}

GraphicMode getGraphicModeFromString(const QString &str)
{
    if( str == "EDITOR")
        return GraphicMode::EDITOR;
    else if( str == "MONITOR")
        return GraphicMode::MONITOR;
    return GraphicMode::REPLAY;
}

const char *toStr(NodeStatus type)
{
    if( type == NodeStatus::IDLE )   return "IDLE";
    if( type == NodeStatus::RUNNING) return "RUNNING";
    if( type == NodeStatus::SUCCESS) return "SUCCESS";
    if( type == NodeStatus::FAILURE) return "FAILURE";

    return nullptr;
}

const char *toStr(NodeType type)
{
    if( type == NodeType::ACTION )   return "Action";
    if( type == NodeType::DECORATOR) return "Decorator";
    if( type == NodeType::CONDITION) return "Condition";
    if( type == NodeType::SUBTREE)   return "SubTree";
    if( type == NodeType::CONTROL)   return "Control";
    if( type == NodeType::CONTROL)   return "Root";
    return nullptr;
}

const char *toStr(GraphicMode type)
{
    if( type == GraphicMode::EDITOR)   return "EDITOR";
    if( type == GraphicMode::MONITOR ) return "MONITOR";
    if( type == GraphicMode::REPLAY)   return "REPLAY";
    return nullptr;
}

const char *toStr(ParamType type)
{
    if( type == ParamType::TEXT)   return "Text";
    if( type == ParamType::INT )   return "Int";
    if( type == ParamType::DOUBLE) return "Double";
    if( type == ParamType::COMBO)  return "Combo";
    return nullptr;
}
