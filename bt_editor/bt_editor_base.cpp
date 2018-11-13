#include "bt_editor_base.h"
#include <QDebug>

AbstractTreeNode *AbsBehaviorTree::rootNode()
{
    return nodeAtIndex( _root_node_index );
}

int AbsBehaviorTree::UidToIndex(uint16_t uid) const
{
    auto it = _UID_to_index.find(uid);
    if( it == _UID_to_index.end() )
        return -1;
    else
        return it->second;
}

AbstractTreeNode *AbsBehaviorTree::nodeAtIndex(int16_t index) {
    return &_nodes.at( index );
}
const AbstractTreeNode *AbsBehaviorTree::nodeAtIndex(int16_t index) const{
    return &_nodes.at( index );
}

AbstractTreeNode *AbsBehaviorTree::nodeAtUID(uint16_t uid)
{
    int index = UidToIndex(uid);
    return nodeAtIndex(index);
}
const AbstractTreeNode *AbsBehaviorTree::nodeAtUID(uint16_t uid) const{
    int index = UidToIndex(uid);
    return nodeAtIndex(index);
}

const AbstractTreeNode *AbsBehaviorTree::findNode(const QString &instance_name)
{
    for( const auto& node: _nodes)
    {
        if( node.instance_name == instance_name)
        {
            return &node;
        }
    }
    return nullptr;
}

const AbstractTreeNode *AbsBehaviorTree::rootNode() const
{
    if( _root_node_index < 0 ||
        _root_node_index >= static_cast<int>(_nodes.size()) )
    {
        return nullptr;
    }
    else{
        return &_nodes.at( _root_node_index );
    }
}

void AbsBehaviorTree::pushBack(uint16_t UID, AbstractTreeNode node)
{
    node.index = _nodes.size();
    if( _UID_to_index.count(UID) > 0 )
    {
        throw std::logic_error("Duplicated UID in AbsBehaviorTree::pushBack");
    }
    _UID_to_index.insert( {UID, node.index} );
    _nodes.push_back( std::move(node) );
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
        for(size_t index = 0; index < index_has_parent.size(); index++)
        {
            if (! index_has_parent[index] )
            {
                qDebug() << nodeAtIndex(index)->instance_name << " is root ?";
            }
        }
        qDebug() << "Malformed AbsBehaviorTree";
    }
}

void AbsBehaviorTree::debugPrint()
{
  std::function<void(const AbstractTreeNode*,int)> recursiveStep;

  recursiveStep = [&](const AbstractTreeNode* node, int indent)
  {
    for(int i=0; i< indent; i++) printf("    ");

    printf("%s (%s)\n",
           node->instance_name.toStdString().c_str(),
           node->registration_name.toStdString().c_str() );

    for(auto& child_index: node->children_index)
    {
      recursiveStep( nodeAtIndex(child_index), indent+1);
    }
  };

  if(  !rootNode() )
  {
      printf("AbsBehaviorTree has root %d and size of nodes vector %d\n",
             rootIndex(), (int)_nodes.size() );
  }
  else {
    recursiveStep( rootNode(), 0 );
  }
}

bool AbsBehaviorTree::operator ==(const AbsBehaviorTree &other) const
{
    if( _nodes.size() != other._nodes.size() ) return false;
    if( _UID_to_index.size() != other._UID_to_index.size() ) return false;

    for (const auto& it: _UID_to_index)
    {
        uint16_t uid   = it.first;
        uint16_t index = it.second;
        auto nodeA = nodeAtIndex(index);
        auto nodeAB = other.nodeAtUID(uid);
        if( *nodeA != *nodeAB ) return false;
    }
    return true;
}

NodeType getNodeTypeFromString(const QString &str)
{
    if( str == "Action")    return NodeType::ACTION;
    if( str == "Decorator") return NodeType::DECORATOR;
    if( str == "Condition") return NodeType::CONDITION;
    if( str == "SubTree")   return NodeType::SUBTREE;
    if( str == "Control")   return NodeType::CONTROL;
    if( str == "Root")      return NodeType::ROOT;
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
    if( type == NodeType::ROOT)      return "Root";
    return "Undefined";
}

const char *toStr(GraphicMode type)
{
    if( type == GraphicMode::EDITOR)   return "EDITOR";
    if( type == GraphicMode::MONITOR ) return "MONITOR";
    if( type == GraphicMode::REPLAY)   return "REPLAY";
    return nullptr;
}


bool AbstractTreeNode::operator ==(const AbstractTreeNode &other) const
{
    bool is_same =
            registration_name == other.registration_name &&
            instance_name == other.instance_name &&
            type == other.type &&
            status == other.status &&
            size == other.size &&
            pos == other.pos &&
            corresponding_node == other.corresponding_node;

    if(!is_same) return false;

    if( parameters.size() != other.parameters.size() ) return false;
    for (size_t index = 0; index < parameters.size(); index++)
    {
        if( parameters[index] != other.parameters[index]) return false;
    }
    return true;
}
