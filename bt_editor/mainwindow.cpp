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

  _editor_widget = new SidepanelEditor(_tree_nodes_model, this);
  _replay_widget = new SidepanelReplay(this);
  _replay_widget->setHidden(true);

  ui->leftFrame->layout()->addWidget( _editor_widget );
  ui->leftFrame->layout()->addWidget( _replay_widget );

  createTab("Behaviortree");

  this->setMenuBar(ui->menubar);
  ui->menubar->setNativeMenuBar(false);

  auto arrange_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);

  connect( arrange_shortcut, &QShortcut::activated,
           this,   &MainWindow::on_actionAuto_arrange_triggered  );

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 5);

  QShortcut* undo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this);
  connect( undo_shortcut, &QShortcut::activated, this, &MainWindow::onUndoInvoked );

  QShortcut* redo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this);
  connect( redo_shortcut, &QShortcut::activated, this, &MainWindow::onRedoInvoked );

  connect( _replay_widget, &SidepanelReplay::loadBehaviorTree,
           this, &MainWindow::on_loadBehaviorTree );

  onSceneChanged();
}


void MainWindow::createTab(const QString &name)
{
  if( _tab_info.count(name) > 0)
  {
    throw std::runtime_error(std::string("There is already a Tab named ") + name.toStdString() );
  }
  GraphicContainer* ti = new GraphicContainer( _model_registry, this );
  _tab_info[name] = ti;

  ti->scene()->setLayout( ui->comboBoxLayout->currentIndex() == 0 ?
                            QtNodes::PortLayout::Horizontal :
                            QtNodes::PortLayout::Vertical );

  ui->tabWidget->addTab( ti->view(), name );

  //--------------------------------

  connect( ti, &GraphicContainer::undoableChange,
           this, &MainWindow::onPushUndo );

  connect( ti, &GraphicContainer::undoableChange,
           this, &MainWindow::onSceneChanged );

  connect( this, SIGNAL(updateGraphic()), ti->view(), SLOT(repaint())  );

  //--------------------------------

  ti->view()->update();
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
      _editor_widget->updateTreeView();

      onPushUndo();
      {
        const QSignalBlocker blocker( currentTabInfo() );

        currentTabInfo()->scene()->clearScene();

        std::cout<< "Starting parsing"<< std::endl;

        CreateTreeInSceneFromXML(document.RootElement()->FirstChildElement("BehaviorTree"),
                             currentTabInfo()->scene() );

        std::cout<<"XML Parsed Successfully!"<< std::endl;

        currentTabInfo()->nodeReorder();
      }
      onSceneChanged();
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

  lockEditing( !ui->radioEditor->isChecked() );
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
  const QtNodes::FlowScene* scene = currentTabInfo()->scene();

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

void MainWindow::on_actionZoom_Out_triggered()
{
  FlowView* view = dynamic_cast<FlowView*>( ui->tabWidget->currentWidget() );
  if(view){
    view->scaleDown();
  }
}

void MainWindow::on_actionZoom_In_triggered()
{
  FlowView* view = dynamic_cast<FlowView*>( ui->tabWidget->currentWidget() );
  if(view){
    view->scaleUp();
  }
}

void MainWindow::on_actionAuto_arrange_triggered()
{
  currentTabInfo()->nodeReorder();
}

void MainWindow::onSceneChanged()
{
  bool valid_BT = (findRoots( *currentTabInfo()->scene() ).size() == 1);

  ui->comboBoxLayout->setEnabled(valid_BT);
  ui->pushButtonReorder->setEnabled(valid_BT);
  ui->actionAuto_arrange->setEnabled(valid_BT);
  ui->actionSave->setEnabled(valid_BT);
  QPixmap pix;

  if(valid_BT)
  {
    pix.load(":/icons/green-circle.png");
    ui->labelSemaphore->setToolTip("Valid Tree");
  }
  else{
    pix.load(":/icons/red-circle.png");
    ui->labelSemaphore->setToolTip("NOT a valid Tree");
  }

  ui->labelSemaphore->setPixmap(pix);
  ui->labelSemaphore->setFixedSize( QSize(24,24) );
  ui->labelSemaphore->setScaledContents(true);

}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  settings.setValue("MainWindow/geometry", saveGeometry());
  settings.setValue("MainWindow/windowState", saveState());
  QMainWindow::closeEvent(event);
}

GraphicContainer* MainWindow::currentTabInfo()
{
  int index = ui->tabWidget->currentIndex();
  QString tab_name = ui->tabWidget->tabText(index);

  auto it = _tab_info.find( tab_name );
  return (it != _tab_info.end()) ? (it->second) : nullptr;
}


void MainWindow::lockEditing(bool locked)
{
  for(auto& tab_it: _tab_info)
  {
    tab_it.second->lockEditing(locked);
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
 // if ( !ui->radioEditor->isChecked() ) return; //locked

  if( !_undo_enabled ) return;

  //-----------------
  const QSignalBlocker blocker( currentTabInfo() );
  _undo_enabled = false;

  currentTabInfo()->scene()->update();
  const QByteArray state = currentTabInfo()->scene()->saveToMemory();

  if( _current_state.size() )
  {
    if( _undo_stack.empty() ||
        ( state != _current_state && _undo_stack.back() != _current_state))
    {
      _undo_stack.push_back( _current_state );
      _redo_stack.clear();
    }
  }
  _current_state = state;
  _undo_enabled = true;
  //-----------------
  qDebug() << "P: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
}

void MainWindow::onUndoInvoked()
{
  if ( !ui->radioEditor->isChecked() ) return; //locked

  if( _undo_stack.size() > 0)
  {
    _redo_stack.push_back( _current_state );
    _current_state = _undo_stack.back();
    _undo_stack.pop_back();

    qDebug() << "U: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();

    {
      const QSignalBlocker blocker( currentTabInfo() );
      _undo_enabled = false;
      auto scene = currentTabInfo()->scene();
      scene->clearScene();
      currentTabInfo()->scene()->loadFromMemory( _current_state );

      int combo_index = ( currentTabInfo()->scene()->layout() == QtNodes::PortLayout::Horizontal) ? 0 : 1;
      if( combo_index != ui->comboBoxLayout->currentIndex() )
      {
        ui->comboBoxLayout->setCurrentIndex(combo_index);
        QApplication::processEvents();
      }
      _undo_enabled = true;
    }

    onSceneChanged();

    qDebug() << "U: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
  }
}

void MainWindow::onRedoInvoked()
{
  if ( !ui->radioEditor->isChecked() ) return; //locked

  if( _redo_stack.size() > 0)
  {
    _undo_stack.push_back( _current_state );
    _current_state = std::move( _redo_stack.back() );
    _redo_stack.pop_back();

    {
      const QSignalBlocker blocker( currentTabInfo() );
      _undo_enabled = false;
      auto scene = currentTabInfo()->scene();
      scene->clearScene();
      currentTabInfo()->scene()->loadFromMemory( _current_state );

      int combo_index = ( currentTabInfo()->scene()->layout() == QtNodes::PortLayout::Horizontal) ? 0 : 1;
      if( combo_index != ui->comboBoxLayout->currentIndex() )
      {
          ui->comboBoxLayout->setCurrentIndex(combo_index);
      }
      _undo_enabled = true;
    }
    onSceneChanged();

    qDebug() << "R: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
  }
}


void MainWindow::on_comboBoxLayout_currentIndexChanged(int index)
{
  auto new_layout = (index==0) ? QtNodes::PortLayout::Horizontal :
                                 QtNodes::PortLayout::Vertical;

  bool refresh = false;
  {
    const QSignalBlocker blocker( currentTabInfo() );
    for(auto& tab: _tab_info)
    {
      auto scene = tab.second->scene();
      if( scene->layout() != new_layout )
      {
        auto abstract_tree = BuildBehaviorTreeFromScene( scene );

        scene->setLayout( (index==0) ? QtNodes::PortLayout::Horizontal :
                                       QtNodes::PortLayout::Vertical);

        NodeReorder( *scene, abstract_tree );
        refresh = true;
      }
    }
    on_pushButtonCenterView_pressed();
  }
  onPushUndo();
}

void MainWindow::on_pushButtonReorder_pressed()
{
    on_actionAuto_arrange_triggered();
}

void MainWindow::on_pushButtonCenterView_pressed()
{
  currentTabInfo()->zoomHomeView();
}

void MainWindow::on_radioEditor_toggled(bool checked)
{
  lockEditing( !checked );
  if( checked )
  {
//    ui->radioMonitor->setChecked(false);
//    ui->radioReplay->setChecked(false);

    _editor_widget->setHidden( false );
    _replay_widget->setHidden( true );
    ui->leftFrame->update();
  }
}

void MainWindow::on_radioMonitor_toggled(bool checked)
{
  if( checked )
  {
//    ui->radioEditor->setChecked(false);
//    ui->radioReplay->setChecked(false);

  }
}

void MainWindow::on_radioReplay_toggled(bool checked)
{
  if( checked )
  {
//    ui->radioMonitor->setChecked(false);
//    ui->radioEditor->setChecked(false);

    _editor_widget->setHidden( true );
    _replay_widget->setHidden( false );
    ui->leftFrame->update();
  }
}

void MainWindow::on_loadBehaviorTree(AbsBehaviorTree tree)
{
    {
        const QSignalBlocker blocker( currentTabInfo() );
        auto scene = currentTabInfo()->scene();

        scene->clearScene();

        BuildSceneFromBehaviorTree( scene, tree);
        onSceneChanged();
        scene->update();

        currentTabInfo()->nodeReorder();
    }
    _undo_stack.clear();
    _redo_stack.clear();
    onPushUndo();
}
