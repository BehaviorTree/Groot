#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSettings>
#include <QTextStream>
#include <QList>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QToolButton>
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


MainWindow::MainWindow(GraphicMode initial_mode, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _current_mode(initial_mode)
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

    ui->leftFrame->layout()->addWidget( _editor_widget );
    ui->leftFrame->layout()->addWidget( _replay_widget );

#ifdef ZMQ_FOUND
    _monitor_widget = new SidepanelMonitor(this);
    ui->leftFrame->layout()->addWidget( _monitor_widget );

    connect( ui->toolButtonConnect, &QToolButton::clicked,
             _monitor_widget, &SidepanelMonitor::on_Connect );
#endif

    updateCurrentMode();

    dynamic_cast<QVBoxLayout*>(ui->leftFrame->layout())->setStretch(1,1);

    createTab("Behaviortree");

    auto arrange_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);

    connect( arrange_shortcut, &QShortcut::activated,
             this,   &MainWindow::onAutoArrange  );

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);

    QShortcut* undo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this);
    connect( undo_shortcut, &QShortcut::activated, this, &MainWindow::onUndoInvoked );

    QShortcut* redo_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this);
    connect( redo_shortcut, &QShortcut::activated, this, &MainWindow::onRedoInvoked );

    connect( _replay_widget, &SidepanelReplay::loadBehaviorTree,
             this, &MainWindow::on_loadBehaviorTree );

    connect( ui->toolButtonSaveFile, &QToolButton::clicked,
             this, &MainWindow::on_actionSave_triggered );

#ifdef ZMQ_FOUND
    connect( _monitor_widget, &SidepanelMonitor::loadBehaviorTree,
             this, &MainWindow::on_loadBehaviorTree );
#endif
    onSceneChanged();

    const QString layout = settings.value("MainWindow/layout").toString();
    if( layout == "HORIZONTAL")
    {
        refreshNodesLayout( QtNodes::PortLayout::Horizontal );
    }
    else{
        refreshNodesLayout( QtNodes::PortLayout::Vertical );
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("EurecatRobotics", "BehaviorTreeEditor");

    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());

    switch(_current_layout )
    {
    case QtNodes::PortLayout::Horizontal:  settings.setValue("MainWindow/layout", "HORIZONTAL");
        break;
    case QtNodes::PortLayout::Vertical:  settings.setValue("MainWindow/layout", "VERTICAL");
        break;
    }

    switch(_current_mode )
    {
    case GraphicMode::EDITOR:  settings.setValue("StartupDialog.Mode", "EDITOR");
        break;
    case GraphicMode::MONITOR:  settings.setValue("StartupDialog.Mode", "MONITOR");
        break;
    case GraphicMode::REPLAY:  settings.setValue("StartupDialog.Mode", "REPLAY");
        break;
    }

    QMainWindow::closeEvent(event);
}


void MainWindow::createTab(const QString &name)
{
    if( _tab_info.count(name) > 0)
    {
        throw std::runtime_error(std::string("There is already a Tab named ") + name.toStdString() );
    }
    GraphicContainer* ti = new GraphicContainer( _model_registry, this );
    _tab_info[name] = ti;

    ti->scene()->setLayout( _current_layout );

    ui->tabWidget->addTab( ti->view(), name );

    //--------------------------------

    connect( ti, &GraphicContainer::undoableChange,
             this, &MainWindow::onPushUndo );

    connect( ti, &GraphicContainer::undoableChange,
             this, &MainWindow::onSceneChanged );

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
        XMLError xml_error = document.Parse( xml_text.toStdString().c_str(), xml_text.size() );
        if( !xml_error )
        {
            ReadTreeNodesModel( document.RootElement(), *_model_registry, _tree_nodes_model );
            _editor_widget->updateTreeView();

            currentTabInfo()->clearScene();

            bool error = false;
            QString err_message;
            QByteArray saved_state = _current_state;
            try {
                {
                    const QSignalBlocker blocker( currentTabInfo() );
                    std::cout<< "Starting parsing"<< std::endl;
                    CreateTreeInSceneFromXML(document.RootElement()->FirstChildElement("BehaviorTree"),
                                             currentTabInfo()->scene() );
                    std::cout<<"XML Parsed Successfully!"<< std::endl;
                    currentTabInfo()->nodeReorder();
                }
            }
            catch (std::runtime_error& err) {
                error = true;
                err_message = err.what();
            }
            catch (std::logic_error& err) {
                error = true;
                err_message = err.what();
            }

            if( error )
            {
                loadSceneFromYAML( saved_state );
                qDebug() << "R: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
                QMessageBox::warning(this, tr("Exception!"),
                                     tr("It was not possible to parse the file. Error:\n\n%1"). arg( err_message ),
                                     QMessageBox::Ok);
            }
            else{
                onSceneChanged();
                onPushUndo();
            }
        }
    }
    catch( std::runtime_error& err)
    {
        QMessageBox messageBox;
        messageBox.critical(this,"Error", err.what() );
        messageBox.show();
        return;
    }

    lockEditing( _current_mode != GraphicMode::EDITOR );
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

void MainWindow::onAutoArrange()
{
    currentTabInfo()->nodeReorder();
}

void MainWindow::onSceneChanged()
{
    const bool valid_BT = currentTabInfo()->containsValidTree();

    ui->toolButtonLayout->setEnabled(valid_BT);
    ui->toolButtonReorder->setEnabled(valid_BT);
    ui->toolButtonReorder->setEnabled(valid_BT);

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
    ui->labelSemaphore->setScaledContents(true);
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
    const QByteArray state = currentTabInfo()->scene()->saveToMemory();

    if( _undo_stack.empty() || ( state != _current_state &&  _undo_stack.back() != _current_state) )
    {
        _undo_stack.push_back( _current_state );
        _redo_stack.clear();
    }
    _current_state = state;

    qDebug() << "P: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
}

void MainWindow::onUndoInvoked()
{
    if ( _current_mode != GraphicMode::EDITOR ) return; //locked

    if( _undo_stack.size() > 0)
    {
        _redo_stack.push_back( _current_state );
        _current_state = _undo_stack.back();
        _undo_stack.pop_back();

        loadSceneFromYAML(_current_state);

        qDebug() << "U: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
    }
}


void MainWindow::onRedoInvoked()
{
    if ( _current_mode != GraphicMode::EDITOR ) return; //locked

    if( _redo_stack.size() > 0)
    {
        _undo_stack.push_back( _current_state );
        _current_state = std::move( _redo_stack.back() );
        _redo_stack.pop_back();

        loadSceneFromYAML(_current_state);

        qDebug() << "R: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
    }
}

void MainWindow::loadSceneFromYAML(QByteArray state)
{
    {
        const QSignalBlocker blocker( currentTabInfo() );
        auto scene = currentTabInfo()->scene();
        scene->clearScene();
        scene->loadFromMemory( state );
        refreshNodesLayout( scene->layout() );
    }
    onSceneChanged();
}
void MainWindow::on_toolButtonReorder_pressed()
{
    onAutoArrange();
}

void MainWindow::on_toolButtonCenterView_pressed()
{
    currentTabInfo()->zoomHomeView();
}

void MainWindow::on_loadBehaviorTree(AbsBehaviorTree &tree)
{
    {
        const QSignalBlocker blocker( currentTabInfo() );
        BuildSceneFromBehaviorTree( currentTabInfo()->scene(), tree );
        currentTabInfo()->nodeReorder();
    }
    _undo_stack.clear();
    _redo_stack.clear();
    onSceneChanged();
    onPushUndo();
}

void MainWindow::on_actionClear_triggered()
{
    currentTabInfo()->clearScene();
    _editor_widget->clear();
    _monitor_widget->clear();
    _replay_widget->clear();
}


void MainWindow::updateCurrentMode()
{
    const bool NOT_EDITOR = _current_mode != GraphicMode::EDITOR;

    _editor_widget->setHidden( NOT_EDITOR );
    _replay_widget->setHidden( _current_mode != GraphicMode::REPLAY );
    _monitor_widget->setHidden( _current_mode != GraphicMode::MONITOR );

    ui->toolButtonLoadFile->setHidden( _current_mode == GraphicMode::MONITOR );
    ui->toolButtonConnect->setHidden( _current_mode != GraphicMode::MONITOR );
    ui->toolButtonLoadRemote->setHidden( NOT_EDITOR );
    ui->toolButtonSaveFile->setHidden( NOT_EDITOR );
    ui->toolButtonReorder->setHidden( NOT_EDITOR );

    if( _current_mode == GraphicMode::EDITOR )
    {
        connect( ui->toolButtonLoadFile, &QToolButton::clicked,
                 this, &MainWindow::on_actionLoad_triggered );
        disconnect( ui->toolButtonLoadFile, &QToolButton::clicked,
                    _replay_widget, &SidepanelReplay::on_LoadLog );
    }
    else if( _current_mode == GraphicMode::REPLAY )
    {
        disconnect( ui->toolButtonLoadFile, &QToolButton::clicked,
                    this, &MainWindow::on_actionLoad_triggered );
        connect( ui->toolButtonLoadFile, &QToolButton::clicked,
                 _replay_widget, &SidepanelReplay::on_LoadLog );
    }
    lockEditing( NOT_EDITOR );

    if( _current_mode == GraphicMode::EDITOR)
    {
        _editor_widget->updateTreeView();
    }
    ui->actionEditor_mode->setEnabled( _current_mode != GraphicMode::EDITOR);
    ui->actionMonitor_mode->setEnabled( _current_mode != GraphicMode::MONITOR);
    ui->actionReplay_mode->setEnabled( _current_mode != GraphicMode::REPLAY);
}


void MainWindow::refreshNodesLayout(QtNodes::PortLayout new_layout)
{
    if( new_layout != _current_layout)
    {
        QString icon_name = ( new_layout == QtNodes::PortLayout::Horizontal ) ?
                    ":/icons/BT-horizontal.png" :
                    ":/icons/BT-vertical.png";
        QIcon icon;
        icon.addFile(icon_name, QSize(), QIcon::Normal, QIcon::Off);
        ui->toolButtonLayout->setIcon(icon);
        ui->toolButtonLayout->update();
    }

    bool refreshed = false;
    {
        const QSignalBlocker blocker( currentTabInfo() );
        for(auto& tab: _tab_info)
        {
            auto scene = tab.second->scene();
            if( scene->layout() != new_layout )
            {
                auto abstract_tree = BuildBehaviorTreeFromScene( scene );
                scene->setLayout( new_layout );
                NodeReorder( *scene, abstract_tree );
                refreshed = true;
            }
        }
        on_toolButtonCenterView_pressed();
    }
    _current_layout = new_layout;
    if(refreshed)
    {
        onPushUndo();
    }
}

void MainWindow::on_toolButtonLayout_clicked()
{
    if( _current_layout == QtNodes::PortLayout::Horizontal)
    {
        refreshNodesLayout( QtNodes::PortLayout::Vertical );
    }
    else{
        refreshNodesLayout( QtNodes::PortLayout::Horizontal );
    }
}

void MainWindow::on_actionEditor_mode_triggered()
{
    _current_mode = GraphicMode::EDITOR;
    updateCurrentMode();
}

void MainWindow::on_actionMonitor_mode_triggered()
{
    QMessageBox::StandardButton res = QMessageBox::Ok;

    if( currentTabInfo()->scene()->nodes().size() > 0)
    {
        res = QMessageBox::warning(this, tr("Carefull!"),
                                   tr("If you switch to Monitor Mode, "
                                      "the current BehaviorTree in the Scene will be deleted"),
                                   QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel);
    }
    if( res == QMessageBox::Ok)
    {
        currentTabInfo()->clearScene();
        _monitor_widget->clear();
        _current_mode = GraphicMode::MONITOR;
        updateCurrentMode();
    }
}

void MainWindow::on_actionReplay_mode_triggered()
{
    QMessageBox::StandardButton res = QMessageBox::Ok;

    if( currentTabInfo()->scene()->nodes().size() > 0)
    {
        res = QMessageBox::warning(this, tr("Carefull!"),
                                   tr("If you switch to Log Replay Mode, "
                                      "the current BehaviorTree in the Scene will be deleted"),
                                   QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel);
    }
    if( res == QMessageBox::Ok)
    {
        currentTabInfo()->clearScene();
        _replay_widget->clear();
        _current_mode = GraphicMode::REPLAY;
        updateCurrentMode();
    }
}
