#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"

#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>

SidepanelEditor::SidepanelEditor(TreeNodeModels &tree_nodes_model, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelEditor),
    _tree_nodes_model(tree_nodes_model)
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

    for (const QString& cat : {"Root", "Action", "Decorator", "Control", "SubTree" } )
    {
      auto item = new QTreeWidgetItem(ui->treeWidget, {cat});
      AdjustFont(item, 11, true);
      item->setFlags( item->flags() ^ Qt::ItemIsDragEnabled );
      item->setFlags( item->flags() ^ Qt::ItemIsSelectable );
      _tree_view_category_items[ cat ] = item;
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

    const std::map<QString,QString> builtin_nodes = {
      { "Sequence", "Control"},
      { "SequenceStar", "Control"},
      { "Fallback", "Control"},
      { "Root", "Root"},
    };

    for (auto const &it : builtin_nodes)
    {
      const QString& ID = it.first;
      const QString& category = it.second;

      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent, {ID});
      AdjustFont(item, 11, false);
      item->setData(0, Qt::UserRole, ID);
    }

    ui->treeWidget->expandAll();
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

    connect( ui->parametersTableWidget,  &QTableWidget::cellChanged, this, &SidepanelEditor::on_parameterChanged);

    for (auto& it: model.params)
    {
      ui->parametersTableWidget->setItem(row,0, new QTableWidgetItem(it.first));
      ui->parametersTableWidget->setItem(row,1, new QTableWidgetItem(toStr(it.second)));
//      auto combo = new QComboBox();
//      combo->addItems( {"Int", "Double", "Text"} );
//      combo->setMaximumWidth(80);
//      ui->parametersTableWidget->setCellWidget(row, 1, combo);
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

void SidepanelEditor::on_parameterChanged(int row, int col)
{
  ui->pushButtonAdd->setEnabled(true);

}
