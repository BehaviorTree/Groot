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
#include <QTreeWidgetItem>
#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/NodeStyle>
#include <nodes/FlowView>
#include <nodes/DataModelRegistry>

#include "editor_flowscene.h"

#include "XmlParsers.hpp"
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


MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  _arrange_shortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this),
  _root_node(nullptr)
{
  ui->setupUi(this);

  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
  restoreState(settings.value("MainWindow/windowState").toByteArray());

  _model_registry = std::make_shared<QtNodes::DataModelRegistry>();

  _model_registry->registerModel<RootNodeModel>("Root");
  _model_registry->registerModel<SequenceModel>("Control");
  _model_registry->registerModel<SequenceStarModel>("Control");
  _model_registry->registerModel<FallbackModel>("Control");

  _model_registry->registerModel<IfThenElseModel>("Control");

  buildTreeView();

  createTab("Behaviortree");

  this->setMenuBar(ui->menubar);
  ui->menubar->setNativeMenuBar(false);

  connect( &_arrange_shortcut, &QShortcut::activated,
           this,   &MainWindow::onNodeMoved  );

  connect( &_periodic_timer, SIGNAL(timeout()), this, SLOT(onTimerUpdate()) );

  _periodic_timer.start(10);

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 10);

  QList<int> splitter_sizes = ui->splitter->sizes();

  if( splitter_sizes[0] < 300)
  {
    splitter_sizes[1] += splitter_sizes[0] - 300;
    splitter_sizes[0] = 300;
    ui->splitter->setSizes(splitter_sizes);
  }
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

  connect( this, SIGNAL(updateGraphic()),  ti.view,   SLOT(repaint())  );

  connect( ti.scene, &QtNodes::FlowScene::nodeContextMenu,
           this, &MainWindow::onNodeContextMenu );

  connect( ti.scene, &QtNodes::FlowScene::connectionContextMenu,
           this, &MainWindow::onConnectionContextMenu );

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
      ReadTreeNodesModel( *_model_registry ,
                          document.RootElement()->FirstChildElement("TreeNodesModel")
                          );
      buildTreeView();

      currentTabInfo()->scene->clearScene();
      QtNodes::Node& first_qt_node = currentTabInfo()->scene->createNode( _model_registry->create("Root") );

      std::cout<< "Starting parsing"<< std::endl;

      ParseBehaviorTreeXML(document.RootElement()->FirstChildElement("BehaviorTree"),
                           currentTabInfo()->scene,
                           first_qt_node);

      std::cout<<"XML Parsed Successfully!"<< std::endl;

      NodeReorder( *currentTabInfo()->scene );
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

void MainWindow::buildTreeView()
{
  auto AdjustFont = [](QTreeWidgetItem* item, int size, bool is_bold)
  {
    QFont font = item->font(0);
    font.setBold(is_bold);
    font.setPointSize(size);
    item->setFont(0, font);
  };

  auto skipText = QStringLiteral("skip me");

  //Add filterbox to the context menu

  ui->lineEditFilter->setPlaceholderText(QStringLiteral("Filter"));
  ui->lineEditFilter->setClearButtonEnabled(true);

  ui->treeWidget->clear();
  _tree_view_top_level_items.clear();

  for (auto const &cat : _model_registry->categories())
  {
    auto item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, cat);
    AdjustFont(item, 12, true);
    item->setData(0, Qt::UserRole, skipText);
    item->setFlags( item->flags() ^ Qt::ItemIsDragEnabled );
    _tree_view_top_level_items[cat] = item;
  }

  for (auto const &assoc : _model_registry->registeredModelsCategoryAssociation())
  {
    const QString& category = assoc.second;
    auto parent = _tree_view_top_level_items[category];
    auto item = new QTreeWidgetItem(parent);
    item->setText(0, assoc.first);
    AdjustFont(item, 11, false);
    item->setData(0, Qt::UserRole, assoc.first);
  }

  ui->treeWidget->expandAll();

  //Setup filtering
  connect(ui->lineEditFilter, &QLineEdit::textChanged, [&](const QString &text)
  {
    for (auto& topLvlItem : _tree_view_top_level_items)
    {
      for (int i = 0; i < topLvlItem->childCount(); ++i)
      {
        auto child = topLvlItem->child(i);
        auto modelName = child->data(0, Qt::UserRole).toString();
        if (modelName.contains(text, Qt::CaseInsensitive))
        {
          child->setHidden(false);
        }
        else
        {
          child->setHidden(true);
        }
      }
    }
  });
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

/*
void MainWindow::recursivelyCreateXml(QDomDocument& doc, QDomElement& parent_element, const QtNodes::Node* node)
{
       const QtNodes::NodeDataModel* node_model = node->nodeDataModel();
    const QString model_name = node_model->name();
    QDomElement element = doc.createElement( model_name );
    if( model_name == "Action" || model_name == "Decorator")
    {
        const auto* action_node = dynamic_cast<const BehaviorTreeNodeModel*>(node_model);
        if( action_node )
        {
            element.setAttribute("ID", action_node->type() );
            auto parameters = action_node->getCurrentParameters();
            for(const auto& param: parameters)
            {
                element.setAttribute( param.first, param.second );
            }
        }
    }

    if( element.attribute("ID") != node_model->caption())
    {
        element.setAttribute("name", node_model->caption() );
    }
    parent_element.appendChild( element );

    auto node_children = getChildren(*_main_scene, *node );
    for(QtNodes::Node* child : node_children)
    {
        recursivelyCreateXml(doc, element, child );
    }
}
*/

void MainWindow::on_actionSave_triggered()
{
  /*    std::vector<QtNodes::Node*> roots = findRoots( *_main_scene );
    bool valid_root = (roots.size() == 1) && ( dynamic_cast<RootNodeModel*>(roots.front()->nodeDataModel() ));

    QtNodes::Node* current_node = nullptr;

    if( valid_root ){
        auto root_children = getChildren(*_main_scene, *roots.front() );
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
    QDomElement root_element = _domDocument.documentElement();

    QDomElement bt_node = root_element.firstChildElement( "BehaviorTree" );
    if( !bt_node.isNull() )
    {
        while( bt_node.childNodes().size() > 0)
        {
           bt_node.removeChild( bt_node.firstChild() );
        }
    }
    else{
        bt_node = _domDocument.createElement( "BehaviorTree" );
        root_element.appendChild(bt_node);
    }

    recursivelyCreateXml(_domDocument, bt_node, current_node );

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

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << _domDocument.toString(4) << endl;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("MainWindow.lastSaveDirectory", directory_path);*/
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
  NodeReorder( * currentTabInfo()->scene );
}

void MainWindow::onNodeMoved()
{

  NodeReorder( * currentTabInfo()->scene );
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
  bool locked = value == 1;
  lockEditing( locked );

  QFont fontA = ui->labelEdit->font();
  fontA.setBold( !locked );
  ui->labelEdit->setFont( fontA );

  QFont fontB = ui->labelMonitor->font();
  fontB.setBold( locked );
  ui->labelMonitor->setFont( fontB );
}

void MainWindow::onTimerUpdate()
{

}

void MainWindow::onNodeContextMenu(QtNodes::Node &node, const QPointF &pos)
{
  const QString category = getCategory( node.nodeDataModel() );
  const auto cursor_pos = QCursor::pos();
  auto names_in_category = _model_registry->registeredModelsByCategory( category );
  names_in_category.erase( node.nodeDataModel()->name() );

  QMenu* nodeMenu = new QMenu(this);

  if( category == "Control")
  {
    auto out_connections = node.nodeState().connections(QtNodes::PortType::Out, 0);
    if( out_connections.size()>3)
    {
      names_in_category.erase( IfThenElseModel::staticName() );
    }
  }

  if( names_in_category.size() > 0)
  {
    QMenu* morph_submenu = nodeMenu->addMenu("Morph into...");
    for(auto& name: names_in_category)
    {
      auto action = new QAction(name, morph_submenu);
      morph_submenu->addAction(action);

      auto scene = currentTabInfo()->scene;
      connect( action, &QAction::triggered, [this,&node, name, scene]
      {
        node.changeDataModel( _model_registry->create(name) );
        NodeReorder( *currentTabInfo()->scene );
      });
    }
  }

  auto *remove = new QAction("Remove", nodeMenu);
  nodeMenu->addAction(remove);

  nodeMenu->exec( cursor_pos );
}

void MainWindow::onConnectionContextMenu(QtNodes::Connection &, const QPointF&)
{
  QMenu nodeMenu;
  auto *insertControl   = new QAction("Insert ControlNode", &nodeMenu);
  auto *insertDecorator = new QAction("Insert DecoratoNode", &nodeMenu);

  nodeMenu.addAction(insertControl);
  nodeMenu.addAction(insertDecorator);

  nodeMenu.exec( QCursor::pos() );
}

void MainWindow::on_splitter_splitterMoved(int , int )
{
  QList<int> sizes = ui->splitter->sizes();
  const int maxLeftWidth = ui->treeWidget->maximumWidth();
  int totalWidth = sizes[0] + sizes[1];

  if( sizes[0] > maxLeftWidth)
  {
    sizes[0] = maxLeftWidth;
    sizes[1] = totalWidth - maxLeftWidth;
    ui->splitter->setSizes(sizes);
  }
}
