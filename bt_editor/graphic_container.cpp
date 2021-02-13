#include "graphic_container.h"
#include "utils.h"
#include "mainwindow.h"

#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

#include <QSignalBlocker>
#include <QMenu>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QInputDialog>

using namespace QtNodes;

GraphicContainer::GraphicContainer(std::shared_ptr<DataModelRegistry> model_registry,
                                   QWidget *parent) :
    QObject(parent),
    _model_registry( std::move(model_registry) ),
    _signal_was_blocked(true)
{
    _scene = new EditorFlowScene( _model_registry, parent );
    _view  = new QtNodes::FlowView( _scene, parent );

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
    std::vector<QtNodes::Node*> subtrees_expanded;
    for (auto& nodes_it: _scene->nodes() )
    {

        QtNodes::Node* node = nodes_it.second.get();
        auto bt_model = dynamic_cast<BehaviorTreeDataModel*>( node->nodeDataModel() );

        if(bt_model->registrationName() == "Root")
        {
            bt_model->lock( locked );
            node->nodeGraphicsObject().setFlag(QGraphicsItem::ItemIsMovable,  false);
            node->nodeGraphicsObject().setFlag(QGraphicsItem::ItemIsFocusable, true);
            node->nodeGraphicsObject().setFlag(QGraphicsItem::ItemIsSelectable, false);
            continue;
        }

        bt_model->lock(locked);
        node->nodeGraphicsObject().lock(locked);

        if( auto subtree = dynamic_cast<SubtreeNodeModel*>( node->nodeDataModel() ) )
        {
            if( subtree->expanded()) subtrees_expanded.push_back(node);
        }

        if( !locked )
        {
            node->nodeGraphicsObject().setGeometryChanged();
            QtNodes::NodeStyle style;
            node->nodeDataModel()->setNodeStyle( style );
            node->nodeGraphicsObject().update();
        }
    }

    for (auto& subtree: subtrees_expanded )
    {
        lockSubtreeEditing(*subtree, true, !locked);
    }

    for (auto& conn_it: _scene->connections() )
    {
        QtNodes::Connection* conn = conn_it.second.get();
        conn->connectionGraphicsObject().lock( locked );
    }
}

void GraphicContainer::lockSubtreeEditing(Node &root_node, bool locked, bool change_style)
{
    for (auto node: getSubtreeNodesRecursively( root_node ) )
    {
        node->nodeGraphicsObject().lock( locked );

        if( auto bt_model = dynamic_cast<BehaviorTreeDataModel*>( node->nodeDataModel() ) )
        {
            bt_model->lock(locked);
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
        //--------------------------------
        QtNodes::NodeStyle style;

        if( locked && change_style )
        {
            style.GradientColor0.setBlue(120);
            style.GradientColor1.setBlue(100);
            style.GradientColor2.setBlue(90);
            style.GradientColor3.setBlue(90);
        }
        node->nodeGraphicsObject().setGeometryChanged();
        node->nodeDataModel()->setNodeStyle( style );
        node->nodeGraphicsObject().update();
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
    rect.setBottom( rect.top() + rect.height()* 1.2 );

    const int min_height = 300;
    if( rect.height() < min_height )
    {
        rect.setBottom( rect.top() + min_height );
    }

    _view->setSceneRect (rect);
    _view->fitInView(rect, Qt::KeepAspectRatio);
    _view->scale(0.9, 0.9);
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

void GraphicContainer::createSubtree(Node &root_node, QString subtree_name )
{
    bool ok = false;
    if( subtree_name.isEmpty() )
    {
        subtree_name = QInputDialog::getText (
                    nullptr, tr ("SubTree Name"),
                    tr ("Insert the name of the Custom SubTree"),
                    QLineEdit::Normal, "", &ok);
        if (!ok)
        {
            return;
        }
    }

    auto main_win = dynamic_cast<MainWindow*>( parent() );

    if( main_win->getTabByName(subtree_name) != nullptr ||
            _model_registry->registeredModelsByCategory("SubTree").count( subtree_name ))
    {
        QMessageBox::warning( nullptr, "SubTree already exists",
                              tr("There is already a SubTree called [%1].\n"
                                 "Use another name.").arg(subtree_name),
                              QMessageBox::Ok);
        return;
    }

    addNewModel( { NodeType::SUBTREE, subtree_name, {}} );
    QApplication::processEvents();

    auto sub_tree = BuildTreeFromScene(_scene, &root_node);

    for(auto& abs_node: sub_tree.nodes())
    {
        if( &abs_node == sub_tree.rootNode() )
        {
            continue;
        }
        _scene->removeNode( *abs_node.graphic_node );
        abs_node.graphic_node = nullptr;
    }
    substituteNode( sub_tree.rootNode()->graphic_node, subtree_name);

    nodeReorder();

    emit requestSubTreeCreate( sub_tree, subtree_name );
}

void GraphicContainer::onNodeDoubleClicked(Node &root_node)
{
    auto nodes = getSubtreeNodesRecursively(root_node);
    for(auto node: nodes)
    {
        node->nodeGraphicsObject().setSelected(true);
    }
}

void GraphicContainer::onPortValueDoubleClicked(QLineEdit *edit_value)
{
    for (const auto& it: _scene->nodes())
    {
        auto node_model = dynamic_cast<BehaviorTreeDataModel*>( it.second->nodeDataModel() );
        QString val = edit_value ?  edit_value->text() : QString();
        node_model->onHighlightPortValue( val );
    }
}

void GraphicContainer::onNodeCreated(Node &node)
{
    if( auto bt_node = dynamic_cast<BehaviorTreeDataModel*>( node.nodeDataModel() ) )
    {
        connect( bt_node, &BehaviorTreeDataModel::parameterUpdated,
                 this, &GraphicContainer::undoableChange );

        connect( bt_node, &BehaviorTreeDataModel::portValueDoubleChicked,
                 this, &GraphicContainer::onPortValueDoubleClicked );

        connect( bt_node, &BehaviorTreeDataModel::instanceNameChanged,
                this, &GraphicContainer::undoableChange );

        if( auto subtree_node = dynamic_cast<SubtreeNodeModel*>( bt_node ) )
        {
            auto main_win = dynamic_cast<MainWindow*>( parent() );
            if( main_win && main_win->getTabByName( bt_node->registrationName() ) == nullptr  )
            {
                subtree_node->expandButton()->setEnabled(true);
            }
            connect( subtree_node, &SubtreeNodeModel::expandButtonPushed,
                     &(node), [&node, this]()
            {
                emit requestSubTreeExpand( *this, node );
            });
        }

        bt_node->initWidget();
    }
    undoableChange();
}

void GraphicContainer::onNodeContextMenu(Node &node, const QPointF &)
{
    // only allow context menu in editor mode
    auto main_win = dynamic_cast<MainWindow*>( parent() );
    if( main_win->getGraphicMode() != GraphicMode::EDITOR )
    {
        return;
    }

    QMenu* node_menu = new QMenu(_view);
    //--------------------------------
    createMorphSubMenu(node, node_menu);
    //--------------------------------
    auto remove = node_menu->addAction("Remove");

    connect( remove, &QAction::triggered,
             this, [this, &node]()
    {
        {
            const QSignalBlocker blocker(this);
            _scene->removeNode(node);
        }
        emit undoableChange();
    });
    //--------------------------------
    createSmartRemoveAction(node, node_menu);
    //--------------------------------
    if(auto bt_node = dynamic_cast<BehaviorTreeDataModel*>(node.nodeDataModel()))
    {
        auto subtree = node_menu->addAction("Create SubTree here");
        auto type = bt_node->nodeType();
        subtree->setEnabled( type == NodeType::DECORATOR || type == NodeType::CONTROL);

        connect( subtree, &QAction::triggered, this, [this, &node]()
        {
            createSubtree(node);
        });
    }
    //--------------------------------
    node_menu->exec( QCursor::pos() );
}

QtNodes::Node* GraphicContainer::substituteNode(Node *old_node, const QString& new_node_ID)
{
    const QSignalBlocker blocker(this);
    QPointF prev_pos   = _scene->getNodePosition( *old_node );
    double prev_width = old_node->nodeGeometry().width();

    auto& new_node = scene()->createNodeAtPos( new_node_ID, new_node_ID, prev_pos);

    auto bt_old_node = dynamic_cast<BehaviorTreeDataModel*>( old_node->nodeDataModel());
    auto bt_new_node = dynamic_cast<BehaviorTreeDataModel*>( new_node.nodeDataModel());

    if( bt_old_node && bt_new_node)
    {
        // if the old one contains an edited instance name, use it in new_node
        if( bt_old_node->instanceName() != bt_old_node->registrationName() &&
            bt_new_node->model().type != NodeType::SUBTREE )
        {
            bt_new_node->setInstanceName( bt_old_node->instanceName() );
        }
        // if the old one contains an editedport remapping, use it in new_node
        for(const auto& old_it:  bt_old_node->getCurrentPortMapping() )
        {
            auto new_mapping = bt_new_node->getCurrentPortMapping();
            if( old_it.second.isEmpty() == false && new_mapping.count( old_it.first ) )
            {
                bt_new_node->setPortMapping( old_it.first, old_it.second );
            }
        }
    }

    QPointF new_pos = prev_pos;
    double new_width = new_node.nodeGeometry().width();

    new_pos.setX( prev_pos.x() - (new_width - prev_width)*0.5 );
    _scene->setNodePosition(new_node, new_pos);

    if( old_node->nodeDataModel()->nPorts( PortType::In ) == 1 &&
        new_node.nodeDataModel()->nPorts( PortType::In ) == 1 )
    {
        auto conn_in  = old_node->nodeState().connections(PortType::In, 0);
        for(auto it: conn_in)
        {
            auto child_node = it.second->getNode(PortType::Out);
            _scene->createConnection( new_node, 0, *child_node, 0 );
        }
    }

    if( old_node->nodeDataModel()->nPorts( PortType::Out ) == 1 &&
        new_node.nodeDataModel()->nPorts( PortType::Out ) == 1 )
    {
        auto conn_in  = old_node->nodeState().connections(PortType::Out, 0);
        for(auto it: conn_in)
        {
            auto child_node = it.second->getNode(PortType::In);
            _scene->createConnection( *child_node, 0, new_node, 0 );
        }
    }
    _scene->removeNode(*old_node);

    return &new_node;
}

void GraphicContainer::deleteSubTreeRecursively(Node &root_node)
{
    const QSignalBlocker blocker1( this );
    const QSignalBlocker blocker2( _scene );
    auto nodes_to_delete = getSubtreeNodesRecursively(root_node);
    for(auto delete_me: nodes_to_delete)
    {
        _scene->removeNode( *delete_me );
    }
}


void GraphicContainer::createMorphSubMenu(QtNodes::Node &node, QMenu* nodeMenu)
{
    auto bt_model =  dynamic_cast<BehaviorTreeDataModel*>( node.nodeDataModel() );
    const QString category ( QString::fromStdString( toStr( bt_model->nodeType()) ));

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

void GraphicContainer::createSmartRemoveAction(QtNodes::Node &node, QMenu* node_menu)
{
    auto *smart_remove = new QAction("Smart Remove ", node_menu);
    node_menu->addAction(smart_remove);

    NodeState::ConnectionPtrSet conn_in  = node.nodeState().connections(PortType::In, 0);
    NodeState::ConnectionPtrSet conn_out = node.nodeState().connections(PortType::Out, 0);

    if( conn_in.size() != 1 || conn_out.size() == 0 )
    {
        smart_remove->setEnabled(false);
        return;
    }

    auto parent_node = conn_in.begin()->second->getNode(PortType::Out);
    auto policy = parent_node->nodeDataModel()->portOutConnectionPolicy(0);

    if( policy == NodeDataModel::ConnectionPolicy::One && conn_out.size() > 1)
    {
        smart_remove->setEnabled(false);
        return;
    }

    QtNodes::Node* node_ptr = &node;
    connect( smart_remove, &QAction::triggered,
             this, [this, node_ptr]()
    {
        onSmartRemove(node_ptr);
    });
}

void GraphicContainer::onSmartRemove(QtNodes::Node* node)
{
    auto parent_node = GetParentNode( node );
    NodeState::ConnectionPtrSet conn_out = node->nodeState().connections(PortType::Out, 0);

    if( !parent_node || conn_out.size() == 0 )
    {
        return;
    }

    {
        const QSignalBlocker blocker(this);
        for( auto& it: conn_out)
        {
            auto child_node = it.second->getNode(PortType::In);
            _scene->createConnection( *child_node, 0, *parent_node, 0 );
        }
        _scene->removeNode( *node );
        nodeReorder();
    }
    undoableChange();
}

void GraphicContainer::insertNodeInConnection(Connection &connection, QString node_name)
{
    {
        const QSignalBlocker blocker(this);

        auto parent_node = connection.getNode(PortType::Out);
        auto child_node  = connection.getNode(PortType::In);

        QPointF pos = child_node->nodeGraphicsObject().pos();
        pos.setX( pos.x() - 50 );

        QtNodes::Node& inserted_node = _scene->createNodeAtPos( node_name, node_name, pos );

        _scene->deleteConnection(connection);
        _scene->createConnection(*child_node, 0, inserted_node, 0);
        _scene->createConnection(inserted_node, 0, *parent_node, 0);
        nodeReorder();
    }
    undoableChange();
}

void GraphicContainer::onConnectionContextMenu(QtNodes::Connection &connection, const QPointF&)
{
    // only allow connection context menu in editor mode
    auto main_win = dynamic_cast<MainWindow*>( parent() );
    if( main_win->getGraphicMode() != GraphicMode::EDITOR )
    {
        return;
    }

    QMenu* conn_menu = new QMenu(_view);

    auto categories = {"Control", "Decorator"};

    for(auto category: categories)
    {
        QMenu* submenu = conn_menu->addMenu(QString("Insert ") + category + QString("Node") );
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

    conn_menu->exec( QCursor::pos() );
}

void GraphicContainer::recursiveLoadStep(QPointF& cursor,
                                         AbsBehaviorTree& tree,
                                         AbstractTreeNode* abs_node,
                                         Node* parent_node, int nest_level)
{
    Node& new_node = _scene->createNodeAtPos( abs_node->model.registration_ID,
                                              abs_node->instance_name,
                                              cursor);
    BehaviorTreeDataModel* bt_node = dynamic_cast<BehaviorTreeDataModel*>( new_node.nodeDataModel() );

    for (auto& port_it: abs_node->ports_mapping)
    {
        bt_node->setPortMapping( port_it.first, port_it.second );
    }
    bt_node->initWidget();

    new_node.nodeGeometry().recalculateSize();

    abs_node->pos = cursor;
    abs_node->size = _scene->getNodeSize( new_node );
    abs_node->graphic_node = &new_node;

    // Special case for node Subtree. Expand if necessary
    if( abs_node->model.type == NodeType::SUBTREE &&
            abs_node->children_index.size() == 1 )
    {
        if( auto subtree_node = dynamic_cast<SubtreeNodeModel*>( bt_node ) )
        {
            subtree_node->setExpanded(true);
            new_node.nodeState().getEntries(PortType::Out).resize(1);
            subtree_node->expandButton()->setHidden( true );
            emit subtree_node->updateNodeSize();
        }
    }

    _scene->createConnection( *abs_node->graphic_node, 0,
                              *parent_node, 0 );

    for ( int index: abs_node->children_index)
    {
        cursor.setX( cursor.x() + abs_node->size.width() );
        cursor.setY( cursor.y() + abs_node->size.height() );
        AbstractTreeNode* child = tree.node(index);
        recursiveLoadStep(cursor, tree, child, abs_node->graphic_node, nest_level+1 );
    }
}


void GraphicContainer::loadSceneFromTree(const AbsBehaviorTree &tree)
{
    AbsBehaviorTree abs_tree = tree;
    _scene->clearScene();

    auto& first_qt_node = _scene->createNodeAtPos( "Root", "Root", QPointF(0,0) );

    QPointF cursor( - first_qt_node.nodeGeometry().width()*0.5,
                    - first_qt_node.nodeGeometry().height()*0.5);

    _scene->setNodePosition( first_qt_node, cursor );

    auto root_node = abs_tree.rootNode();

    if( root_node->model.registration_ID == "Root" )
    {
        root_node->graphic_node = &first_qt_node;
        int root_child_index = root_node->children_index.front();
        root_node = abs_tree.node(root_child_index);
    }

    recursiveLoadStep(cursor, abs_tree, root_node, &first_qt_node, 1 );
    NodeReorder( *_scene, abs_tree );
}

void GraphicContainer::appendTreeToNode(Node &node, AbsBehaviorTree& subtree)
{
    const QSignalBlocker blocker( this );

    for (auto& abs_node: subtree.nodes() )
    {
        abs_node.graphic_node = nullptr;
    }

    //--------------------------------------
    QPointF cursor = _scene->getNodePosition(node) + QPointF(100,100);

    auto root_node = subtree.rootNode();

    if( root_node->model.registration_ID == "Root" )
    {
        if( root_node->children_index.size() == 1)
        {
            // first node become the child of Root
            int root_child_index = root_node->children_index.front();
            root_node = subtree.node(root_child_index);
        }
        else{
            // Root has no child. Stop
            //  qDebug() << "Error: can't expand empty subtree";
            return;
        }
    }

    recursiveLoadStep(cursor, subtree, root_node , &node, 1 );
}

void GraphicContainer::loadFromJson(const QByteArray &data)
{
    const QSignalBlocker blocker( this );
    clearScene();
    scene()->loadFromMemory( data );
}


