#include "graphic_container.h"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

#include <QSignalBlocker>
#include <QMenu>
#include <QDebug>

using namespace QtNodes;

GraphicContainer::GraphicContainer(std::shared_ptr<DataModelRegistry> model_registry,
                                   QWidget *parent) :
    QObject(parent),
    _model_registry( std::move(model_registry) ),
    _signal_was_blocked(true)
{
    _scene = new EditorFlowScene( _model_registry, parent );
    _view  = new FlowView( _scene, parent );

    connect( _scene, &QtNodes::FlowScene::nodeDoubleClicked,
             this, &GraphicContainer::onNodeDoubleClicked);

    connect( _scene, &QtNodes::FlowScene::nodeCreated,
             this,   &GraphicContainer::onNodeCreated  );

    connect( _scene, &QtNodes::FlowScene::nodeContextMenu,
             this, &GraphicContainer::onNodeContextMenu );

    connect( _scene, &QtNodes::FlowScene::connectionContextMenu,
             this, &GraphicContainer::onConnectionContextMenu );

    connect( _scene, &QtNodes::FlowScene::nodeDeleted,
             this,   &GraphicContainer::undoableChange  );

    connect( _scene, &QtNodes::FlowScene::nodeMoved,
             this,   &GraphicContainer::undoableChange  );

    connect( _scene, &QtNodes::FlowScene::connectionDeleted,
             this,   &GraphicContainer::undoableChange  );

    connect( _view, &QtNodes::FlowView::startNodeDelete,
             this, [this]()
    {
        _signal_was_blocked = this->blockSignals(true);
    } );

    connect( _view, &QtNodes::FlowView::finishNodeDelete,
             this, [this]()
    {
        this->blockSignals(_signal_was_blocked);
        this->undoableChange();
    }  );


    connect( _scene, &QtNodes::FlowScene::connectionCreated,
             this, [this](QtNodes::Connection &c )
    {
        if( c.getNode(QtNodes::PortType::In) && c.getNode(QtNodes::PortType::Out))
        {
            undoableChange();
        }
    });

}

void GraphicContainer::lockEditing(bool locked)
{
    for (auto& nodes_it: _scene->nodes() )
    {
        QtNodes::Node* node = nodes_it.second.get();
        node->nodeGraphicsObject().lock( locked );

        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>( node->nodeDataModel() );
        if( bt_model )
        {
            bt_model->lock(locked);
        }

        if( !locked )
        {
            node->nodeGraphicsObject().setGeometryChanged();
            QtNodes::NodeStyle style;
            node->nodeDataModel()->setNodeStyle( style );
            node->nodeGraphicsObject().update();
        }
    }

    for (auto& conn_it: _scene->connections() )
    {
        QtNodes::Connection* conn = conn_it.second.get();
        conn->connectionGraphicsObject().lock( locked );
    }
}

void GraphicContainer::lockSubtreeEditing(Node &node, bool locked)
{
    auto nodes = getSubtreeNodesRecursively( node );

    for (auto node: nodes )
    {
        node->nodeGraphicsObject().lock( locked );

        if( locked)
        {
          QtNodes::NodeStyle style;

          style.GradientColor0.setBlue(120);
          style.GradientColor1.setBlue(100);
          style.GradientColor2.setBlue(80);
          style.GradientColor3.setBlue(70);
          node->nodeDataModel()->setNodeStyle(style);
        }

        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>( node->nodeDataModel() );
        if( bt_model )
        {
            bt_model->lock(locked);
        }

        if( !locked ) //TODO : change style
        {
            node->nodeGraphicsObject().setGeometryChanged();
            QtNodes::NodeStyle style;
            node->nodeDataModel()->setNodeStyle( style );
            node->nodeGraphicsObject().update();
        }

        auto connections = node->nodeState().getEntries(PortType::Out);
        for (auto& conn_by_port: connections )
        {
            for (auto& conn_it: conn_by_port )
            {
                QtNodes::Connection* conn = conn_it.second;
                conn->connectionGraphicsObject().lock( locked );
            }
        }
    }
}

void GraphicContainer::nodeReorder()
{
    {
        const QSignalBlocker blocker(this);
        auto abstract_tree = BuildTreeFromScene( _scene );
        NodeReorder( *_scene, abstract_tree );
        zoomHomeView();
    }
    emit undoableChange();
}

void GraphicContainer::zoomHomeView()
{
    QRectF rect = _scene->itemsBoundingRect();
    _view->setSceneRect (rect);
    _view->fitInView(rect, Qt::KeepAspectRatio);
    _view->scaleDown();
}

bool GraphicContainer::containsValidTree() const
{
    if( _scene->nodes().empty())
    {
        return false;
    }

    auto connections =  _scene->connections();

    std::set<const QtNodes::Node*> nodes_with_input;
    std::set<const QtNodes::Node*> nodes_with_output;

    for (const auto& it: _scene->connections())
    {
        const QtNodes::Connection* connection = it.second.get();
        auto node = connection->getNode( QtNodes::PortType::In);
        if( node ){
            nodes_with_input.insert( node );
        }
        node = connection->getNode( QtNodes::PortType::Out);
        if( node ){
            nodes_with_output.insert( node );
        }
    }

    for (const auto& it: _scene->nodes())
    {
        const QtNodes::Node* node = it.second.get();
        if( node->nodeDataModel()->nPorts(QtNodes::PortType::In) == 1 )
        {
            if( nodes_with_input.find(node) == nodes_with_input.end() )
            {
                return false;
            }
        }
        if( node->nodeDataModel()->nPorts(QtNodes::PortType::Out) == 1 )
        {
            if( nodes_with_output.find(node) == nodes_with_output.end() )
            {
                return false;
            }
        }
    }
    return true;
}

void GraphicContainer::clearScene()
{
    const QSignalBlocker blocker( this );
    _scene->clearScene();
}


AbsBehaviorTree GraphicContainer::loadedTree() const
{
    return BuildTreeFromScene( _scene );
}

std::set<QtNodes::Node*> GraphicContainer::getSubtreeNodesRecursively(Node &root_node)
{
    std::set<QtNodes::Node*> nodes;
    std::function<void(QtNodes::Node&)> selectRecursively;

    selectRecursively = [&](QtNodes::Node& node)
    {
        nodes.insert( &node );
        auto children = getChildren(*_scene, node, false);
        for (auto& child: children)
        {
            selectRecursively(*child);
        }
    };

    selectRecursively(root_node);
    return nodes;
}

void GraphicContainer::onNodeDoubleClicked(Node &root_node)
{
    auto nodes = getSubtreeNodesRecursively(root_node);
    for(auto node: nodes)
    {
        node->nodeGraphicsObject().setSelected(true);
    }
}

void GraphicContainer::onNodeCreated(Node &node)
{
    if( auto bt_node = dynamic_cast<BehaviorTreeDataModel*>( node.nodeDataModel() ) )
    {
        connect( bt_node, &BehaviorTreeDataModel::parameterUpdated,
                 this, &GraphicContainer::undoableChange );

        connect( bt_node, &BehaviorTreeDataModel::instanceNameChanged,
                 this, &GraphicContainer::undoableChange );

        if( auto sub_node = dynamic_cast<SubtreeNodeModel*>( bt_node ) )
        {
          connect( sub_node, &SubtreeNodeModel::expandButtonPushed,
                   &(node), [&node, this]()
          {
            emit requestSubTreeExpand( *this, node );
          });
        }

        if( auto sub_node = dynamic_cast<SubtreeExpandedNodeModel*>( bt_node ) )
        {
          connect( sub_node, &SubtreeExpandedNodeModel::collapseButtonPushed,
                   &(node), [&node, this]()
          {
            emit requestSubTreeExpand( *this, node );
          });
        }
    }
    undoableChange();
}

void GraphicContainer::onNodeContextMenu(Node &node, const QPointF &)
{
    QMenu* nodeMenu = new QMenu(_view);

    //--------------------------------
    createMorphSubMenu(node, nodeMenu);
    //--------------------------------
    auto *remove = new QAction("Remove ", nodeMenu);
    nodeMenu->addAction(remove);

    auto scene = _scene;

    connect( remove, &QAction::triggered,
             this, [this, &node, scene]()
    {
        {
            const QSignalBlocker blocker(this);
            scene->removeNode(node);
        }
        undoableChange();
    });
    //--------------------------------
    createSmartRemoveAction(node, nodeMenu);
    //--------------------------------
    nodeMenu->exec( QCursor::pos() );
}

QtNodes::Node* GraphicContainer::substituteNode(Node *node, const QString& new_node_name)
{
    const QSignalBlocker blocker(this);
    auto pos = _scene->getNodePosition( *node );
    auto new_datamodel = _model_registry->create(new_node_name);

    if( !new_datamodel )
    {
        return nullptr;
    }

    auto& new_node = _scene->createNode( std::move(new_datamodel), pos );

    if( node->nodeDataModel()->nPorts( PortType::In ) == 1 &&
        new_node.nodeDataModel()->nPorts( PortType::In ) == 1 )
    {
        auto conn_in  = node->nodeState().connections(PortType::In, 0);
        for(auto it: conn_in)
        {
            auto child_node = it.second->getNode(PortType::Out);
            _scene->createConnection( new_node, 0, *child_node, 0 );
        }
    }

    if( node->nodeDataModel()->nPorts( PortType::Out ) == 1 &&
        new_node.nodeDataModel()->nPorts( PortType::Out ) == 1 )
    {
        auto conn_in  = node->nodeState().connections(PortType::Out, 0);
        for(auto it: conn_in)
        {
            auto child_node = it.second->getNode(PortType::In);
            _scene->createConnection( *child_node, 0, new_node, 0 );
        }
    }
    _scene->removeNode(*node);

    return &new_node;
}

void GraphicContainer::deleteSubTreeRecursively(Node &root_node)
{
    const QSignalBlocker blocker( this );
    auto nodes_to_delete = getSubtreeNodesRecursively(root_node);
    for(auto delete_me: nodes_to_delete)
    {
        _scene->removeNode( *delete_me );
    }
}


void GraphicContainer::createMorphSubMenu(QtNodes::Node &node, QMenu* nodeMenu)
{
    const QString category = getCategory( node.nodeDataModel() );
    auto names_in_category = _model_registry->registeredModelsByCategory( category );
    names_in_category.erase( node.nodeDataModel()->name() );

    QMenu* morph_submenu = nodeMenu->addMenu("Morph into...");

    if( names_in_category.size() == 0)
    {
        morph_submenu->setEnabled(false);
    }
    else
    {
        for(auto& name: names_in_category)
        {
            auto action = new QAction(name, morph_submenu);
            morph_submenu->addAction(action);

            connect( action, &QAction::triggered, this, [this, &node, name]
            {
                substituteNode( &node, name);
                undoableChange();
            });
        }
    }
}

void GraphicContainer::createSmartRemoveAction(QtNodes::Node &node, QMenu* nodeMenu)
{
    auto *smart_remove = new QAction("Smart Remove ", nodeMenu);
    nodeMenu->addAction(smart_remove);

    NodeState::ConnectionPtrSet conn_in  = node.nodeState().connections(PortType::In,0);
    NodeState::ConnectionPtrSet conn_out;
    auto port_entries = node.nodeState().getEntries(PortType::Out);
    if( port_entries.size() == 1)
    {
        conn_out = port_entries.front();
    }

    if( conn_in.size() == 1 && conn_out.size() >= 1 )
    {
        auto parent_node = conn_in.begin()->second->getNode(PortType::Out);
        auto policy = parent_node->nodeDataModel()->portOutConnectionPolicy(0);

        if( policy == NodeDataModel::ConnectionPolicy::One && conn_out.size() > 1)
        {
            smart_remove->setEnabled(false);
        }
        else{
            auto node_ptr = &node;
            auto scene = _scene;
            connect( smart_remove, &QAction::triggered,
                     this, [this, node_ptr, parent_node, conn_out, scene]()
            {
                {
                    const QSignalBlocker blocker(this);
                    for( auto& it: conn_out)
                    {
                        auto child_node = it.second->getNode(PortType::In);
                        scene->createConnection( *child_node, 0, *parent_node, 0 );
                    }
                    scene->removeNode( *node_ptr );
                    nodeReorder();
                }
                undoableChange();
            });
        }
    }
    else{
        smart_remove->setEnabled(false);
    }
}

void GraphicContainer::insertNodeInConnection(Connection &connection, QString node_name)
{
    {
        const QSignalBlocker blocker(this);

        auto node_model = _model_registry->create(node_name);
        auto parent_node = connection.getNode(PortType::Out);
        auto child_node  = connection.getNode(PortType::In);

        QPointF pos = child_node->nodeGraphicsObject().pos();
        pos.setX( pos.x() - 50 );

        QtNodes::Node& inserted_node = _scene->createNode( std::move(node_model), pos );

        _scene->deleteConnection(connection);
        _scene->createConnection(*child_node, 0, inserted_node, 0);
        _scene->createConnection(inserted_node, 0, *parent_node, 0);
        nodeReorder();
    }
    undoableChange();
}

void GraphicContainer::onConnectionContextMenu(QtNodes::Connection &connection, const QPointF&)
{
    QMenu* nodeMenu = new QMenu(_view);
    auto categories = {"Control", "Decorator"};

    for(auto category: categories)
    {
        QMenu* submenu = nodeMenu->addMenu(QString("Insert ") + category + QString("Node") );
        auto model_names = _model_registry->registeredModelsByCategory( category );

        if( model_names.empty() )
        {
            submenu->setEnabled(false);
        }
        else{
            for(auto& name: model_names)
            {
                auto action = new QAction(name, submenu);
                submenu->addAction(action);
                connect( action, &QAction::triggered,
                         this,  [this, &connection, name]
                {
                    this->insertNodeInConnection( connection, name);
                });
            }
        }
    }

    nodeMenu->exec( QCursor::pos() );
}

void GraphicContainer::recursiveLoadStep(QPointF& cursor, double &x_offset,
                                         AbsBehaviorTree& tree,
                                         AbstractTreeNode* abs_node,
                                         Node* parent_node, int nest_level)
{
    std::unique_ptr<NodeDataModel> data_model = _scene->registry().create( abs_node->registration_name );

    if (!data_model)
    {
        QString ID = abs_node->registration_name;

        if(  abs_node->type == NodeType::ACTION )
        {
            DataModelRegistry::RegistryItemCreator node_creator = [ID]()
            {
                return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID, ParameterWidgetCreators() ) );
            };
            _scene->registry().registerModel("Action", node_creator);
        }
        else if( abs_node->type == NodeType::CONDITION)
        {
            DataModelRegistry::RegistryItemCreator node_creator = [ID]()
            {
                return std::unique_ptr<ConditionNodeModel>( new ConditionNodeModel(ID, ParameterWidgetCreators() ) );
            };
            _scene->registry().registerModel("Condition", node_creator);
        }
        else if( abs_node->type == NodeType::DECORATOR )
        {
            DataModelRegistry::RegistryItemCreator node_creator = [ID]()
            {
                return std::unique_ptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, ParameterWidgetCreators()) );
            };
            _scene->registry().registerModel("Decorator", node_creator);
        }
        else if( abs_node->type == NodeType::SUBTREE )
        {
            DataModelRegistry::RegistryItemCreator node_creator = [ID]()
            {
                return std::unique_ptr<SubtreeNodeModel>( new SubtreeNodeModel(ID, ParameterWidgetCreators()) );
            };
            _scene->registry().registerModel("SubTree", node_creator);
        }
        data_model = _scene->registry().create( abs_node->registration_name );
    }

    if(!data_model)
    {
        char buffer[250];
        sprintf(buffer, "No registered model with name: [%s](%s)",
                abs_node->registration_name.toStdString().c_str(),
                abs_node->instance_name.toStdString().c_str() );
        throw std::runtime_error( buffer );
    }

    BehaviorTreeDataModel* bt_node = dynamic_cast<BehaviorTreeDataModel*>( data_model.get() );

    bt_node->setInstanceName( abs_node->instance_name );

    for (auto& it: abs_node->parameters)
    {
        bt_node->setParameterValue( it.first, it.second );
    }

    cursor.setY( cursor.y() + 65);
    cursor.setX( nest_level * 400 + x_offset );

    Node& new_node = _scene->createNode( std::move(data_model), cursor);
    abs_node->pos = cursor;
    abs_node->size = _scene->getNodeSize( new_node );

    abs_node->corresponding_node = &new_node;

    _scene->createConnection( *abs_node->corresponding_node, 0,
                             *parent_node, 0 );

    for ( int16_t index: abs_node->children_index )
    {
        AbstractTreeNode* child = tree.nodeAtIndex(index);
        recursiveLoadStep(cursor, x_offset, tree, child, abs_node->corresponding_node, nest_level+1 );
        x_offset += 30;
    }
}


void GraphicContainer::loadSceneFromTree(const AbsBehaviorTree &tree)
{
    AbsBehaviorTree abstract_tree = tree;

    QPointF cursor(0,0);
    double x_offset = 0;

    _scene->clearScene();
    auto first_qt_node = &(_scene->createNode( _scene->registry().create("Root"),
                                          cursor ));

    auto root_node = abstract_tree.rootNode();
    if( root_node->type == NodeType::ROOT)
    {
      root_node->corresponding_node = first_qt_node;
      root_node = abstract_tree.nodeAtIndex( root_node->children_index.front() );
    }

    recursiveLoadStep(cursor, x_offset, abstract_tree, root_node, first_qt_node, 1 );
}

void GraphicContainer::appendTreeToNode(Node &node, AbsBehaviorTree subtree)
{
    const QSignalBlocker blocker( this );

    for (auto abs_node: subtree.nodes() )
    {
        abs_node.corresponding_node = nullptr;
    }

    //--------------------------------------
    QPointF cursor = _scene->getNodePosition(node) + QPointF(100,100);
    double x_offset = 0;

    auto root_node = subtree.rootNode();

    if( root_node->registration_name == "Root" &&
        root_node->corresponding_node->nodeDataModel()->nPorts(PortType::In) == 0 &&
        root_node->children_index.size() == 1 )
    {
        root_node = subtree.nodeAtIndex( root_node->children_index.front() );
    }

    recursiveLoadStep(cursor, x_offset, subtree, root_node , &node, 1 );
}

void GraphicContainer::loadFromJson(const QByteArray &data)
{
    const QSignalBlocker blocker( this );
    clearScene();
    scene()->loadFromMemory( data );
}


