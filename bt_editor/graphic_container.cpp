#include "graphic_container.h"
#include "utils.h"
#include "models/BehaviorTreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"
#include "models/ControlNodeModel.hpp"

#include <QSignalBlocker>
#include <QMenu>

using namespace QtNodes;

GraphicContainer::GraphicContainer(std::shared_ptr<DataModelRegistry> model_registry,
                                   QObject *parent) :
  QObject(parent),
  _model_registry(model_registry),
  _signal_was_blocked(true)
{
  _scene = new EditorFlowScene( model_registry );
  _view  = new FlowView( _scene );

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

  connect( _view, &QtNodes::FlowView::startMultipleDelete, [this]()
  {
    _signal_was_blocked = this->blockSignals(true);
  } );

  connect( _view, &QtNodes::FlowView::finishMultipleDelete, [this]()
  {
    this->blockSignals(_signal_was_blocked);
    this->undoableChange();
  }  );


  connect( _scene, &QtNodes::FlowScene::connectionCreated,
           [this](QtNodes::Connection &c )
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

    auto bt_model = dynamic_cast<BehaviorTreeNodeModel*>( node->nodeDataModel() );
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
    auto abstract_tree = BuildAbstractTree( *_scene );
    NodeReorder( *_scene, std::move(abstract_tree) );
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
  if( auto bt_node = dynamic_cast<BehaviorTreeNodeModel*>( node.nodeDataModel() ) )
  {
    connect( bt_node, &BehaviorTreeNodeModel::parameterUpdated,
             this, &GraphicContainer::undoableChange );

    connect( bt_node, &BehaviorTreeNodeModel::instanceNameChanged,
             this, &GraphicContainer::undoableChange );
  }

  undoableChange();
}

void GraphicContainer::onNodeContextMenu(Node &node, const QPointF &pos)
{
  QMenu* nodeMenu = new QMenu(_view);

  //--------------------------------
  createMorphSubMenu(node, nodeMenu);
  //--------------------------------
  auto *remove = new QAction("Remove ", nodeMenu);
  nodeMenu->addAction(remove);

  connect( remove, &QAction::triggered, [this,&node]()
  {
    _scene->removeNode(node);
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

      connect( action, &QAction::triggered, [this, &node, name]
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
      connect( smart_remove, &QAction::triggered, [this, node_ptr, parent_node, conn_out]()
      {
        {
          const QSignalBlocker blocker(this);
          for( auto& it: conn_out)
          {
            auto child_node = it.second->getNode(PortType::In);
            _scene->createConnection( *child_node, 0, *parent_node, 0 );
          }
          _scene->removeNode( *node_ptr );
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
        connect( action, &QAction::triggered, [this, &connection, name]
        {
          this->insertNodeInConnection( connection, name);
        });
      }
    }
  }

  nodeMenu->exec( QCursor::pos() );
}

