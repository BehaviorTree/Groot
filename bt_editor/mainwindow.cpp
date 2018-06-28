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
#include <QTabBar>
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

    _model_registry->registerModel<RootNodeModel>("Root");
    _model_registry->registerModel<SequenceModel>("Control");
    _model_registry->registerModel<SequenceStarModel>("Control");
    _model_registry->registerModel<FallbackModel>("Control");

    _model_registry->registerModel<RetryNodeModel>("Decorator");
    _model_registry->registerModel<NegationNodeModel>("Decorator");
    _model_registry->registerModel<RepeatNodeModel>("Decorator");

    _tree_nodes_model["Root"]         = { NodeType::ROOT, {} };
    _tree_nodes_model["Sequence"]     = { NodeType::CONTROL, {} };
    _tree_nodes_model["SequenceStar"] = { NodeType::CONTROL, {} };
    _tree_nodes_model["Fallback"]     = { NodeType::CONTROL, {} };
    _tree_nodes_model["Negation"]            = { NodeType::DECORATOR, {} };
    _tree_nodes_model["RetryUntilSuccesful"] = RetryNodeModel::NodeModel();
    _tree_nodes_model["Repeat"]              = RepeatNodeModel::NodeModel();

    _editor_widget = new SidepanelEditor(_tree_nodes_model, this);
    _replay_widget = new SidepanelReplay(this);

    ui->leftFrame->layout()->addWidget( _editor_widget );
    ui->leftFrame->layout()->addWidget( _replay_widget );

#ifdef ZMQ_FOUND
    _monitor_widget = new SidepanelMonitor(this);
    ui->leftFrame->layout()->addWidget( _monitor_widget );

    connect( ui->toolButtonConnect, &QToolButton::clicked,
             _monitor_widget, &SidepanelMonitor::on_Connect );

    connect( _monitor_widget, &SidepanelMonitor::connectionUpdate,
             this, &MainWindow::onConnectionUpdate );
#else
    ui->actionMonitor_mode->setVisible(false);
#endif

    updateCurrentMode();

    dynamic_cast<QVBoxLayout*>(ui->leftFrame->layout())->setStretch(1,1);

    createTab("BehaviorTree");

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
             this, &MainWindow::onLoadAbsBehaviorTree );

    connect( ui->toolButtonSaveFile, &QToolButton::clicked,
             this, &MainWindow::on_actionSave_triggered );

    connect( _replay_widget, &SidepanelReplay::changeNodeStyle,
             this, &MainWindow::onChangeNodesStyle);

#ifdef ZMQ_FOUND

    connect( _monitor_widget, &SidepanelMonitor::changeNodeStyle,
             this, &MainWindow::onChangeNodesStyle);

    connect( _monitor_widget, &SidepanelMonitor::loadBehaviorTree,
             this, &MainWindow::onLoadAbsBehaviorTree );
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

    settings.setValue("StartupDialog.Mode", toStr( _current_mode ) );

    QMainWindow::closeEvent(event);
}


GraphicContainer* MainWindow::createTab(const QString &name)
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

    connect( ti, &GraphicContainer::requestSubTreeExpand,
             this, &MainWindow::onRequestSubTreeExpand );

    //--------------------------------

    ti->view()->update();

    return ti;
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
        auto document_root = document.RootElement();
        if( xml_error == XMLError::XML_SUCCESS && document_root )
        {
            ReadTreeNodesModel( document_root, *_model_registry, _tree_nodes_model );
            _editor_widget->updateTreeView();

            onActionClearTriggered(false);

            bool error = false;
            QString err_message;
            auto saved_state = _current_state;
            try {
                const QSignalBlocker blocker( currentTabInfo() );
                std::cout<< "Starting parsing"<< std::endl;

                for (auto bt_root = document_root->FirstChildElement("BehaviorTree");
                     bt_root != nullptr;
                     bt_root = bt_root->NextSiblingElement("BehaviorTree"))
                {
                    auto tree = BuildTreeFromXML( bt_root );
                    QString tree_name("BehaviorTree");
                    if( bt_root->Attribute("ID") )
                    {
                        tree_name = bt_root->Attribute("ID");
                    }

                    onLoadAbsBehaviorTree(tree, tree_name);
                }
                std::cout<<"XML Parsed Successfully!"<< std::endl;

                if( document_root->Attribute("main_tree_to_execute"))
                {
                    QString main_bt_name = document_root->Attribute("main_tree_to_execute");
                    for (int i=0; i< ui->tabWidget->count(); i++)
                    {
                        if( ui->tabWidget->tabText( i ) == main_bt_name)
                        {
                            ui->tabWidget->tabBar()->moveTab(i, 0);
                            ui->tabWidget->setCurrentIndex(0);
                            break;
                        }
                    }
                }
                if( currentTabInfo() == nullptr)
                {
                    createTab("BehaviorTree");
                }
                else{
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
                loadSavedStateFromJson( saved_state );
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
    for (auto& it: _tab_info)
    {
        auto& container = it.second;
        if( !container->containsValidTree() )
        {
            QMessageBox::warning(this, tr("Oops!"),
                                 tr("Malformed behavior tree. File can not be saved"),
                                 QMessageBox::Cancel);
            return;
        }
    }

    //----------------------------
    using namespace tinyxml2;
    XMLDocument doc;
    XMLNode* root = doc.InsertEndChild( doc.NewElement( "root" ) );

    for (auto& it: _tab_info)
    {
        auto& container = it.second;
        auto  scene = container->scene();

        QtNodes::Node* root_node = container->loadedTree().rootNode()->corresponding_node;

        root->InsertEndChild( doc.NewComment("-----------------------------------") );
        XMLElement* root_element = doc.NewElement("BehaviorTree");
        root_element->SetAttribute("ID", it.first.toStdString().c_str());
        root->InsertEndChild(root_element);

        RecursivelyCreateXml(*scene, doc, root_element, root_node );
    }
    root->InsertEndChild( doc.NewComment("-----------------------------------") );

    XMLElement* root_models = doc.NewElement("TreeNodesModel");

    for(const auto& tree_it: _tree_nodes_model)
    {
        const auto& ID    = tree_it.first;
        const auto& model = tree_it.second;

        if( model.node_type == NodeType::ROOT )
        {
            continue;
        }
        XMLElement* node = doc.NewElement( toStr(model.node_type) );

        if( node )
        {
            node->SetAttribute("ID", ID.toStdString().c_str());
            for(const auto& param: model.params)
            {
                XMLElement* param_node = doc.NewElement( "Parameter" );
                param_node->InsertEndChild(root_models);
                param_node->SetAttribute("label",   param.label.toStdString().c_str() );
                param_node->SetAttribute("default", param.default_value.toStdString().c_str() );
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

    lockEditing( _current_mode != GraphicMode::EDITOR );
}


GraphicContainer* MainWindow::currentTabInfo()
{
    int index = ui->tabWidget->currentIndex();
    QString tab_name = ui->tabWidget->tabText(index);
    return getTabByName(tab_name);
}

GraphicContainer *MainWindow::getTabByName(const QString &tab_name)
{
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
    SavedState saved;
    int index = ui->tabWidget->currentIndex();
    saved.current_tab_name =   ui->tabWidget->tabText(index);

    for (auto& it: _tab_info)
    {
        saved.json_states[it.first] = it.second->scene()->saveToMemory();
    }

    if( _undo_stack.empty() || ( saved != _current_state &&  _undo_stack.back() != _current_state) )
    {
        _undo_stack.push_back( std::move(_current_state) );
        _redo_stack.clear();
    }
    _current_state = saved;

    qDebug() << "P: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
}

void MainWindow::onUndoInvoked()
{
    if ( _current_mode != GraphicMode::EDITOR ) return; //locked

    if( _undo_stack.size() > 0)
    {
        _redo_stack.push_back( std::move(_current_state) );
        _current_state = _undo_stack.back();
        _undo_stack.pop_back();

        loadSavedStateFromJson(_current_state);

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

        loadSavedStateFromJson(_current_state);

        qDebug() << "R: Undo size: " << _undo_stack.size() << " Redo size: " << _redo_stack.size();
    }
}

void MainWindow::onConnectionUpdate(bool connected)
{
    if(connected)
    {
        ui->toolButtonConnect->setStyleSheet("background-color: rgb(50, 150, 0); color:white");
        ui->toolButtonConnect->setText("Disconnect");
    }
    else{
        ui->toolButtonConnect->setStyleSheet(
                    "QToolButton {color:white; }"
                    "QToolButton:hover{ background-color: rgb(110, 110, 110); }"
                    "QToolButton:pressed{ background-color: rgb(50, 150, 0) }"
                    "QToolButton:disabled{color:gray; background-color: rgb(50, 50, 50) }");
        ui->toolButtonConnect->setText("Connect");
    }
}

void MainWindow::onRequestSubTreeExpand(GraphicContainer& container,
                                        QtNodes::Node& node)
{
    bool is_expanded_subtree  = dynamic_cast< SubtreeExpandedNodeModel*>( node.nodeDataModel() );
    bool is_collapsed_subtree = dynamic_cast< SubtreeNodeModel*>( node.nodeDataModel() );

    if( !is_expanded_subtree && !is_collapsed_subtree )
    {
        throw std::logic_error("passing to onRequestSubTreeExpand something that is not a SubTree");
    }

    if( is_expanded_subtree )
    {
        subTreeExpand( container, node, SUBTREE_COLLAPSE );
    }
    else if( is_collapsed_subtree )
    {
        subTreeExpand( container, node, SUBTREE_EXPAND );
    }
}

void MainWindow::loadSavedStateFromJson(const SavedState& saved_state)
{
    for(auto& it: saved_state.json_states)
    {
        auto container = getTabByName( it.first );
        container->loadFromJson( it.second );
        refreshNodesLayout( container->scene()->layout() );
    }
    for (int i=0; i< ui->tabWidget->count(); i++)
    {
        if( ui->tabWidget->tabText( i ) == saved_state.current_tab_name)
        {
            ui->tabWidget->setCurrentIndex(i);
            break;
        }
    }
    onSceneChanged();
}

void MainWindow::subTreeExpand(GraphicContainer &container,
                               QtNodes::Node &node,
                               MainWindow::SubtreeExpandOption option)
{
    const QSignalBlocker blocker( this );
    const QString& subtree_name = dynamic_cast<BehaviorTreeDataModel*>( node.nodeDataModel() )->registrationName();

    if( option == SUBTREE_EXPAND )
    {
        auto it = _tab_info.find( subtree_name );
        if( it == _tab_info.end())
        {
            qDebug() << "ERROR: not found " << subtree_name;
            return;
        }
        const auto& subtree  = it->second->loadedTree();
        auto new_node_ptr = container.substituteNode( &node, subtree_name + EXPANDED_SUFFIX );
        if( new_node_ptr )
        {
            container.appendTreeToNode( *new_node_ptr, subtree );
            container.nodeReorder();
            container.lockSubtreeEditing( *new_node_ptr, true );
        }
    }
    else if( option == SUBTREE_COLLAPSE )
    {
        const auto& conn_out = node.nodeState().connections(PortType::Out, 0 );
        QtNodes::Node* child_node = nullptr;
        if(conn_out.size() == 1)
        {
            child_node = conn_out.begin()->second->getNode( PortType::In );
        }
        else{
            return;
        }

        auto new_subtree_name = subtree_name.left( subtree_name.size()-EXPANDED_SUFFIX.length() );
        auto new_node_ptr = container.substituteNode( &node,  new_subtree_name );
        if( new_node_ptr && child_node)
        {
            container.deleteSubTreeRecursively( *child_node );
            container.nodeReorder();
        }
    }
    else if( option == SUBTREE_REFRESH && dynamic_cast<SubtreeExpandedNodeModel*>(node.nodeDataModel()) )
    {
        const auto& conn_out = node.nodeState().connections(PortType::Out, 0 );
        if(conn_out.size() != 1)
        {
            throw std::logic_error("subTreeExpand with SUBTREE_REFRESH, but not an expanded SubTree");
        }

        QtNodes::Node* child_node = conn_out.begin()->second->getNode( PortType::In );

        auto original_subtree_name =  subtree_name.left( EXPANDED_SUFFIX.length() );
        auto it = _tab_info.find(  original_subtree_name );
        if( it == _tab_info.end())
        {
            qDebug() << "ERROR: not found " <<  original_subtree_name;
            return;
        }
        const auto& subtree  = it->second->loadedTree();

        container.deleteSubTreeRecursively( *child_node );
        container.appendTreeToNode( node, subtree );
        container.nodeReorder();
        container.lockSubtreeEditing( node, true );
    }
}

void MainWindow::on_toolButtonReorder_pressed()
{
    onAutoArrange();
}

void MainWindow::on_toolButtonCenterView_pressed()
{
    currentTabInfo()->zoomHomeView();
}

void MainWindow::onLoadAbsBehaviorTree(const AbsBehaviorTree &tree, const QString &bt_name)
{
    {
        auto container = getTabByName(bt_name);
        if( !container )
        {
            container = createTab(bt_name);
        }
        const QSignalBlocker blocker( container );

        container->loadSceneFromTree( tree );
        container->nodeReorder();
    }
    _undo_stack.clear();
    _redo_stack.clear();
    onSceneChanged();
    onPushUndo();
}


void MainWindow::on_actionClear_triggered()
{
    onActionClearTriggered(true);
}


void MainWindow::onActionClearTriggered(bool create_new)
{
    for (auto& it: _tab_info)
    {
        it.second->clearScene();
    }
    _tab_info.clear();

    ui->tabWidget->clear();
    if( create_new )
    {
        createTab("BehaviorTree");
    }

    _editor_widget->clear();
    _replay_widget->clear();
#ifdef ZMQ_FOUND
    _monitor_widget->clear();
#endif
}


void MainWindow::updateCurrentMode()
{
    const bool NOT_EDITOR = _current_mode != GraphicMode::EDITOR;

    _editor_widget->setHidden( NOT_EDITOR );
    _replay_widget->setHidden( _current_mode != GraphicMode::REPLAY );
#ifdef ZMQ_FOUND
    _monitor_widget->setHidden( _current_mode != GraphicMode::MONITOR );
#endif

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
#ifdef ZMQ_FOUND
    ui->actionMonitor_mode->setEnabled( _current_mode != GraphicMode::MONITOR);
#endif
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
                auto abstract_tree = BuildTreeFromScene( scene );
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

void MainWindow::refreshExpandedSubtrees()
{
    auto container = currentTabInfo();
    if( !container){
        return;
    }
    auto scene = container->scene();
    auto root_node = findRoot( *scene );
    if( !root_node )
    {
        return;
    }

    std::vector<QtNodes::Node*> subtree_nodes;
    std::function<void(QtNodes::Node*)> selectRecursively;

    selectRecursively = [&](QtNodes::Node* node)
    {
        if(dynamic_cast<SubtreeExpandedNodeModel*>(node->nodeDataModel()))
        {
            subtree_nodes.push_back( node );
        }
        else{
            auto children = getChildren( scene, *node, false );
            for(auto child_node: children)
            {
                selectRecursively(child_node);
            }
        }
    };
    selectRecursively( root_node );

    for (auto subtree_node: subtree_nodes)
    {
        subTreeExpand( *container, *subtree_node, SUBTREE_REFRESH );
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

#ifdef ZMQ_FOUND
    _monitor_widget->clear();
#endif

    _replay_widget->clear();
}

void MainWindow::on_actionMonitor_mode_triggered()
{
#ifdef ZMQ_FOUND
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
#endif
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
        onActionClearTriggered(true);
        _replay_widget->clear();
        _current_mode = GraphicMode::REPLAY;
        updateCurrentMode();
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    QString tab_name = ui->tabWidget->tabText(index);
    auto tab = getTabByName(tab_name);
    if( tab )
    {
        const QSignalBlocker blocker( tab );
        tab->nodeReorder();
        _current_state.current_tab_name = ui->tabWidget->tabText( index );
        refreshExpandedSubtrees();
    }
}

bool MainWindow::SavedState::operator ==(const MainWindow::SavedState &other) const
{
    if( current_tab_name != other.current_tab_name ||
            json_states.size() != other.json_states.size())
    {
        return false;
    }
    for(auto& it: json_states  )
    {
        if( it.second != other.json_states.at( it.first ))
        {
            return false;
        }
    }
    return true;
}

void MainWindow::onChangeNodesStyle(const QString& bt_name,
                                    const std::unordered_map<int, NodeStatus>& node_status)
{
    auto tree = _tab_info[bt_name]->loadedTree();

    for (auto& it: node_status)
    {
        const int index = it.first;
        auto abs_node = tree.nodeAtIndex(index);
        abs_node->status = it.second;
        qDebug() << abs_node->instance_name << " -> " << tr(toStr(abs_node->status));
        auto& node = abs_node->corresponding_node;
        auto style = getStyleFromStatus( abs_node->status );
        node->nodeDataModel()->setNodeStyle( style.first );
        node->nodeGraphicsObject().update();

        const auto& conn_in = node->nodeState().connections(PortType::In, 0 );
        if(conn_in.size() == 1)
        {
            auto conn = conn_in.begin()->second;
            conn->setStyle( style.second );
            conn->connectionGraphicsObject().update();
        }
    }
}


