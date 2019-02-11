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
               node->model.registration_ID.c_str() );
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



GraphicMode getGraphicModeFromString(const QString &str)
{
    if( str == "EDITOR")
        return GraphicMode::EDITOR;
    else if( str == "MONITOR")
        return GraphicMode::MONITOR;
    return GraphicMode::REPLAY;
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
    return  model.registration_ID == other.model.registration_ID &&
            status == other.status &&
            size == other.size &&
            pos == other.pos &&
            instance_name == other.instance_name;
}

// FIXME VER_3
//bool BT_NodeModel::operator ==(const BT_NodeModel &other) const
//{
//    bool is_same = ( type == other.type &&
//                     ports.size() == other.ports.size() &&
//                     registration_ID == other.registration_ID);
//    if( ! is_same ) return false;

//    for (size_t index = 0; index < ports.size(); index++)
//    {
//        if( ports[index]. != other.ports[index].label ||
//            ports[index].value != other.ports[index].value ) return false;
//    }
//    return true;
//}
