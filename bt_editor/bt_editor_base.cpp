#include "bt_editor_base.h"
#include <behaviortree_cpp_v3/decorators/subtree_node.h>
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



AbstractTreeNode* AbsBehaviorTree::addNode(AbstractTreeNode* parent,
                                           AbstractTreeNode && new_node )
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
    bool same_registration = model.registration_ID == other.model.registration_ID;
    return  same_registration &&
            status == other.status &&
            size == other.size &&
          // temporary removed  pos == other.pos &&
            instance_name == other.instance_name;
}

bool NodeModel::operator ==(const NodeModel &other) const
{
    bool is_same = ( type == other.type &&
                    ports.size() == other.ports.size() &&
                    registration_ID == other.registration_ID);
    if( ! is_same ) return false;

    auto other_it = other.ports.begin();
    for (const auto& port_it: ports)
    {
        if( port_it.first  != other_it->first)
        {
            return false;
        }
        if( port_it.second.direction  != other_it->second.direction )
        {
            return false;
        }
        if( port_it.second.type_name  != other_it->second.type_name )
        {
            return false;
        }
        other_it++;
    }
    return true;
}

NodeModel &NodeModel::operator =(const BT::TreeNodeManifest &src)
{
    this->type = src.type;
    this->registration_ID = QString::fromStdString(src.registration_ID);
    for (const auto& port_it: src.ports)
    {
        const auto& port_name = port_it.first;
        const auto& bt_port = port_it.second;
        PortModel port_model;
        port_model = bt_port;
        this->ports.insert( { QString::fromStdString(port_name), std::move(port_model) } );
    }
    return *this;
}


const NodeModels &BuiltinNodeModels()
{
    static NodeModels builtin_node_models =
            []() -> NodeModels
    {
        BT::BehaviorTreeFactory factory;

        factory.registerNodeType<BT::SubtreeNode>("Root");

        NodeModels out;
        for( const auto& it: factory.manifests())
        {
            const auto& model_name = it.first;
            if( model_name == "SubTree" || model_name == "SubTreePlus" )
            {
                continue;
            }
            const auto& bt_model = it.second;
            NodeModel groot_model;
            groot_model = bt_model;
            out.insert( { QString::fromStdString(model_name), std::move(groot_model) });
        }
        return out;
     }();

    return builtin_node_models;
}

PortModel &PortModel::operator =(const BT::PortInfo &src)
{
    this->direction = src.direction();
    this->description = QString::fromStdString(src.description());
    this->type_name = QString::fromStdString(BT::demangle(src.type()));
    this->default_value = QString::fromStdString( src.defaultValue());
    return *this;
}
