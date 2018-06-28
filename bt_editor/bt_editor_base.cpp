#include "bt_editor_base.h"
#include <QDebug>

AbstractTreeNode *AbsBehaviorTree::rootNode()
{
    if( _root_node_index < 0 ||
        _root_node_index >= static_cast<int>(_nodes.size()) )
    {
        return nullptr;
    }
    else
        return &_nodes.at( _root_node_index );
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

int AbsBehaviorTree::UidToIndex(uint16_t uid) const
{
    return _UID_to_index.at(uid);
}

void AbsBehaviorTree::updateRootIndex()
{
    std::vector<bool> index_has_parent(_nodes.size(), false);

    for(const auto& node: _nodes)
    {
        //qDebug() << "----\nparent node " << node.instance_name ;

        for(int child_index: node.children_index)
        {
//            qDebug() << "node " << nodeAtIndex( child_index )->instance_name <<
//                        " with index " << child_index <<
//                        " has parent ";
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
    if( type == NodeType::ROOT)      return "Root";
    return nullptr;
}

const char *toStr(GraphicMode type)
{
    if( type == GraphicMode::EDITOR)   return "EDITOR";
    if( type == GraphicMode::MONITOR ) return "MONITOR";
    if( type == GraphicMode::REPLAY)   return "REPLAY";
    return nullptr;
}

