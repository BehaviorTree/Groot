#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSettings>
#include <QTextStream>
#include <QList>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QWidgetAction>
#include <QTreeWidgetItem>
#include <QShortcut>
#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/NodeStyle>
#include <nodes/FlowView>

#include "editor_flowscene.h"

#include "XML_utilities.hpp"
#include "models/ActionNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/RootNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"


#include "utils.h"

using QtNodes::DataModelRegistry;
using QtNodes::FlowView;
using QtNodes::FlowScene;
using QtNodes::NodeGraphicsObject;
using QtNodes::NodeState;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  _arrange_shortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this),
  _root_node(nullptr),
  _undo_enabled(true)
{
  ui->setupUi(this);

  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
  restoreState(settings.value("MainWindow/windowState").toByteArray());

  _model_registry = std::make_shared<QtNodes::DataModelRegistry>();

  _model_registry->registerModel("Root", [](){ return std::make_unique<RootNodeModel>();} );

  _model_registry->registerModel("Control", [](){ return std::make_unique<SequenceModel>();} );
  _model_registry->registerModel("Control", [](){ return std::make_unique<SequenceStarModel>();} );
  _model_registry->registerModel("Control", [](){ return std::make_unique<FallbackModel>();} );

  _node_palette_widget = new NodePalette(_tree_nodes_model, this);
  ui->leftFrame->layout()->addWidget( _node_palette_widget );

  createTab("Behaviortree");

  this->setMenuBar(ui->menubar);
  ui->menubar->setNativeMenuBar(false);

  connect( &_arrange_shortcut, &QShortcut::activated,
           this,   &MainWindow::on_actionAuto_arrange_triggered  );

  connect( &_periodic_timer, SIGNAL(timeout()), this, SLOT(onTimerUpdate()) );

  _periodic_timer.start(10);

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 5);

  QShortcut* undo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this);
  connect( undo_shortcut, &QShortcut::activated, this, &MainWindow::onUndoInvoked );

  QShortcut* redo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this);
  connect( redo_shortcut, &QShortcut::activated, this, &MainWindow::onRedoInvoked );

}


void MainWindow::createTab(const QString &name)
{
  if( _tab_info.count(name) > 0)
  {
    throw std::runtime_error(std::string("There is already a Tab named ") + name.toStdString() );
  }
  TabInfo ti;
  ti.scene = new EditorFlowScene( _model_registry );
  ti.view  = new FlowView( ti.scene );
  _tab_info[name] = ti;

  ui->tabWidget->addTab( ti.view, name );

  connect( ti.scene, &QtNodes::FlowScene::changed,
           this,   &MainWindow::onSceneChanged  );

  connect( ti.scene, &QtNodes::FlowScene::nodeCreated,
           this,   &MainWindow::onNodeCreated  );

  connect( ti.view, &QtNodes::FlowView::startMultipleDelete,
           [this]() { this->_undo_enabled.store(false); }  );

  connect( ti.scene, &QtNodes::FlowScene::nodeDeleted,
           this,   &MainWindow::onPushUndo  );

  connect( ti.view, &QtNodes::FlowView::finishMultipleDelete,
           [this]() {
    this->_undo_enabled.store(true);
    this->onPushUndo();
  }  );

  connect( ti.scene, &QtNodes::FlowScene::nodeMoved,
           this,   &MainWindow::onPushUndo  );

  connect( ti.scene, &QtNodes::FlowScene::connectionCreated,
           this,   &MainWindow::onPushUndo  );

  connect( this, SIGNAL(updateGraphic()), ti.view, SLOT(repaint())  );

  connect( ti.scene, &QtNodes::FlowScene::nodeContextMenu,
           this, &MainWindow::onNodeContextMenu );

  connect( ti.scene, &QtNodes::FlowScene::connectionContextMenu,
           this, &MainWindow::onConnectionContextMenu );

  connect( ti.scene, &QtNodes::FlowScene::nodeDoubleClicked,
           this, &MainWindow::onNodeDoubleClicked);

  ti.view->update();
}

MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::loadFromXML(const QString& xml_text)
{
  try{
    using namespace tinyxml2;
    XMLDocument document;
    XMLError err = document.Parse( xml_text.toStdString().c_str(), xml_text.size() );
    if( !err )
    {
      ReadTreeNodesModel( document.RootElement(), *_model_registry, _tree_nodes_model );
      _node_palette_widget->updateTreeView();

      onPushUndo();
      _undo_enabled.store(false);

      currentTabInfo()->scene->clearScene();
      QtNodes::Node& first_qt_node = currentTabInfo()->scene->createNode( _model_registry->create("Root"), QPointF() );

      std::cout<< "Starting parsing"<< std::endl;

      ParseBehaviorTreeXML(document.RootElement()->FirstChildElement("BehaviorTree"),
                           currentTabInfo()->scene,
                           first_qt_node);

      std::cout<<"XML Parsed Successfully!"<< std::endl;

      NodeReorder( *currentTabInfo()->scene );
      _undo_enabled.store(true);
      onPushUndo();
    }
  }
  catch( std::runtime_error& err)
  {
    QMessageBox messageBox;
    messageBox.critical(this,"Error", err.what() );
    messageBox.show();
    return;
  }

  lockEditing( ui->selectMode->value() == 1 );
}


void MainWindow::on_actionLoad_triggered()
{
  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  QString directory_path  = settings.value("MainWindow.lastLoadDirectory",
                                           QDir::homePath() ).toString();

  QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                  tr("Open Flow Scene"), directory_path,
                                                  tr("XML StateMachine Files (*.xml)"));
  if (!QFileInfo::exists(fileName)){
    return;
  }

  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly)){
    return;
  }

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("MainWindow.lastLoadDirectory", directory_path);
  settings.sync();

  QString xml_text;

  QTextStream in(&file);
  while (!in.atEnd()) {
    xml_text += in.readLine();
  }

  loadFromXML(xml_text);
}




void MainWindow::on_actionSave_triggered()
{
  const QtNodes::FlowScene* scene = currentTabInfo()->scene;

  std::vector<QtNodes::Node*> roots = findRoots( *scene );
  bool valid_root = (roots.size() == 1) && ( dynamic_cast<RootNodeModel*>(roots.front()->nodeDataModel() ));

  QtNodes::Node* current_node = nullptr;

  if( valid_root )
  {
    auto root_children = getChildren(*scene, *roots.front() );
    if( root_children.size() == 1){
      current_node = root_children.front();
    }
    else{
      valid_root = false;
    }
  }

  if( !valid_root || !current_node)
  {
    QMessageBox::warning(this, tr("Oops!"),
                         tr("Malformed behavior tree. There must be only 1 root node"),
                         QMessageBox::Ok);
    return;
  }

  //----------------------------
  using namespace tinyxml2;
  XMLDocument doc;
  XMLNode* root = doc.InsertEndChild( doc.NewElement( "root" ) );

  root->InsertEndChild( doc.NewComment("-----------------------------------") );
  XMLElement* root_tree = doc.NewElement("BehaviorTree");
  root->InsertEndChild(root_tree);

  RecursivelyCreateXml(*scene, doc, root_tree, current_node );

  root->InsertEndChild( doc.NewComment("-----------------------------------") );

  XMLElement* root_models = doc.NewElement("TreeNodesModel");

  for(const auto& it: _tree_nodes_model)
  {
    const auto& ID    = it.first;
    const auto& model = it.second;

    XMLElement* node = doc.NewElement( toStr(model.node_type) );

    if( node )
    {
      node->SetAttribute("ID", ID.toStdString().c_str());
      for(const auto& it: model.params)
      {
        XMLElement* param_node = doc.NewElement( "Parameter" );
        param_node->InsertEndChild(root_models);
        param_node->SetAttribute("label", it.first.toStdString().c_str() );
        param_node->SetAttribute("type",  toStr( it.second ) );
        node->InsertEndChild(param_node);
      }
    }
    root_models->InsertEndChild(node);
  }
  root->InsertEndChild(root_models);
  root->InsertEndChild( doc.NewComment("-----------------------------------") );

  //-------------------------------------
  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  QString directory_path  = settings.value("MainWindow.lastSaveDirectory",
                                           QDir::currentPath() ).toString();

  QFileDialog saveDialog;
  saveDialog.setAcceptMode(QFileDialog::AcceptSave);
  saveDialog.setDefaultSuffix("xml");
  saveDialog.setNameFilter("State Machine (*.xml)");
  saveDialog.setDirectory(directory_path);
  saveDialog.exec();

  QString fileName;
  if(saveDialog.result() == QDialog::Accepted && saveDialog.selectedFiles().size() == 1)
  {
    fileName = saveDialog.selectedFiles().at(0);
  }

  if (fileName.isEmpty()){
    return;
  }

  XMLPrinter printer;
  doc.Print( &printer );

  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream stream(&file);
    stream << printer.CStr() << endl;
  }

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("MainWindow.lastSaveDirectory", directory_path);
}

void MainWindow::on_actionZoom_In_triggered()
{
  FlowView* view = dynamic_cast<FlowView*>( ui->tabWidget->currentWidget() );
  if(view){
    view->scaleDown();
  }
}

void MainWindow::on_actionZoom_ut_triggered()
{
  FlowView* view = dynamic_cast<FlowView*>( ui->tabWidget->currentWidget() );
  if(view){
    view->scaleUp();
  }
}

void MainWindow::on_actionAuto_arrange_triggered()
{
  _undo_enabled = false;
  NodeReorder( * currentTabInfo()->scene );
  _undo_enabled = true;
  onPushUndo();
}

void MainWindow::onNodeCreated(QtNodes::Node& node)
{
  if( auto bt_node = dynamic_cast<BehaviorTreeNodeModel*>( node.nodeDataModel() ) )
  {
    connect( bt_node, &BehaviorTreeNodeModel::parameterUpdated,
             this, &MainWindow::onNodeParameterUpdated );

    connect( bt_node, &BehaviorTreeNodeModel::instanceNameChanged,
             this, &MainWindow::onPushUndo );
  }
  onPushUndo();
}

void MainWindow::onNodeSizeChanged()
{
  for(const auto& tab_it: _tab_info)
  {
    const auto& tab = tab_it.second;

    for (auto& node_it: tab.scene->nodes() )
    {
      QtNodes::Node* node = node_it.second.get();

      node->nodeGeometry().recalculateSize();
      node->nodeGraphicsObject().update();
    }
    tab.scene->update();
    NodeReorder( *(tab.scene) );
  }
}

void MainWindow::onSceneChanged()
{
  //qDebug() << "onSceneChanged " ;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  settings.setValue("MainWindow/geometry", saveGeometry());
  settings.setValue("MainWindow/windowState", saveState());
  QMainWindow::closeEvent(event);
}

/*
void MainWindow::updateStates(QXmlInputSource* source)
{
    //TODO

    std::unique_lock<std::mutex> lock(_mutex);

    if( !_root_node ) return;

    for (auto& it: _main_scene->nodes())
    {
        it.second->nodeGraphicsObject().setGeometryChanged();
    }

    QXmlSimpleReader parser;
    StateUpdateXmlHandler handler(_main_scene, _root_node);

    parser.setContentHandler( &handler );

    std::cout<<	"Start parsing"<< std::endl;

    if(parser.parse(source))
    {
        std::cout<<"Parsed Successfully!"<< std::endl;
    }
    else {
        std::cout<<"Parsing Failed..."  << std::endl;
    }

    for (auto& it: _main_scene->nodes())
    {
        it.second->nodeGraphicsObject().update();
    }
}
*/
MainWindow::TabInfo *MainWindow::currentTabInfo()
{
  int index = ui->tabWidget->currentIndex();
  QString tab_name = ui->tabWidget->tabText(index);

  auto it = _tab_info.find( tab_name );
  return (it != _tab_info.end()) ? &(it->second) : nullptr;
}


void MainWindow::lockEditing(bool locked)
{
  for(auto tab_it: _tab_info)
  {
    const QtNodes::FlowScene &scene = *(tab_it.second.scene);
    if( locked)
    {
      std::vector<QtNodes::Node*> roots = findRoots( scene );
      bool valid_root = (roots.size() == 1) && ( dynamic_cast<RootNodeModel*>(roots.front()->nodeDataModel() ));

      if( valid_root) _root_node = roots.front();
    }

    for (auto& nodes_it: scene.nodes() )
    {
      QtNodes::Node* node = nodes_it.second.get();
      node->nodeGraphicsObject().lock( locked );

      auto bt_model = dynamic_cast<BehaviorTreeNodeModel*>( node->nodeDataModel() );
      if( bt_model )
      {
        bt_model->lock(locked);
      }
      else{
        auto ctr_model = dynamic_cast<ControlNodeModel*>( node->nodeDataModel() );
        if( ctr_model )
        {
          ctr_model->lock(locked);
        }
      }
      if( !locked )
      {
        node->nodeGraphicsObject().setGeometryChanged();
        QtNodes::NodeStyle style;
        node->nodeDataModel()->setNodeStyle( style );
        node->nodeGraphicsObject().update();
      }
    }

    for (auto& scene_it: scene.connections() )
    {
      QtNodes::Connection* conn = scene_it.second.get();
      conn->getConnectionGraphicsObject().lock( locked );
    }
  }

  if( !locked ){
    emit updateGraphic();
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  auto view = dynamic_cast<QtNodes::FlowView*>(obj);
  if( view )
  {
    qDebug() << "A " << event->type();
    return false;
  }
  else
  {
    qDebug() << "B " << event->type();
    return QMainWindow::eventFilter(obj,event);
  }
}

void MainWindow::resizeEvent(QResizeEvent *)
{
  on_splitter_splitterMoved();
}

void MainWindow::on_selectMode_sliderPressed()
{
  const int new_value = (ui->selectMode->value() == 0) ? 1 : 0;
  ui->selectMode->setValue( new_value );
}

void MainWindow::on_selectMode_valueChanged(int value)
{
  bool locked = (value == 1);
  lockEditing( locked );

  QFont fontA = ui->labelEdit->font();
  fontA.setBold( !locked );
  ui->labelEdit->setFont( fontA );

  QFont fontB = ui->labelMonitor->font();
  fontB.setBold( locked );
  ui->labelMonitor->setFont( fontB );

  for (auto& it: _tab_info)
  {
    auto& scene = it.second.scene;
    scene->lock( locked );
  }
}

void MainWindow::onTimerUpdate()
{

}


void MainWindow::onNodeContextMenu(QtNodes::Node &node, const QPointF&)
{
  QMenu* nodeMenu = new QMenu(this);

  //--------------------------------
  createMorphSubMenu(node, nodeMenu);
  //--------------------------------
  auto *remove = new QAction("Remove ", nodeMenu);
  nodeMenu->addAction(remove);

  connect( remove, &QAction::triggered, [this,&node]()
  {
    currentTabInfo()->scene->removeNode(node);
  });
  //--------------------------------
  createSmartRemoveAction(node, nodeMenu);
  //--------------------------------
  nodeMenu->exec( QCursor::pos() );
}

void MainWindow::createMorphSubMenu(QtNodes::Node &node, QMenu* nodeMenu)
{
  const QString category = getCategory( node.nodeDataModel() );
  auto names_in_category = _model_registry->registeredModelsByCategory( category );
  names_in_category.erase( node.nodeDataModel()->name() );

  QMenu* morph_submenu = nodeMenu->addMenu("Morph into...");

  if( category == "Control")
  {
    auto out_connections = node.nodeState().connections(QtNodes::PortType::Out, 0);
    if( out_connections.size()>3)
    {
      names_in_category.erase( IfThenElseModel::staticName() );
    }
  }

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
        _undo_enabled = false;
        node.changeDataModel( _model_registry->create(name) );
        NodeReorder( *currentTabInfo()->scene );
        _undo_enabled = true;
        onPushUndo();
      });
    }
  }
}

void MainWindow::createSmartRemoveAction(QtNodes::Node &node, QMenu* nodeMenu)
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
        _undo_enabled = false;
        currentTabInfo()->scene->removeNode( *node_ptr );
        for( auto& it: conn_out)
        {
          auto child_node = it.second->getNode(PortType::In);
          currentTabInfo()->scene->createConnection( *child_node, 0, *parent_node, 0 );
        }
        NodeReorder( *currentTabInfo()->scene );
        _undo_enabled = true;
        onPushUndo();
      });
    }
  }
  else{
    smart_remove->setEnabled(false);
  }
}

void MainWindow::insertNodeInConnection(QtNodes::Connection& connection, QString node_name)
{
  _undo_enabled = false;
  auto scene = currentTabInfo()->scene;

  auto node_model = _model_registry->create(node_name);
  auto parent_node = connection.getNode(PortType::Out);
  auto child_node  = connection.getNode(PortType::In);

  QPointF pos = child_node->nodeGraphicsObject().pos();
  pos.setX( pos.x() - 50 );

  QtNodes::Node& inserted_node = scene->createNode( std::move(node_model), pos );

  scene->deleteConnection(connection);
  scene->createConnection(*child_node, 0, inserted_node, 0);
  scene->createConnection(inserted_node, 0, *parent_node, 0);
  NodeReorder( *scene );
  _undo_enabled = true;
  onPushUndo();
}


void MainWindow::onConnectionContextMenu(QtNodes::Connection &connection, const QPointF&)
{
  QMenu* nodeMenu = new QMenu(this);
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

void MainWindow::on_splitter_splitterMoved(int , int )
{
  this->update();
  QList<int> sizes = ui->splitter->sizes();
  const int maxLeftWidth = ui->leftFrame->maximumWidth();
  int totalWidth = sizes[0] + sizes[1];

  if( sizes[0] > maxLeftWidth)
  {
    sizes[0] = maxLeftWidth;
    sizes[1] = totalWidth - maxLeftWidth;
    ui->splitter->setSizes(sizes);
  }
}

void MainWindow::onPushUndo()
{
  if (ui->selectMode->value() == 1) return; //locked

  if( !_undo_enabled ) return;

  _undo_enabled.store(false);
  //-----------------
  currentTabInfo()->scene->update();
  const QByteArray state = currentTabInfo()->scene->saveToMemory();

  if( _current_state.size() > 0)
  {
    _undo_stack.push_back( _current_state );
  }
  _current_state = state;
  //-----------------
  _undo_enabled.store(true);
  //std::cout << _current_state.toStdString() << std::endl;
  qDebug() << "P: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
}

void MainWindow::onUndoInvoked()
{
  if (ui->selectMode->value() == 1) return; //locked

  if( _undo_stack.size() > 0)
  {
    _redo_stack.push_back( std::move(_current_state) );
    _current_state = std::move( _undo_stack.back() );
    _undo_stack.pop_back();

    _undo_enabled.store(false);
    auto& scene = currentTabInfo()->scene;
    scene->clearScene();
    currentTabInfo()->scene->loadFromMemory( _current_state );
    _undo_enabled.store(true);
    qDebug() << "U: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
  }
}

void MainWindow::onRedoInvoked()
{
  if (ui->selectMode->value() == 1) return; //locked

  if( _redo_stack.size() > 0)
  {
    _undo_stack.push_back( _current_state );
    _current_state = std::move( _redo_stack.back() );
    _redo_stack.pop_back();

    _undo_enabled.store(false);
    auto& scene = currentTabInfo()->scene;
    scene->clearScene();
    currentTabInfo()->scene->loadFromMemory( _current_state );
    _undo_enabled.store(true);
    qDebug() << "R: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
  }
}

void MainWindow::onNodeParameterUpdated(QString label, QWidget *)
{
  qDebug() << "parameter " << label << " updated";
  onPushUndo();
}

void MainWindow::onNodeDoubleClicked(QtNodes::Node &root_node)
{
  auto& scene = currentTabInfo()->scene;
  std::function<void(QtNodes::Node&)> selectRecursively;

  selectRecursively = [&](QtNodes::Node& node)
  {
    node.nodeGraphicsObject().setSelected(true);
    auto children = getChildren(*scene,node);
    for (auto& child: children)
    {
      selectRecursively(*child);
    }
  };

  selectRecursively(root_node);
}
