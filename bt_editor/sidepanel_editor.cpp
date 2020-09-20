#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"
#include "custom_node_dialog.h"
#include "utils.h"

#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QFileInfo>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>

SidepanelEditor::SidepanelEditor(QtNodes::DataModelRegistry *registry,
                                 NodeModels &tree_nodes_model,
                                 QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelEditor),
    _tree_nodes_model(tree_nodes_model),
    _model_registry(registry)
{
    ui->setupUi(this);   
    ui->paramsFrame->setHidden(true);
    ui->paletteTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect( ui->paletteTreeWidget, &QWidget::customContextMenuRequested,
             this, &SidepanelEditor::onContextMenu);

    auto table_header = ui->portsTableWidget->horizontalHeader();

    table_header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_header->setSectionResizeMode(1, QHeaderView::Interactive);
    table_header->setSectionResizeMode(2, QHeaderView::Stretch);

    ui->buttonLock->setChecked(false);

    QSettings settings;
    table_header->restoreState( settings.value("SidepanelEditor/header").toByteArray() );
}

SidepanelEditor::~SidepanelEditor()
{
    QSettings settings;
    settings.setValue("SidepanelEditor/header",
                      ui->portsTableWidget->horizontalHeader()->saveState() );

    delete ui;
}

void SidepanelEditor::updateTreeView()
{
    ui->paletteTreeWidget->clear();
    _tree_view_category_items.clear();

    for (const QString& category : {"Action", "Condition",
                                    "Control", "Decorator", "SubTree" } )
    {
      auto item = new QTreeWidgetItem(ui->paletteTreeWidget, {category});
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
      const auto& ID = it.first;
      const NodeModel& model = it.second;

      if( model.registration_ID == "Root")
      {
          continue;
      }
      QString category = QString::fromStdString(toStr(model.type));
      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent, {ID});
      const bool is_builtin = BuiltinNodeModels().count( ID ) > 0;
      const bool is_editable = (!ui->buttonLock->isChecked() && !is_builtin);

      QFont font = item->font(0);
      font.setItalic( is_builtin );
      font.setPointSize(11);
      item->setFont(0, font);
      item->setData(0, Qt::UserRole, ID);

      item->setTextColor(0, is_editable ? Qt::blue : Qt::black);
    }

    ui->paletteTreeWidget->expandAll();
}

void SidepanelEditor::clear()
{

}

void SidepanelEditor::on_paletteTreeWidget_itemSelectionChanged()
{
  auto selected_items = ui->paletteTreeWidget->selectedItems();
  if(selected_items.size() == 0)
  {
    ui->paramsFrame->setHidden(true);
  }
  else {
    auto selected_item = selected_items.front();
    QString item_name = selected_item->text(0);
    ui->paramsFrame->setHidden(false);
    ui->label->setText( item_name + QString(" Parameters"));

    const auto& model = _tree_nodes_model.at(item_name);

    ui->portsTableWidget->setRowCount( model.ports.size() );

    int row = 0;
    for (const auto& port_it: model.ports)
    {
        ui->portsTableWidget->setItem(row,0, new QTableWidgetItem( QString::fromStdString(toStr(port_it.second.direction))));
        ui->portsTableWidget->setItem(row,1, new QTableWidgetItem( port_it.first ));
        ui->portsTableWidget->setItem(row,2, new QTableWidgetItem( port_it.second.description) );
        row++;
    }
    ui->portsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
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


void SidepanelEditor::on_buttonAddNode_clicked()
{
    CustomNodeDialog dialog(_tree_nodes_model, QString(), this);
    if( dialog.exec() == QDialog::Accepted)
    {
        auto new_model = dialog.getTreeNodeModel();
        if( new_model.type == NodeType::SUBTREE )
        {
            emit addSubtree( new_model.registration_ID );
        }
        emit addNewModel( new_model );
    }
    updateTreeView();
}

void SidepanelEditor::onRemoveModel(QString selected_name)
{
    NodeType node_type = _tree_nodes_model.at(selected_name).type;

    _tree_nodes_model.erase( selected_name );
    _model_registry->unregisterModel(selected_name);
    updateTreeView();
    if( node_type == NodeType::SUBTREE)
    {
        emit destroySubtree(selected_name);
    }
}



void SidepanelEditor::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* selected_item = ui->paletteTreeWidget->itemAt(pos);
    if( selected_item == nullptr)
    {
        return;
    }
    QString selected_name = selected_item->text(0);

    if( ui->buttonLock->isChecked() ||
        BuiltinNodeModels().count( selected_name ) != 0 )
    {
        return;
    }

    // Loop through the category items and prevent the right click
    // menu from showing for any of the items
    for (const auto& it : _tree_view_category_items)
    {
        const auto category_item = it.second;
        if( category_item == selected_item ) {
            return;
        }
    }

    QMenu menu(this);

    QAction* edit   = menu.addAction("Edit");
    connect( edit, &QAction::triggered, this, [this, selected_name]()
            {
                CustomNodeDialog dialog(_tree_nodes_model, selected_name, this);
                if( dialog.exec() == QDialog::Accepted)
                {
                    onReplaceModel( selected_name, dialog.getTreeNodeModel() );
                }
            } );

    QAction* remove = menu.addAction("Remove");

    connect( remove, &QAction::triggered, this,[this, selected_name]()
    {
        emit modelRemoveRequested(selected_name);
    } );

    QPoint globalPos = ui->paletteTreeWidget->mapToGlobal(pos);
    menu.exec(globalPos);

    QApplication::processEvents();
}

void SidepanelEditor::onReplaceModel(const QString& old_name,
                                     const NodeModel &new_model)
{
    _tree_nodes_model.erase( old_name );
    _model_registry->unregisterModel( old_name );
    emit addNewModel( new_model );

    if( new_model.type == NodeType::SUBTREE )
    {
       emit renameSubtree(old_name, new_model.registration_ID);
    }

    emit nodeModelEdited(old_name, new_model.registration_ID);
}


void SidepanelEditor::on_buttonUpload_clicked()
{
    QDomDocument doc;

    QDomElement root = doc.createElement( "root" );
    doc.appendChild( root );

    QDomElement root_models = doc.createElement("TreeNodesModel");

    for(const auto& tree_it: _tree_nodes_model)
    {
        const auto& ID    = tree_it.first;
        const auto& model = tree_it.second;

        if( BuiltinNodeModels().count(ID) != 0 )
        {
            continue;
        }

        QDomElement node = doc.createElement( QString::fromStdString(toStr(model.type)) );

        if( !node.isNull() )
        {
            node.setAttribute("ID", ID.toStdString().c_str());
            for(const auto& port_it: model.ports)
            {
                node.appendChild(writePortModel(port_it.first, port_it.second, doc));
            }
        }
        root_models.appendChild(node);
    }
    root.appendChild(root_models);

    //-------------------------------------
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastSaveDirectory",
                                             QDir::currentPath() ).toString();

    auto fileName = QFileDialog::getSaveFileName(this,"Save BehaviorTree to file",
                                                 directory_path,"BehaviorTree files (*.xml)");
    if (fileName.isEmpty()){
        return;
    }
    if (!fileName.endsWith(".xml"))
    {
        fileName += ".xml";
    }


    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << doc.toString(4) << endl;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("SidepanelEditor.lastSaveDirectory", directory_path);

}

void SidepanelEditor::on_buttonDownload_clicked()
{
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastLoadDirectory",
                                             QDir::homePath() ).toString();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load TreenNodeModel from file"),
                                                    directory_path,
                                                    tr("BehaviorTree (*.xml *.skills.json)" ));
    QFileInfo fileInfo(fileName);

    if (!fileInfo.exists(fileName)){
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
    NodeModels imported_models;
    if( fileInfo.suffix() == "xml" )
    {
        QFile file(fileName);
        imported_models = importFromXML( &file );
    }
    else if( fileInfo.completeSuffix() == "skills.json" )
    {
        imported_models = importFromSkills( fileName );
    }

    if( imported_models.empty() )
    {
        return;
    }

    auto models_to_remove = GetModelsToRemove(this, _tree_nodes_model, imported_models );

    for(QString model_name: models_to_remove)
    {
        emit modelRemoveRequested(model_name);
    }

    for(auto& it: imported_models)
    {
        emit addNewModel( it.second );
    }
}

NodeModels SidepanelEditor::importFromXML(QFile* file)
{
    QDomDocument doc;


    if (!file->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel from file",
                             "The XML was not correctly loaded");
        return {};
    }

    QString errorMsg;
    int errorLine;
    if( ! doc.setContent(file, &errorMsg, &errorLine ) )
    {
        auto error = tr("Error parsing XML (line %1): %2").arg(errorLine).arg(errorMsg);
        QMessageBox::warning(this,"Error loading TreeNodeModel form file", error);
        file->close();
        return {};
    }
    file->close();

    NodeModels custom_models;

    QDomElement xml_root = doc.documentElement();
    if ( xml_root.isNull() || xml_root.tagName() != "root")
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel form file",
                             "The XML must have a root node called <root>");
        return custom_models;
    }

    auto manifest_root = xml_root.firstChildElement("TreeNodesModel");

    if ( manifest_root.isNull() )
    {
        QMessageBox::warning(this,"Error loading TreeNodeModel form file",
                             "Expecting <TreeNodesModel> under <root>");
        return custom_models;
    }

    for( QDomElement model_element = manifest_root.firstChildElement();
         !model_element.isNull();
         model_element = model_element.nextSiblingElement() )
    {
        auto model = buildTreeNodeModelFromXML(model_element);
        custom_models.insert( { model.registration_ID, model } );
    }

    return custom_models;
}

NodeModels SidepanelEditor::importFromSkills(const QString &fileName)
{
    NodeModels custom_models;

    QFile loadFile(fileName);

    if (!loadFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"Error loading Skills",
                             tr("Something wrong with %1").arg(fileName) );
        return custom_models;
    }

    // TODO VER_3
//     QJsonDocument loadDoc =  QJsonDocument::fromJson( loadFile.readAll() ) ;

//     QJsonArray root_array = loadDoc.array();

//     for (QJsonValueRef skill_node : root_array)
//     {

//         auto skill = skill_node.toObject()["skill"].toObject();
//         auto name = skill["name"].toString();
//         qDebug() << name;

//         auto attributes = skill["in-attribute"].toObject();
//         auto params_keys = attributes.keys();

//         PortModels ports_models;

//         for (const auto& key: params_keys)
//         {
//             ports_models.insert(  {key, attributes[key].toString()} );
//         }
//         NodeModel model = { NodeType::ACTION, name, ports_mapping };
//         custom_models.insert( {name, model} );
//     }

    return custom_models;
}


void SidepanelEditor::on_buttonLock_toggled(bool locked)
{
    static QIcon icon_locked( QPixmap(":/icons/svg/lock.svg" ) );
    static QIcon icon_unlocked( QPixmap(":/icons/svg/lock_open.svg") );

    ui->buttonLock->setIcon( locked ? icon_locked : icon_unlocked);
    updateTreeView();
}
