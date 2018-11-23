#include "bt_editor_base.h"
#include <QDebug>

void AbsBehaviorTree::clear()
{
    _nodes.resize(0);
}

AbsBehaviorTree::~AbsBehaviorTree()
{
    clear();
}

AbstractTreeNode *AbsBehaviorTree::rootNode()
{
    if( _nodes.empty() ) return nullptr;
    return &_nodes.front();
}

const AbstractTreeNode *AbsBehaviorTree::rootNode() const
{
    if( _nodes.empty() ) return nullptr;
    return &_nodes.front();
}


std::vector<const AbstractTreeNode*> AbsBehaviorTree::findNodes(const QString &instance_name)
{
    std::vector<const AbstractTreeNode*> out;
    out.reserve( 4 );

    for( const auto& node: _nodes)
    {
        if( node.instance_name == instance_name)
        {
            out.push_back( &node );
        }
    }
    return out;
}

const AbstractTreeNode* AbsBehaviorTree::findFirstNode(const QString &instance_name)
{
    for( const auto& node: _nodes)
    {
        if( node.instance_name == instance_name)
        {
            return ( &node );
        }
    }
    return nullptr;
}



AbstractTreeNode* AbsBehaviorTree::addNode(AbstractTreeNode* parent, AbstractTreeNode && new_node )
{
    int index = _nodes.size();
    new_node.index = index;
    if( parent )
    {
        _nodes.push_back( std::move(new_node) );
        parent->children_index.push_back( index );
    }
    else{
        _nodes.clear();
        _nodes.push_back(new_node);
    }
    return &_nodes.back();
}

void AbsBehaviorTree::debugPrint() const
{
    if( !rootNode() )
    {
        qDebug() << "Empty AbsBehaviorTree";
        return;
    }
    std::function<void(const AbstractTreeNode*,int)> recursiveStep;

    recursiveStep = [&](const AbstractTreeNode* node, int indent)
    {
        for(int i=0; i< indent; i++) printf("    ");

        printf("%s (%s)",
               node->instance_name.toStdString().c_str(),
               node->model.registration_ID.toStdString().c_str() );
        std::cout << std::endl; // force flush

        for(int index: node->children_index)
        {
            auto child_node = &_nodes[index];
            recursiveStep( child_node, indent+1);
        }
    };

    recursiveStep( rootNode(), 0 );

}

bool AbsBehaviorTree::operator ==(const AbsBehaviorTree &other) const
{
    if( _nodes.size() != other._nodes.size() ) return false;

    for (size_t index = 0; index < _nodes.size(); index++)
    {
        if( _nodes[index] != other._nodes[index]) return false;
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
    return  model == other.model &&
            status == other.status &&
            size == other.size &&
            pos == other.pos &&
            instance_name == other.instance_name;
}

bool TreeNodeModel::operator ==(const TreeNodeModel &other) const
{
    bool is_same = ( type == other.type &&
                     params.size() == other.params.size() &&
                     registration_ID == other.registration_ID);
    if( ! is_same ) return false;

    for (size_t index = 0; index < params.size(); index++)
    {
        if( params[index].label != other.params[index].label ||
            params[index].value != other.params[index].value ) return false;
    }
    return true;
}
