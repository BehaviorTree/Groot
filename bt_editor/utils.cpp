#include "utils.h"
#include <set>
#include <QDebug>
#include <QDomDocument>
#include <QMessageBox>
#include "nodes/Node"
#include "nodes/DataModelRegistry"
#include "nodes/internal/memory.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

using QtNodes::PortLayout;
using QtNodes::DataModelRegistry;
using QtNodes::Node;
using QtNodes::FlowScene;

QtNodes::Node* findRoot(const QtNodes::FlowScene &scene)
{
    Node* root = nullptr;

    for (auto& it: scene.nodes() )
    {
        Node* node = it.second.get();
        if( node->nodeDataModel()->nPorts( PortType::In ) == 0 )
        {
            if( !root ) root = node;
            else return nullptr;
        }
        else if( node->nodeState().connections( PortType::In, 0 ).empty() )
        {
            if( !root ) root = node;
            else return nullptr;
        }
    }
    return root;
}

std::vector<Node*> getChildren(const QtNodes::FlowScene &scene,
                               const Node& parent_node,
                               bool ordered)
{
    std::vector<Node*> children;

    if( parent_node.nodeDataModel()->nPorts(PortType::Out) == 0)
    {
        return children;
    }

    const auto& conn_out = parent_node.nodeState().connections(PortType::Out, 0);
    children.reserve( conn_out.size() );

    for( auto& it: conn_out)
    {
        auto child_node = it.second->getNode(PortType::In);
        if( child_node )
        {
            children.push_back( child_node );
        }
    }

    if( ordered && children.size() > 1)
    {
        if( scene.layout() == PortLayout::Vertical)
        {
            std::sort(children.begin(), children.end(),
                      [&](const Node* a, const Node* b)
            {
                double pos_a = scene.getNodePosition( *a ).x() + scene.getNodeSize( *a ).width()*0.5;
                double pos_b = scene.getNodePosition( *b ).x() + scene.getNodeSize( *b ).width()*0.5;
                return pos_a < pos_b;
            } );
        }
        else{
            std::sort(children.begin(), children.end(),
                      [&](const Node* a, const Node* b)
            {
                double pos_a = scene.getNodePosition( *a ).y() + scene.getNodeSize( *a ).height()*0.5;
                double pos_b = scene.getNodePosition( *b ).y() + scene.getNodeSize( *b ).height()*0.5;
                return pos_a < pos_b;
            } );
        }
    }
    return children;
}


//---------------------------------------------------

void RecursiveNodeReorder(AbsBehaviorTree& tree, PortLayout layout)
{
    std::function<void(unsigned, AbstractTreeNode*)> recursiveStep;

    std::vector<QPointF> layer_cursor;
    std::vector< std::vector<AbstractTreeNode*> > nodes_by_level(1);

    const qreal LEVEL_SPACING = 80;
    const qreal NODE_SPACING  = 40;

    recursiveStep = [&](unsigned current_layer, AbstractTreeNode* node)
    {
        node->pos = layer_cursor[current_layer];
        nodes_by_level[current_layer].push_back( node );

        //------------------------------------
        if( node->children_index.size() == 0 )
        {
            return;
        }
        qreal recommended_pos = NODE_SPACING * 0.5;

        current_layer++;

        if( layout == PortLayout::Vertical)
        {
            recommended_pos += (node->pos.x() + node->size.width()*0.5)  ;
            for(int index: node->children_index)
            {
                const auto& child_node = tree.nodes()[index];
                recommended_pos -=  (child_node.size.width() + NODE_SPACING) * 0.5;
            }

            if( current_layer >= layer_cursor.size())
            {
                QPointF new_cursor( recommended_pos,
                                    node->pos.y() + LEVEL_SPACING );
                layer_cursor.push_back( new_cursor );
                nodes_by_level.push_back( std::vector<AbstractTreeNode*>() );
            }
            else{
                recommended_pos = std::max( recommended_pos, layer_cursor[current_layer].x() );
                layer_cursor[current_layer].setX(recommended_pos);
            }
        }//------------------------------------
        else
        {
            recommended_pos += node->pos.y() + node->size.height()*0.5;
            for(int index: node->children_index)
            {
                const auto& child_node = tree.nodes()[index];
                recommended_pos -=  (child_node.size.height() + NODE_SPACING) * 0.5;
            }

            if( current_layer >= layer_cursor.size())
            {
                QPointF new_cursor( node->pos.x() + LEVEL_SPACING,
                                    recommended_pos);
                layer_cursor.push_back( new_cursor );
                nodes_by_level.push_back( std::vector<AbstractTreeNode*>() );
            }
            else{
                recommended_pos = std::max( recommended_pos, layer_cursor[current_layer].y() );
                layer_cursor[current_layer].setY(recommended_pos);
            }
        }
        //------------------------------------

        auto initial_pos = layer_cursor[current_layer];

        for(int index: node->children_index)
        {
            AbstractTreeNode* child_node = tree.node(index);

            recursiveStep(current_layer, child_node);
            if( layout == PortLayout::Vertical)
            {
                layer_cursor[current_layer]+= QPointF( child_node->size.width() + NODE_SPACING, 0 );
            }
            else{
                layer_cursor[current_layer]+= QPointF( 0, child_node->size.height() + NODE_SPACING );
            }
        }

        auto final_pos = layer_cursor[current_layer];

        if( node->children_index.size() > 0)
        {
            auto first_child = tree.node( node->children_index.front() );
            auto last_child = tree.node( node->children_index.back() );

            initial_pos = QPointF( first_child->pos.x() + first_child->size.width()*0.5,
                                   first_child->pos.y() + first_child->size.height()*0.5);

            final_pos   = QPointF( last_child->pos.x() + last_child->size.width()*0.5,
                                   last_child->pos.y() + last_child->size.height()*0.5);
        }

        // rebalance father
        QPointF pos_offset(0,0);
        if( layout == PortLayout::Vertical)
        {
            double new_x = (final_pos.x() + initial_pos.x()) * 0.5 - node->size.width()*0.5;
            double diff = node->pos.x() - new_x;
            pos_offset = QPointF( - diff, 0 );
        }
        else{
            double new_y = (final_pos.y() + initial_pos.y()) * 0.5 - node->size.height()*0.5;
            double diff = node->pos.y() - new_y;
            pos_offset = QPointF( 0, - diff );
        }

        node->pos += pos_offset;
        layer_cursor[current_layer -1 ] += pos_offset;
    };

    auto root_node = tree.rootNode();
    layer_cursor.push_back(QPointF( -root_node->size.width()*0.5,
                                    -root_node->size.height()*0.5));
    recursiveStep(0, root_node);

    if( layout == PortLayout::Vertical)
    {
        qreal offset = root_node->size.height() + LEVEL_SPACING;
        for(unsigned i=1; i< nodes_by_level.size(); i++ )
        {
            qreal max_height = 0;
            for(auto node: nodes_by_level[i])
            {
                max_height = std::max( max_height, node->size.height() );
                node->pos.setY( offset );
            }
            offset += max_height + LEVEL_SPACING;
        }
    }
    else{
        qreal offset = root_node->size.width() + LEVEL_SPACING;
        for(unsigned i=1; i< nodes_by_level.size(); i++ )
        {
            qreal max_width = 0;
            for(auto node: nodes_by_level[i])
            {
                max_width = std::max( max_width, node->size.width() );
                node->pos.setX( offset );
            }
            offset += max_width + LEVEL_SPACING;
        }
    }
}

void NodeReorder(QtNodes::FlowScene &scene, AbsBehaviorTree & tree)
{

    for (const auto& abs_node: tree.nodes())
    {
        Node* node =  abs_node.graphic_node;
        if( node == nullptr )
        {
            throw std::runtime_error("one or more nodes haven't been created yet");
        }
    }

    if( tree.nodesCount() == 0)
    {
        return;
    }

    RecursiveNodeReorder(tree, scene.layout() );

    for (const auto& abs_node: tree.nodes())
    {
        Node* node =  abs_node.graphic_node;
        scene.setNodePosition( *node, abs_node.pos );
    }
}


AbsBehaviorTree BuildTreeFromScene(const QtNodes::FlowScene *scene,
                                   QtNodes::Node* root_node)
{
    if(!root_node )
    {
        root_node = findRoot( *scene );
    }
    if( !root_node )
    {
        if( scene->nodes().size() != 0)
        {
            qDebug() << "Error: can not create a tree from a scene unless it has a single root node ";
        }
        return AbsBehaviorTree();
    }

    AbsBehaviorTree tree;

    std::function<void(AbstractTreeNode*, QtNodes::Node*)> pushRecursively;

    pushRecursively = [&](AbstractTreeNode* parent, QtNodes::Node* node)
    {
        AbstractTreeNode abs_node;

        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>(node->nodeDataModel());

        abs_node.model = bt_model->model();
        abs_node.instance_name = bt_model->instanceName();
        abs_node.pos  = scene->getNodePosition(*node) ;
        abs_node.size = scene->getNodeSize(*node);
        abs_node.graphic_node = node;
        abs_node.ports_mapping = bt_model->getCurrentPortMapping();

        auto added_node = tree.addNode( parent, std::move(abs_node) );

        auto children = getChildren( *scene, *node, true );

        for(auto& child_node: children )
        {
            pushRecursively( added_node, child_node );
        }
    };

    pushRecursively( nullptr, root_node );

    return tree;
}

AbsBehaviorTree BuildTreeFromXML(const QDomElement& bt_root, const NodeModels& models )
{
    AbsBehaviorTree tree;

    if( bt_root.tagName() != "BehaviorTree" )
    {
        throw std::runtime_error( "Expecting a node called <BehaviorTree>");
    }

    //-------------------------------------
    std::function<void(AbstractTreeNode* parent, QDomElement)> recursiveStep;
    recursiveStep = [&](AbstractTreeNode* parent, QDomElement xml_node)
    {
        // The nodes with a ID used that QString to insert into the registry()
        QString modelID = xml_node.tagName();
        if( xml_node.hasAttribute("ID") )
        {
            modelID = xml_node.attribute("ID");
        }

        AbstractTreeNode tree_node;

        auto model_it = models.find(modelID);
        if( model_it ==  models.end() )
        {
             throw std::runtime_error( (QString("This model has not been registered: ") + modelID).toStdString() );
        }
        tree_node.model = model_it->second;

        if( xml_node.hasAttribute("name") )
        {
            tree_node.instance_name = ( xml_node.attribute("name") );
        }
        else{
            tree_node.instance_name = modelID;
        }

        auto attributes = xml_node.attributes();
        for( int attr=0; attr < attributes.size(); attr++ )
        {
            auto attribute = attributes.item(attr).toAttr();
            if( attribute.name() != "ID" && attribute.name() != "name")
            {
                tree_node.ports_mapping.insert( { attribute.name(), attribute.value() } );
            }
        }

        auto added_node = tree.addNode(parent, std::move(tree_node));

        for (QDomElement  child = xml_node.firstChildElement( )  ;
             !child.isNull();
             child = child.nextSiblingElement( ) )
        {
            recursiveStep( added_node, child );
        }
    };

    auto first_child = bt_root.firstChildElement();
    if( first_child.tagName() == "Root")
    {
        QMessageBox::question(nullptr,
                              "Fix your file!",
                              "Please remove the node <Root> from your <BehaviorTree>",
                              QMessageBox::Ok );
        first_child = first_child.firstChildElement();
    }

    // start recursion
    recursiveStep( nullptr, first_child );

    return tree;
}


std::pair<AbsBehaviorTree, std::unordered_map<int, int>>
BuildTreeFromFlatbuffers(const Serialization::BehaviorTree *fb_behavior_tree)
{
    AbsBehaviorTree tree;
    std::unordered_map<int, int> uid_to_index;

    AbstractTreeNode abs_root;
    abs_root.instance_name = "Root";
    abs_root.model.registration_ID = "Root";
    abs_root.model.registration_ID = "Root";
    abs_root.children_index.push_back( 1 );

    tree.addNode( nullptr, std::move(abs_root) );

    //-----------------------------------------
    NodeModels models;

    for( const Serialization::NodeModel* model_node: *(fb_behavior_tree->node_models()) )
    {
        NodeModel model;
        model.registration_ID = model_node->registration_name()->c_str();
        model.type = convert( model_node->type() );

        for( const Serialization::PortModel* port: *(model_node->ports()) )
        {
            PortModel port_model;
            QString port_name = port->port_name()->c_str();
            port_model.direction = convert( port->direction() );
            port_model.type_name = port->type_info()->c_str();
            port_model.description = port->description()->c_str();

            model.ports.insert( { port_name, std::move(port_model) } );
        }

        models.insert( { model.registration_ID, std::move(model)} );
    }

    //-----------------------------------------
    for( const Serialization::TreeNode* fb_node: *(fb_behavior_tree->nodes()) )
    {
        AbstractTreeNode abs_node;
        abs_node.instance_name = fb_node->instance_name()->c_str();
        const char* registration_ID = fb_node->registration_name()->c_str();
        abs_node.status = convert( fb_node->status() );
        abs_node.model = (models.at(registration_ID));

        for( const Serialization::PortConfig* pair: *(fb_node->port_remaps()) )
        {
            abs_node.ports_mapping.insert( { QString(pair->port_name()->c_str()),
                                             QString(pair->remap()->c_str()) } );
        }
        int index = tree.nodesCount();
        abs_node.index = index;
        tree.nodes().push_back( std::move(abs_node) );
        uid_to_index.insert( { fb_node->uid(), index} );
    }

    for(size_t index = 0; index < fb_behavior_tree->nodes()->size(); index++ )
    {
        const Serialization::TreeNode* fb_node = fb_behavior_tree->nodes()->Get(index);
        AbstractTreeNode* abs_node = tree.node( index + 1);
        for( const auto child_uid: *(fb_node->children_uid()) )
        {
            int child_index = uid_to_index[ child_uid ];
            abs_node->children_index.push_back(child_index);
        }
    }
    return { tree, uid_to_index };
}

std::pair<QtNodes::NodeStyle, QtNodes::ConnectionStyle>
getStyleFromStatus(NodeStatus status, NodeStatus prev_status)
{
    QtNodes::NodeStyle  node_style;
    QtNodes::ConnectionStyle conn_style;

    float penWidth = 3.0;

    conn_style.HoveredColor = Qt::transparent;

    //printf("status=%d, old=%d\n", status, prev_status);

    if( status == NodeStatus::IDLE )
    {
        if(prev_status != NodeStatus::IDLE){
            node_style.PenWidth *= penWidth;
            node_style.HoveredPenWidth = node_style.PenWidth;

            if( prev_status == NodeStatus::SUCCESS )
            {
                node_style.NormalBoundaryColor =
                        node_style.ShadowColor = QColor(100, 150, 100);
                conn_style.NormalColor = node_style.NormalBoundaryColor;
            }
            else if( prev_status == NodeStatus::RUNNING )
            {
                node_style.NormalBoundaryColor =
                        node_style.ShadowColor =  QColor(150, 130, 40);
                conn_style.NormalColor = node_style.NormalBoundaryColor;
            }
            else if( prev_status == NodeStatus::FAILURE )
            {
                node_style.NormalBoundaryColor =
                        node_style.ShadowColor = QColor(150, 80, 80);
                conn_style.NormalColor = node_style.NormalBoundaryColor;
            }
        }

        return {node_style, conn_style};
    }

    node_style.PenWidth *= penWidth;
    node_style.HoveredPenWidth = node_style.PenWidth;

    if( status == NodeStatus::SUCCESS )
    {
        node_style.NormalBoundaryColor =
                node_style.ShadowColor = QColor(51, 200, 51);
                node_style.ShadowColor = QColor(51, 250, 51);
        conn_style.NormalColor = node_style.NormalBoundaryColor;
    }
    else if( status == NodeStatus::RUNNING )
    {
        node_style.NormalBoundaryColor =
                node_style.ShadowColor =  QColor(220, 140, 20);
        conn_style.NormalColor = node_style.NormalBoundaryColor;
    }
    else if( status == NodeStatus::FAILURE )
    {
        node_style.NormalBoundaryColor =
                node_style.ShadowColor = QColor(250, 50, 50);
        conn_style.NormalColor = node_style.NormalBoundaryColor;
    }

    return {node_style, conn_style};
}

QtNodes::Node *GetParentNode(QtNodes::Node *node)
{
    using namespace QtNodes;
    auto conn_in = node->nodeState().connections(PortType::In, 0);
    if( conn_in.size() == 0)
    {
        return nullptr;
    }
    else{
        return conn_in.begin()->second->getNode(PortType::Out);
    }
}

std::set<QString> GetModelsToRemove(QWidget* parent,
                                    NodeModels& prev_models,
                                    const NodeModels& new_models)
{
    std::set<QString> prev_custom_models;

    if( prev_models.size() > BuiltinNodeModels().size() )
    {
        for(const auto& it: prev_models)
        {
            const QString& model_name = it.first;
            if( BuiltinNodeModels().count(model_name) == 0 &&
                new_models.count(model_name) == 0)
            {
                prev_custom_models.insert( model_name );
            }
        }
    }

    if( prev_custom_models.size() > 0 )
    {
        int ret = QMessageBox::question(parent, "Clear Palette?",
                                        "Do you want to remove the previously loaded custom nodes?",
                                        QMessageBox::No | QMessageBox::Yes );
        if( ret == QMessageBox::No)
        {
            prev_custom_models.clear();
        }
    }
    return prev_custom_models;
}

BT::NodeType convert(Serialization::NodeType type)
{
    switch (type)
    {
    case Serialization::NodeType::ACTION:
        return BT::NodeType::ACTION;
    case Serialization::NodeType::DECORATOR:
        return BT::NodeType::DECORATOR;
    case Serialization::NodeType::CONTROL:
        return BT::NodeType::CONTROL;
    case Serialization::NodeType::CONDITION:
        return BT::NodeType::CONDITION;
    case Serialization::NodeType::SUBTREE:
        return BT::NodeType::SUBTREE;
    case Serialization::NodeType::UNDEFINED:
        return BT::NodeType::UNDEFINED;
    }
    return BT::NodeType::UNDEFINED;
}

BT::NodeStatus convert(Serialization::NodeStatus type)
{
    switch (type)
    {
    case Serialization::NodeStatus::IDLE:
        return BT::NodeStatus::IDLE;
    case Serialization::NodeStatus::SUCCESS:
        return BT::NodeStatus::SUCCESS;
    case Serialization::NodeStatus::RUNNING:
        return BT::NodeStatus::RUNNING;
    case Serialization::NodeStatus::FAILURE:
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::IDLE;
}

BT::PortDirection convert(Serialization::PortDirection direction)
{
    switch (direction)
    {
    case Serialization::PortDirection::INPUT :
        return BT::PortDirection::INPUT;
    case Serialization::PortDirection::OUTPUT:
        return BT::PortDirection::OUTPUT;
    case Serialization::PortDirection::INOUT:
        return BT::PortDirection::INOUT;
    }
    return BT::PortDirection::INOUT;
}
