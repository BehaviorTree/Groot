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
    _model_registry( model_registry ),
    _signal_was_blocked(true)
{
    _scene = new EditorFlowScene( model_registry, parent );
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
        conn->getConnectionGraphicsObject().lock( locked );
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
    undoableChange();
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

void GraphicContainer::onNodeDoubleClicked(Node &root_node)
{
    std::function<void(QtNodes::Node&)> selectRecursively;

    selectRecursively = [&](QtNodes::Node& node)
    {
        node.nodeGraphicsObject().setSelected(true);
        auto children = getChildren(*_scene,node);
        for (auto& child: children)
        {
            selectRecursively(*child);
        }
    };

    selectRecursively(root_node);
}

void GraphicContainer::onNodeCreated(Node &node)
{
    if( auto bt_node = dynamic_cast<BehaviorTreeDataModel*>( node.nodeDataModel() ) )
    {
        connect( bt_node, &BehaviorTreeDataModel::parameterUpdated,
                 this, &GraphicContainer::undoableChange );

        connect( bt_node, &BehaviorTreeDataModel::instanceNameChanged,
                 this, &GraphicContainer::undoableChange );
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
                {
                    const QSignalBlocker blocker(this);
                    node.changeDataModel( _model_registry->create(name) );
                    nodeReorder();
                }
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

void GraphicContainer::loadSceneFromTree(const AbsBehaviorTree &tree)
{
    _abstract_tree = tree;

    _scene->clearScene();

    QPointF cursor(0,0);
    double x_offset = 0;

    std::function<void(AbstractTreeNode*, Node*, int)> recursiveStep;

    recursiveStep = [&](AbstractTreeNode* abs_node, Node* parent_node, int nest_level)
    {
        std::unique_ptr<NodeDataModel> dataModel = _scene->registry().create( abs_node->registration_name );

        if (!dataModel)
        {
            QString ID = abs_node->registration_name;

            if(  abs_node->type == NodeType::ACTION || abs_node->type == NodeType::CONDITION)
            {
                DataModelRegistry::RegistryItemCreator node_creator = [ID]()
                {
                    return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID, ParameterWidgetCreators() ) );
                };
                _scene->registry().registerModel("Action", node_creator);
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
            dataModel = _scene->registry().create( abs_node->registration_name );
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
        bt_node->setUID( abs_node->index );

        for (auto& it: abs_node->parameters)
        {
            bt_node->setParameterValue( it.first, it.second );
        }

        cursor.setY( cursor.y() + 65);
        cursor.setX( nest_level * 400 + x_offset );

        Node& new_node = _scene->createNode( std::move(dataModel), cursor);
        abs_node->pos = cursor;
        abs_node->size = _scene->getNodeSize( new_node );

        // free if it was already present
        if( abs_node->corresponding_node )
        {
            _scene->removeNode( *abs_node->corresponding_node );
        }
        abs_node->corresponding_node = &new_node;

        _scene->createConnection( *abs_node->corresponding_node, 0,
                                 *parent_node, 0 );

        for ( int16_t index: abs_node->children_index )
        {
            AbstractTreeNode* child = &(_abstract_tree.nodeAtIndex(index));
            recursiveStep( child, abs_node->corresponding_node, nest_level+1 );
            x_offset += 30;
        }
    };

   // start recursion
    Node& first_qt_node = _scene->createNode( _scene->registry().create("Root"), QPointF() );
    recursiveStep( _abstract_tree.rootNode(), &first_qt_node, 1 );

}

void GraphicContainer::loadFromJson(const QByteArray &data)
{
    const QSignalBlocker blocker( this );
    clearScene();
    scene()->loadFromMemory( data );
    _abstract_tree = BuildTreeFromScene( scene() );
}


