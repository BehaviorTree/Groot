#include "utils.h"
#include <nodes/Node>
#include <nodes/DataModelRegistry>
#include <QDebug>
#include <set>

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

using QtNodes::PortLayout;
using QtNodes::DataModelRegistry;

std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene)
{
    std::set<QUuid> roots;
    for (auto& it: scene.nodes() ){
        roots.insert( it.first );
    }

    for (auto it: scene.connections())
    {
        std::shared_ptr<QtNodes::Connection> connection = it.second;
        const QtNodes::Node* child  = connection->getNode( QtNodes::PortType::In );

        if( child ) {
            roots.erase( child->id() );
        }
    }

    std::vector<QtNodes::Node*> sorted_roots;
    for (auto uuid: roots)
    {
        sorted_roots.push_back( scene.nodes().find(uuid)->second.get() );
    }

    auto CompareFunction = [&](const QtNodes::Node* a, const QtNodes::Node* b)
    {
        return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
    };
    std::sort(sorted_roots.begin(), sorted_roots.end(), CompareFunction );

    return sorted_roots;
}

std::vector<QtNodes::Node*> getChildren(const QtNodes::FlowScene &scene,
                                        const QtNodes::Node& parent_node)
{
    std::vector<QtNodes::Node*> children;

    for (auto it: scene.connections())
    {
        std::shared_ptr<QtNodes::Connection> connection = it.second;

        QtNodes::Node* parent = connection->getNode( QtNodes::PortType::Out );
        QtNodes::Node* child  = connection->getNode( QtNodes::PortType::In );

        if( parent && child )
        {
            if( parent->id() == parent_node.id())
            {
                children.push_back( child );
            }
        }
    }

    if( scene.layout() == PortLayout::Vertical)
    {
        std::sort(children.begin(), children.end(),
                  [&](const QtNodes::Node* a, const QtNodes::Node* b)
        {
            return scene.getNodePosition( *a ).x() <  scene.getNodePosition( *b ).x();
        } );
    }
    else{
        std::sort(children.begin(), children.end(),
                  [&](const QtNodes::Node* a, const QtNodes::Node* b)
        {
            return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
        } );
    }

    return children;
}


//---------------------------------------------------

void RecursiveNodeReorder(AbsBehaviorTree & tree, AbstractTreeNode* root_node, PortLayout layout)
{
    std::function<void(unsigned, AbstractTreeNode*)> recursiveStep;

    std::vector<QPointF> layer_cursor;
    std::vector< std::vector<AbstractTreeNode*> > nodes_by_level(1);
    layer_cursor.push_back(QPointF(0,0));
    const qreal LEVEL_SPACING = 100;
    const qreal NODE_SPACING  = 40;

    recursiveStep = [&](unsigned current_layer, AbstractTreeNode* node)
    {
        node->pos = layer_cursor[current_layer];
        nodes_by_level[current_layer].push_back( node );

        //------------------------------------
        if( node->children_uid.size() > 0 )
        {
            qreal recommended_pos = NODE_SPACING * 0.5;

            current_layer++;

            if( layout == PortLayout::Vertical)
            {
                recommended_pos += (node->pos.x() + node->size.width()*0.5)  ;
                for(uint16_t child_uid: node->children_uid)
                {
                    AbstractTreeNode* child = &(tree.nodes.at( child_uid ));
                    recommended_pos -=  (child->size.width() + NODE_SPACING) * 0.5;
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
                for(uint16_t child_uid: node->children_uid)
                {
                    AbstractTreeNode* child = &(tree.nodes.at( child_uid ));
                    recommended_pos -=  (child->size.height() + NODE_SPACING) * 0.5;
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

            for(uint16_t child_uid: node->children_uid)
            {
                AbstractTreeNode* child = &(tree.nodes.at( child_uid ));

                recursiveStep(current_layer, child);
                if( layout == PortLayout::Vertical)
                {
                    layer_cursor[current_layer]+= QPointF( child->size.width() + NODE_SPACING, 0 );
                }
                else{
                    layer_cursor[current_layer]+= QPointF( 0, child->size.height() + NODE_SPACING );
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
        }
    };

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
    if( tree.root_node_uid == 0)
    {
        return;
    }

    AbstractTreeNode* root_node = &( tree.nodes.at( tree.root_node_uid ) );
    RecursiveNodeReorder(tree, root_node, scene.layout() );

    for (auto& it: tree.nodes)
    {
        QtNodes::Node& node = *(it.second.corresponding_node);
        scene.setNodePosition( node, it.second.pos );
    }
}


AbsBehaviorTree BuildBehaviorTreeFromScene(const QtNodes::FlowScene *scene)
{
    auto roots = findRoots( *scene );
    if( roots.size() != 1)
    {
        return AbsBehaviorTree();
    }

    AbsBehaviorTree tree;
    auto bt_root = dynamic_cast<BehaviorTreeDataModel*>(roots.front()->nodeDataModel());
    tree.root_node_uid = bt_root->UID();

    for (const auto& it: scene->nodes() )
    {
        QtNodes::Node* node = (it.second.get());
        AbstractTreeNode abs_node;

        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>(node->nodeDataModel());

        abs_node.instance_name     = bt_model-> instanceName();
        abs_node.registration_name = bt_model-> registrationName();
        abs_node.pos  = scene->getNodePosition(*node) ;
        abs_node.size = scene->getNodeSize(*node);
        abs_node.uid  = bt_model->UID();
        abs_node.corresponding_node = node;
        abs_node.parameters = bt_model->getCurrentParameters();

        tree.nodes.insert( { abs_node.uid, abs_node} );
    }

    for (auto& it: tree.nodes)
    {
        auto& abs_node = it.second;
        QtNodes::Node* node = abs_node.corresponding_node;

        auto children = getChildren( *scene, *node );
        abs_node.children_uid.reserve( children.size() );

        for(auto& child: children )
        {
            auto bt_model = dynamic_cast<BehaviorTreeDataModel*>(child->nodeDataModel());
            abs_node.children_uid.push_back( bt_model->UID() );
        }
    }
    return tree;
}




QString getCategory(const QtNodes::NodeDataModel *dataModel)
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


void BuildSceneFromBehaviorTree(QtNodes::FlowScene* scene, AbsBehaviorTree &abstract_tree)
{
    QPointF cursor(0,0);
    double x_offset = 0;

    std::function<void(AbstractTreeNode*, QtNodes::Node*, int)> recursiveStep;

    recursiveStep = [&](AbstractTreeNode* abs_node, QtNodes::Node* parent_node, int nest_level)
    {
        std::unique_ptr<NodeDataModel> dataModel = scene->registry().create( abs_node->registration_name );

        if (!dataModel)
        {
            QString ID = abs_node->registration_name;

            if(  abs_node->type == NodeType::ACTION || abs_node->type == NodeType::CONDITION)
            {
                DataModelRegistry::RegistryItemCreator node_creator = [ID]()
                {
                    return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID, ParameterWidgetCreators() ) );
                };
                scene->registry().registerModel("Action", node_creator);
            }
            else if( abs_node->type == NodeType::DECORATOR )
            {
                DataModelRegistry::RegistryItemCreator node_creator = [ID]()
                {
                    return std::unique_ptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, ParameterWidgetCreators()) );
                };
                scene->registry().registerModel("Decorator", node_creator);
            }
            else if( abs_node->type == NodeType::SUBTREE )
            {
                DataModelRegistry::RegistryItemCreator node_creator = [ID]()
                {
                    return std::unique_ptr<SubtreeNodeModel>( new SubtreeNodeModel(ID, ParameterWidgetCreators()) );
                };
                scene->registry().registerModel("SubTree", node_creator);
            }
            dataModel = scene->registry().create( abs_node->registration_name );
        }

        if (!dataModel){
            char buffer[250];
            sprintf(buffer, "No registered model with name: [%s](%s)",
                    abs_node->registration_name.toStdString().c_str(),
                    abs_node->instance_name.toStdString().c_str() );
            throw std::runtime_error( buffer );
        }

        BehaviorTreeDataModel* bt_node = dynamic_cast<BehaviorTreeDataModel*>( dataModel.get() );

        bt_node->setInstanceName( abs_node->instance_name );
        bt_node->setUID( abs_node->uid );

        for (auto& it: abs_node->parameters)
        {
            bt_node->setParameterValue( it.first, it.second );
        }

        cursor.setY( cursor.y() + 65);
        cursor.setX( nest_level * 400 + x_offset );

        QtNodes::Node& new_node = scene->createNode( std::move(dataModel), cursor);
        abs_node->pos = cursor;
        abs_node->size = scene->getNodeSize( new_node );

        // free if it was already present
        if( abs_node->corresponding_node )
        {
            scene->removeNode( *abs_node->corresponding_node );
        }
        abs_node->corresponding_node = &new_node;

        scene->createConnection( *abs_node->corresponding_node, 0,
                                 *parent_node, 0 );

        for ( uint16_t child_uid: abs_node->children_uid )
        {
            AbstractTreeNode* child = &(abstract_tree.nodes.at(child_uid));
            recursiveStep( child, abs_node->corresponding_node, nest_level+1 );
            x_offset += 30;
        }

        return;
    };

   // start recursion
    QtNodes::Node& first_qt_node = scene->createNode( scene->registry().create("Root"), QPointF() );

    uint16_t root_uid = abstract_tree.root_node_uid;
    recursiveStep( &(abstract_tree.nodes[ root_uid ]), &first_qt_node, 1 );

}

AbsBehaviorTree BuildBehaviorTreeFromFlatbuffers(const char* buffer)
{
    auto fb_behavior_tree = BT_Serialization::GetBehaviorTree( buffer );

    AbsBehaviorTree tree;
    tree.root_node_uid = fb_behavior_tree->root_uid();

    tree.nodes.clear();

    for( const BT_Serialization::TreeNode* node: *(fb_behavior_tree->nodes()) )
    {
        AbstractTreeNode abs_node;
        abs_node.instance_name = node->instance_name()->c_str();
        abs_node.registration_name = node->registration_name()->c_str();
        abs_node.uid = node->uid();
        abs_node.type   = convert( node->type() );
        abs_node.status = convert( node->status() );

        tree.nodes.insert( {abs_node.uid, abs_node} );
    }

    for( const BT_Serialization::TreeNode* node: *(fb_behavior_tree->nodes()) )
    {
        AbstractTreeNode& abs_node = tree.nodes.at( node->uid() );
        abs_node.children_uid.clear();

        for (size_t i=0; i< node->children_uid()->size(); i++ )
        {
            uint16_t child_uid = node->children_uid()->Get(i);
            abs_node.children_uid.push_back( child_uid );
        }
    }
    return tree;
}

QtNodes::NodeStyle getStyleFromStatus(NodeStatus status)
{
    QtNodes::NodeStyle style;

    if( status == NodeStatus::IDLE )
    {
        return style;
    }

    if( status == NodeStatus::SUCCESS )
    {
        style.NormalBoundaryColor = QColor(102, 255, 51);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(51, 153, 51);
    }
    else if( status == NodeStatus::RUNNING )
    {
        style.NormalBoundaryColor = QColor(255, 204, 0);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(204, 122, 0);
    }
    else if( status == NodeStatus::FAILURE )
    {
        style.NormalBoundaryColor = QColor(255, 51, 0);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(204, 51, 0);
    }

    return style;
}
