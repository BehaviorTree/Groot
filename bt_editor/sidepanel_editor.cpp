#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"
#include "custom_node_dialog.h"
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QFileInfo>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include "models/ActionNodeModel.hpp"


SidepanelEditor::SidepanelEditor(QtNodes::DataModelRegistry *registry,
                                 TreeNodeModels &tree_nodes_model,
                                 QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelEditor),
    _tree_nodes_model(tree_nodes_model),
    _model_registry(registry)
{
    ui->setupUi(this);   
    ui->paramsFrame->setHidden(true);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( ui->treeWidget, &QWidget::customContextMenuRequested,
             this, &SidepanelEditor::onContextMenu);

    ui->buttonLock->setChecked(true);
}

SidepanelEditor::~SidepanelEditor()
{
    delete ui;
}

void SidepanelEditor::updateTreeView()
{
    ui->treeWidget->clear();
    _tree_view_category_items.clear();

    for (const QString& category : {"Root", "Action", "Condition", "Control", "Decorator", "SubTree" } )
    {
      auto item = new QTreeWidgetItem(ui->treeWidget, {category});
      QFont font = item->font(0);
      font.setBold(true);
      font.setPointSize(11);
      item->setFont(0, font);
      item->setFlags( item->flags() ^ Qt::ItemIsDragEnabled );
      item->setFlags( item->flags() ^ Qt::ItemIsSelectable );
      _tree_view_category_items[ category ] = item;
    }

    for (const auto &it : _tree_nodes_model)
    {
      const QString& ID = it.first;
      const TreeNodeModel& model = it.second;

      const QString& category = toStr(model.node_type);
      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent, {ID});
      QFont font = item->font(0);
      font.setItalic( BuiltinNodeModels().count(ID) == 1 );
      font.setPointSize(11);
      item->setFont(0, font);
      item->setData(0, Qt::UserRole, ID);
      item->setTextColor(0, model.is_editable ? Qt::blue : Qt::black);
    }

    ui->treeWidget->expandAll();
}

void SidepanelEditor::clear()
{

}

void SidepanelEditor::on_treeWidget_itemSelectionChanged()
{
  auto selected_items = ui->treeWidget->selectedItems();
  if(selected_items.size() == 0)
  {
    ui->paramsFrame->setHidden(true);
  }
  else {
    auto selected_item = selected_items.front();
    QString item_name = selected_item->text(0);
    ui->paramsFrame->setHidden(false);
    ui->label->setText( item_name + QString(" Parameters"));

    const auto& model = _tree_nodes_model[item_name];
    int row = 0;

    ui->parametersTableWidget->setRowCount(model.params.size());

    for (const auto& param: model.params)
    {
      ui->parametersTableWidget->setItem(row,0, new QTableWidgetItem( param.label ));
      ui->parametersTableWidget->setItem(row,1, new QTableWidgetItem( param.default_value ));
      row++;
    }

    ui->parametersTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->parametersTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  }

}

void SidepanelEditor::on_lineEditFilter_textChanged(const QString &text)
{
  for (auto& it : _tree_view_category_items)
  {
    for (int i = 0; i < it.second->childCount(); ++i)
    {
      auto child = it.second->child(i);
      auto modelName = child->data(0, Qt::UserRole).toString();
      bool show = modelName.contains(text, Qt::CaseInsensitive);
      child->setHidden( !show);
    }
  }
}

void SidepanelEditor::addNewModel(const QString& name, const TreeNodeModel& model)
{
    if( _tree_nodes_model.count(name) == 0)
    {
        _tree_nodes_model[name] = model;
        updateTreeView();
        addToModelRegistry(*_model_registry,name, model );
    }
}

void SidepanelEditor::on_buttonAddNode_clicked()
{
    CustomNodeDialog dialog(_tree_nodes_model, QString(), this);
    if( dialog.exec() == QDialog::Accepted)
    {
        auto new_model = dialog.getTreeNodeModel();
        addNewModel( new_model.first, new_model.second );
    }
    updateTreeView();
}

void SidepanelEditor::onContextMenu(const QPoint& pos)
{
    auto selected_items = ui->treeWidget->selectedItems();
    if( selected_items.count() != 1)
    {
        return;
    }
    QTreeWidgetItem* selected_item = selected_items.front();
    QString selected_name          = selected_item->text(0);
    const TreeNodeModel& model     = _tree_nodes_model.at(selected_name);

    if( !model.is_editable )
    {
        return;
    }

    QMenu menu(this);
    QAction* edit   = menu.addAction("Edit");
    QAction* remove = menu.addAction("Remove");

    connect( edit, &QAction::triggered, this, [this, selected_name]()
    {
        CustomNodeDialog dialog(_tree_nodes_model, selected_name, this);
        if( dialog.exec() == QDialog::Accepted)
        {
            auto new_model = dialog.getTreeNodeModel();
            new_model.second.is_editable = !ui->buttonLock->isChecked();
            _tree_nodes_model.erase( selected_name );
            _model_registry->unregisterModel( selected_name );
            addNewModel( new_model.first, new_model.second );
        }
    } );

    connect( remove, &QAction::triggered, this,[this, selected_name]()
    {
        _tree_nodes_model.erase( selected_name );
        updateTreeView();
    } );

    QPoint globalPos = ui->treeWidget->mapToGlobal(pos);
    menu.exec(globalPos);

    QApplication::processEvents();
}


void SidepanelEditor::on_buttonUpload_clicked()
{
    using namespace tinyxml2;
    XMLDocument doc;

    XMLElement* root = doc.NewElement( "root" );
    doc.InsertEndChild( root );

    XMLElement* root_models = doc.NewElement("TreeNodesModel");

    for(const auto& tree_it: _tree_nodes_model)
    {
        const auto& ID    = tree_it.first;
        const auto& model = tree_it.second;

        if( BuiltinNodeModels().count(ID) != 0 )
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

    //-------------------------------------
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastSaveDirectory",
                                             QDir::currentPath() ).toString();

    QFileDialog saveDialog(this);
    saveDialog.setWindowTitle("Save TreeNodeModel to file");
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("xml");
    saveDialog.setNameFilter("BehaviorTree files (*.xml)");
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
    settings.setValue("SidepanelEditor.lastSaveDirectory", directory_path);

}

void SidepanelEditor::on_buttonDownload_clicked()
{
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastLoadDirectory",
                                             QDir::homePath() ).toString();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load TreenNodeModel from file"), directory_path,
                                                    tr("BehaviorTree files (*.xml)"));
    if (!QFileInfo::exists(fileName)){
        return;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)){
        return;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("SidepanelEditor.lastLoadDirectory", directory_path);
    settings.sync();

    //--------------------------------
    using namespace tinyxml2;
    XMLDocument doc;
    doc.LoadFile( fileName.toStdString().c_str() );

    if (doc.Error())
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel form file",
                             "The XML was not correctly loaded");
        return;
    }

    auto strEqual = [](const char* str1, const char* str2) -> bool {
        return strcmp(str1, str2) == 0;
    };

    const tinyxml2::XMLElement* xml_root = doc.RootElement();
    if (!xml_root || !strEqual(xml_root->Name(), "root"))
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel form file",
                             "The XML must have a root node called <root>");
        return;
    }

    auto meta_root = xml_root->FirstChildElement("TreeNodesModel");

    if (!meta_root)
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel form file",
                             "Expecting <TreeNodesModel> under <root>");
        return ;
    }

    TreeNodeModels custom_models;

    for( const XMLElement* node = meta_root->FirstChildElement();
         node != nullptr;
         node = node->NextSiblingElement() )
    {
        auto model_pair =  buildTreeNodeModel(node, true);
        const auto& name  = model_pair.first;
        const auto& model = model_pair.second;

        if( _tree_nodes_model.count(name) == 0)
        {
            custom_models[name] = model;
            addToModelRegistry(*_model_registry, name, model );
        }
    }

    MergeTreeNodeModels(this, _tree_nodes_model, custom_models );

    updateTreeView();
}

void SidepanelEditor::on_buttonLock_toggled(bool locked)
{
    static QIcon icon_locked( QPixmap(":/icons/svg/lock.svg" ) );
    static QIcon icon_unlocked( QPixmap(":/icons/svg/lock_open.svg") );

    ui->buttonLock->setIcon( locked ? icon_locked : icon_unlocked);

    for( auto& it: _tree_nodes_model)
    {
        const auto& name = it.first;
        auto& model = it.second;

        model.is_editable = (!locked && BuiltinNodeModels().count( name ) == 0);
    }
    updateTreeView();
}
