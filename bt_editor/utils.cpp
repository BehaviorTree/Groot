#include "utils.h"
#include <nodes/Node>
#include <nodes/DataModelRegistry>
#include <QDebug>
#include <set>
#include <QMessageBox>
#include "nodes/internal/memory.hpp"
#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"
#include "tinyXML2/tinyxml2.h"

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

    if( ordered )
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
    layer_cursor.push_back(QPointF(0,0));
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

        // rebalance father
        if( layout == PortLayout::Vertical)
        {
            double diff = (node->pos.x() + node->size.width()*0.5) - (final_pos.x() + initial_pos.x() - NODE_SPACING)* 0.5 ;
            node->pos += QPointF( - diff, 0 );
            layer_cursor[current_layer -1 ] += QPointF( - diff, 0 );
        }
        else{
            double diff = (node->pos.y() + node->size.height()*0.5) - (final_pos.y() + initial_pos.y() - NODE_SPACING)* 0.5 ;
            node->pos += QPointF( 0, - diff );
            layer_cursor[current_layer -1 ] += QPointF( 0, - diff );
        }
    };

    auto root_node = tree.rootNode();
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
    if( tree.nodesCount() == 0)
    {
        return;
    }

    RecursiveNodeReorder(tree, scene.layout() );

    for (const auto& abs_node: tree.nodes())
    {
        Node& node = *( abs_node.graphic_node);
        scene.setNodePosition( node, abs_node.pos );
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

        abs_node.instance_name     = bt_model->instanceName();
        abs_node.model.registration_ID = bt_model->registrationName();
        abs_node.pos  = scene->getNodePosition(*node) ;
        abs_node.size = scene->getNodeSize(*node);
        abs_node.graphic_node = node;
        abs_node.model.params = bt_model->getCurrentParameters();
        abs_node.model.type = bt_model->nodeType();

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


QString getCategory(const NodeDataModel *dataModel)
{
    QString category;
    if( dynamic_cast<const ActionNodeModel*>(dataModel) )
    {
        category = "Action";
    }
    else if( dynamic_cast<const DecoratorNodeModel*>(dataModel) )
    {
        category = "Decorator";
    }
    else if( dynamic_cast<const ControlNodeModel*>(dataModel) )
    {
        category = "Control";
    }
    else if( dynamic_cast<const RootNodeModel*>(dataModel) )
    {
        category = "Root";
    }
    else if( dynamic_cast<const SubtreeNodeModel*>(dataModel) )
    {
        category = "SubTree";
    }
    return category;
}


AbsBehaviorTree BuildTreeFromXML(const tinyxml2::XMLElement* bt_root )
{
    using namespace tinyxml2;
    AbsBehaviorTree tree;

    if( strcmp( bt_root->Name(), "BehaviorTree" ) != 0)
    {
        throw std::runtime_error( "expecting a node called <BehaviorTree>");
    }

    std::function<void(AbstractTreeNode* parent, const XMLElement*)> recursiveStep;

    recursiveStep = [&](AbstractTreeNode* parent, const XMLElement* xml_node)
    {
        // The nodes with a ID used that QString to insert into the registry()
        QString modelID = xml_node->Name();
        if( xml_node->Attribute("ID") )
        {
            modelID = xml_node->Attribute("ID");
        }

        AbstractTreeNode tree_node;

        tree_node.model.registration_ID = modelID;
        tree_node.model.type = getNodeTypeFromString( xml_node->Name() );

        if( xml_node->Attribute("name") )
        {
            tree_node.instance_name = ( xml_node->Attribute("name") );
        }
        else{
            tree_node.instance_name = modelID;
        }

        for( const XMLAttribute* attribute= xml_node->FirstAttribute();
             attribute != nullptr;
             attribute = attribute->Next() )
        {
            const QString attr_name( attribute->Name() );
            if( attr_name!= "ID" && attr_name != "name")
            {
                tree_node.model.params.push_back( { attr_name, attribute->Value() } );
            }
        }

        auto added_node = tree.addNode(parent, std::move(tree_node));

        for (const XMLElement*  child = xml_node->FirstChildElement( )  ;
             child != nullptr;
             child = child->NextSiblingElement( ) )
        {
            recursiveStep( added_node, child );
        }
    };

    // start recursion
    recursiveStep( nullptr, bt_root->FirstChildElement() );

    return tree;
}


AbsBehaviorTree BuildTreeFromFlatbuffers(const BT_Serialization::BehaviorTree *fb_behavior_tree)
{
    AbsBehaviorTree tree;

    AbstractTreeNode abs_root;
    abs_root.instance_name = "Root";
    abs_root.model.registration_ID = "Root";
    abs_root.model.type = NodeType::ROOT;
    abs_root.children_index.push_back( 1 );

    tree.addNode( nullptr, std::move(abs_root) );

    std::unordered_map<int, int> uid_to_index;

    // just copy the vector
    for( const BT_Serialization::TreeNode* fb_node: *(fb_behavior_tree->nodes()) )
    {
        AbstractTreeNode abs_node;
        abs_node.instance_name = fb_node->instance_name()->c_str();
        abs_node.model.registration_ID = fb_node->registration_name()->c_str();
        abs_node.model.type   = convert( fb_node->type() );
        abs_node.status = convert( fb_node->status() );

        for( const BT_Serialization::KeyValue* pair: *(fb_node->params()) )
        {
            abs_node.model.params.push_back( { QString(pair->key()->c_str()),
                                             QString(pair->value()->c_str()) } );
        }
        int index = tree.nodesCount();
        abs_node.index = index;
        tree.nodes().push_back( std::move(abs_node) );
        uid_to_index.insert( { fb_node->uid(), index} );
    }

    for(size_t index = 0; index < fb_behavior_tree->nodes()->size(); index++ )
    {
        const BT_Serialization::TreeNode* fb_node = fb_behavior_tree->nodes()->Get(index);
        AbstractTreeNode* abs_node = tree.node( index + 1);
        for( const auto child_uid: *(fb_node->children_uid()) )
        {
            int child_index = uid_to_index[ child_uid ];
            abs_node->children_index.push_back(child_index);
        }
    }
    return tree;
}

std::pair<QtNodes::NodeStyle, QtNodes::ConnectionStyle>
getStyleFromStatus(NodeStatus status)
{
    QtNodes::NodeStyle  node_style;
    QtNodes::ConnectionStyle conn_style;

    conn_style.HoveredColor = Qt::transparent;

    if( status == NodeStatus::IDLE )
    {
        return {node_style, conn_style};
    }

    node_style.PenWidth *= 3.0;
    node_style.HoveredPenWidth = node_style.PenWidth;

    if( status == NodeStatus::SUCCESS )
    {
        node_style.NormalBoundaryColor =
                node_style.ShadowColor = QColor(51, 200, 51);
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

ParameterWidgetCreator buildWidgetCreator(const TreeNodeModel::Param& param)
{
    ParameterWidgetCreator creator;
    creator.label = param.label;

    creator.instance_factory = [param]()
    {
        QLineEdit* line = new QLineEdit();
        line->setAlignment( Qt::AlignHCenter);
        line->setMaximumWidth(140);
        line->setText( param.value );
        return line;
    };

    return creator;
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

void CleanPreviousModels(QWidget *parent,
                         TreeNodeModels &prev_models,
                         const TreeNodeModels &new_models)
{
    std::set<const QString *> prev_custom_models;

    if( prev_models.size() > BuiltinNodeModels().size() )
    {
        for(const auto& it: prev_models)
        {
            if( BuiltinNodeModels().count(it.first) == 0)
            {
                prev_custom_models.insert( &it.first );
            }
        }
    }

    for( const auto& name: prev_custom_models)
    {
        if( new_models.count( *name ) == 0)
        {
            int ret = QMessageBox::question(parent, "Clear Palette?",
                                            "Do yoy want to remove the previously loaded custom nodes?",
                                            QMessageBox::No | QMessageBox::Yes );
            if( ret == QMessageBox::Yes)
            {
                prev_models = BuiltinNodeModels();
            }
            break;
        }
    }
}
