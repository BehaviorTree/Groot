#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"
#include "custom_node_dialog.h"
#include <QHeaderView>
#include <QPushButton>
#include "models/ActionNodeModel.hpp"
#include "settings_dialog.h"

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
    ui->buttonsFrame->setHidden(true);
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

    for (auto const &it : _tree_nodes_model)
    {
      const QString& ID = it.first;
      const TreeNodeModel& model = it.second;

      const QString& category = toStr(model.node_type);
      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent, {ID});
      AdjustFont(item, 11, false);
      item->setData(0, Qt::UserRole, ID);
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

    auto & model = _tree_nodes_model[item_name];
    int row = 0;

    ui->parametersTableWidget->setRowCount(model.params.size());

    connect( ui->parametersTableWidget,  &QTableWidget::cellChanged,
             this, &SidepanelEditor::on_parameterChanged);

    for (auto& param: model.params)
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

void SidepanelEditor::on_parametersTableWidget_itemSelectionChanged()
{
  auto selected_items = ui->parametersTableWidget->selectedItems();

  ui->pushButtonDelete->setEnabled( selected_items.size() != 0 );
}

void SidepanelEditor::on_parameterChanged(int, int )
{
  ui->pushButtonAdd->setEnabled(true);

}

void SidepanelEditor::on_buttonAddNode_pressed()
{
    CustomNodeDialog dialog(_tree_nodes_model, this);
    if( dialog.exec() == QDialog::Accepted)
    {
        auto res = dialog.getTreeNodeModel();
        const auto& name = res.first;
        const auto& model = res.second;
        if( _tree_nodes_model.count(name) == 0)
        {
            _tree_nodes_model[name] = model;
            updateTreeView();

            if( model.node_type == NodeType::ACTION)
            {
                QtNodes::DataModelRegistry::RegistryItemCreator creator =
                        [name](){
                    return QtNodes::detail::make_unique<ActionNodeModel>(name, ParameterWidgetCreators());
                };
                _model_registry->registerModel("Action", creator, name);
            }
            else if( model.node_type == NodeType::CONDITION)
            {
                QtNodes::DataModelRegistry::RegistryItemCreator creator =
                        [name](){
                    return QtNodes::detail::make_unique<ConditionNodeModel>(name, ParameterWidgetCreators());
                };
                _model_registry->registerModel("Condition", creator, name);
            }
        }
    }
}

void SidepanelEditor::on_buttonSettings_clicked()
{
    SettingsDialog dialog;
    dialog.exec();
}
