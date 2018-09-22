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
                return scene.getNodePosition( *a ).x() <  scene.getNodePosition( *b ).x();
            } );
        }
        else{
            std::sort(children.begin(), children.end(),
                      [&](const Node* a, const Node* b)
            {
                return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
            } );
        }
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
        if( node->children_index.size() > 0 )
        {
            qreal recommended_pos = NODE_SPACING * 0.5;

            current_layer++;

            if( layout == PortLayout::Vertical)
            {
                recommended_pos += (node->pos.x() + node->size.width()*0.5)  ;
                for(int16_t index: node->children_index)
                {
                    AbstractTreeNode* child = (tree.nodeAtIndex( index ));
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
                for(int index: node->children_index)
                {
                    AbstractTreeNode* child = (tree.nodeAtIndex( index ));
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

            for(int index: node->children_index)
            {
                AbstractTreeNode* child = (tree.nodeAtIndex( index ));

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
    if( tree.nodesCount() == 0)
    {
        return;
    }

    AbstractTreeNode* root_node =  tree.rootNode();
    RecursiveNodeReorder(tree, root_node, scene.layout() );

    for (size_t index = 0; index < tree.nodesCount(); index++)
    {
        auto abs_node = tree.nodeAtIndex(index);
        Node& node = *( abs_node->corresponding_node);
        scene.setNodePosition( node, abs_node->pos );
    }
}


AbsBehaviorTree BuildTreeFromScene(const QtNodes::FlowScene *scene)
{
    auto root_node = findRoot( *scene );
    if( !root_node )
    {
        qDebug() << "Error: can not create a tree from a scene unless it has a single root node ";
        return AbsBehaviorTree();
    }

    AbsBehaviorTree tree;

    std::map<QtNodes::Node*,int> node_to_index;

    std::function<void(QtNodes::Node*)> pushRecursively;
    pushRecursively = [&](QtNodes::Node* node)
    {
        AbstractTreeNode abs_node;

        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>(node->nodeDataModel());

        abs_node.instance_name     = bt_model-> instanceName();
        abs_node.registration_name = bt_model-> registrationName();
        abs_node.pos  = scene->getNodePosition(*node) ;
        abs_node.size = scene->getNodeSize(*node);
        abs_node.corresponding_node = node;
        abs_node.parameters = bt_model->getCurrentParameters();
        abs_node.index = tree.nodesCount();

        node_to_index.insert( { node, abs_node.index } );
        tree.pushBack( bt_model->UID(), abs_node );

        auto children = getChildren( *scene, *node, true );

        for(auto& child_node: children )
        {
            pushRecursively( child_node );
        }
    };

    pushRecursively( root_node );

    for( auto& abs_node: tree.nodes())
    {
        auto node = abs_node.corresponding_node;
        auto children = getChildren( *scene, *node, true );

        abs_node.children_index.reserve( children.size() );

        for(auto& child_node: children )
        {
            abs_node.children_index.push_back( node_to_index[child_node] );
        }
    }

    tree.updateRootIndex();

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

    std::function<void(const XMLElement*, int)> recursiveStep;

    recursiveStep = [&](const XMLElement* xml_node, int parent_index)
    {
        // The nodes with a ID used that QString to insert into the registry()
        QString modelID = xml_node->Name();
        if( xml_node->Attribute("ID") )
        {
            modelID = xml_node->Attribute("ID");
        }

        AbstractTreeNode tree_node;

        tree_node.registration_name = modelID;
        tree_node.type = getNodeTypeFromString( xml_node->Name() );

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
                tree_node.parameters.push_back( { attr_name, attribute->Value() } );
            }
        }

        const int CURRENT_INDEX = tree.nodesCount();
        tree.pushBack( CURRENT_INDEX, tree_node);

        if( parent_index >= 0 )
        {
            auto parent_node = tree.nodeAtIndex(parent_index);
            parent_node->children_index.push_back( CURRENT_INDEX );
        }

        for (const XMLElement*  child = xml_node->FirstChildElement( )  ;
             child != nullptr;
             child = child->NextSiblingElement( ) )
        {
            recursiveStep( child, CURRENT_INDEX );
        }
    };

    // start recursion
    recursiveStep( bt_root->FirstChildElement(), -1 );

    tree.updateRootIndex();

    return tree;
}


AbsBehaviorTree BuildTreeFromFlatbuffers(const BT_Serialization::BehaviorTree *fb_behavior_tree)
{
    AbsBehaviorTree tree;

    AbstractTreeNode root_node;
    root_node.instance_name = "Root";
    root_node.registration_name = "Root";
    root_node.type = NodeType::ROOT;
    root_node.children_index.push_back( 1 );

    tree.pushBack( std::numeric_limits<uint16_t>::max(), root_node );

    for( const BT_Serialization::TreeNode* node: *(fb_behavior_tree->nodes()) )
    {
        AbstractTreeNode abs_node;
        abs_node.instance_name = node->instance_name()->c_str();
        abs_node.registration_name = node->registration_name()->c_str();
        abs_node.type   = convert( node->type() );
        abs_node.status = convert( node->status() );

        for( const BT_Serialization::KeyValue* pair: *(node->params()) )
        {
            abs_node.parameters.push_back( { QString(pair->key()->c_str()),
                                             QString(pair->value()->c_str()) } );
        }

        tree.pushBack(node->uid(), abs_node);
    }

    for(size_t index = 0; index < fb_behavior_tree->nodes()->size(); index++ )
    {
        const BT_Serialization::TreeNode* fb_node = fb_behavior_tree->nodes()->Get(index);
        // index+1 because we added the Root
        AbstractTreeNode* abs_node = tree.nodeAtIndex( index+1 );

        for( const auto child_uid: *(fb_node->children_uid()) )
        {
            int child_index = tree.UidToIndex( child_uid );
            abs_node->children_index.push_back( child_index );
        }
    }

    tree.updateRootIndex();
//    tree.debugPrint();
//    std::cout << " .....  " << std::endl;
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
        line->setText( param.default_value );
        return line;
    };

    return creator;
}


const std::set<std::string> &BuiltInRegisteredModels()
{
    const static std::set<std::string> models_ID = {
        SequenceModel::Name(),
        SequenceStarModel::Name(),
        FallbackModel::Name(),
        NegationNodeModel::Name(),
        RetryNodeModel::Name(),
        RepeatNodeModel::Name()
    };
    return  models_ID;
}

bool addToModelRegistry(QtNodes::DataModelRegistry& registry,
                   const QString& ID,
                   const ParameterWidgetCreators& parameters,
                   NodeType node_type)
{
    if( BuiltInRegisteredModels().count(ID.toStdString()) )
    {
        return false;
    }
    if( node_type == NodeType::ACTION )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return detail::unique_qptr<ActionNodeModel>( new ActionNodeModel(ID, parameters) );
        };
        registry.registerModel("Action", node_creator, ID);
    }
    else if( node_type == NodeType::CONDITION )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return detail::unique_qptr<ConditionNodeModel>( new ConditionNodeModel(ID, parameters) );
        };
        registry.registerModel("Condition", node_creator, ID);
    }
    else if( node_type == NodeType::DECORATOR )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return detail::unique_qptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, parameters) );
        };
        registry.registerModel("Decorator", node_creator, ID);
    }
    else if( node_type == NodeType::SUBTREE )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return detail::unique_qptr<SubtreeNodeModel>( new SubtreeNodeModel(ID,parameters) );
        };
        registry.registerModel("SubTree", node_creator, ID);

        auto otherID = ID + EXPANDED_SUFFIX;
        node_creator = [ID, otherID, parameters]()
        {
          auto node = detail::unique_qptr<SubtreeExpandedNodeModel>(
                new SubtreeExpandedNodeModel(otherID, parameters) );

          node->setInstanceName(ID);
          return node;
        };
        registry.registerModel("SubTreeExpanded", node_creator, otherID);
    }
    return true;
}

