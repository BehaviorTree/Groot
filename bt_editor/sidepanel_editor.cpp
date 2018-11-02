#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"
#include "custom_node_dialog.h"
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include "models/ActionNodeModel.hpp"
#include "model_repository_dialog.h"

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
}

SidepanelEditor::~SidepanelEditor()
{
    delete ui;
}

void SidepanelEditor::updateTreeView()
{
    auto AdjustFont = [](QTreeWidgetItem* item, int size, bool is_bold)
    {
      QFont font = item->font(0);
      font.setBold(is_bold);
      font.setPointSize(size);
      item->setFont(0, font);
    };

    ui->treeWidget->clear();
    _tree_view_category_items.clear();

    for (const QString& category : {"Root", "Action", "Condition", "Control", "Decorator", "SubTree" } )
    {
      auto item = new QTreeWidgetItem(ui->treeWidget, {category});
      AdjustFont(item, 11, true);
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
      AdjustFont(item, 11, false);
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

        ParameterWidgetCreators widget_creators;

        for( const auto& param: model.params)
        {
            widget_creators.push_back( buildWidgetCreator(param) );
        }

        if( model.node_type == NodeType::ACTION)
        {
            QtNodes::DataModelRegistry::RegistryItemCreator creator =
                    [name, widget_creators](){
                return QtNodes::detail::make_unique<ActionNodeModel>(name, widget_creators);
            };
            _model_registry->registerModel("Action", creator, name);
        }
        else if( model.node_type == NodeType::CONDITION)
        {
            QtNodes::DataModelRegistry::RegistryItemCreator creator =
                    [name, widget_creators](){
                return QtNodes::detail::make_unique<ConditionNodeModel>(name, widget_creators);
            };
            _model_registry->registerModel("Condition", creator, name);
        }
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


void SidepanelEditor::on_toolButtonLoad_clicked()
{
    ModelsRepositoryDialog dialog(&_tree_nodes_model, this);
    dialog.exec();
}
