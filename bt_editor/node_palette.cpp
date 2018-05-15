#include "node_palette.h"
#include "ui_node_palette.h"

NodePalette::NodePalette(TreeNodeModels &tree_nodes_model, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NodePalette),
    _tree_nodes_model(tree_nodes_model)
{
    ui->setupUi(this);   
}

NodePalette::~NodePalette()
{
    delete ui;
}

void NodePalette::updateTreeView()
{
    auto AdjustFont = [](QTreeWidgetItem* item, int size, bool is_bold)
    {
      QFont font = item->font(0);
      font.setBold(is_bold);
      font.setPointSize(size);
      item->setFont(0, font);
    };

    auto skipText = QStringLiteral("skip me");

    ui->treeWidget->clear();
    _tree_view_category_items.clear();

    for (const QString& cat : {"Root", "Action", "Decorator", "Control", "SubTree" } )
    {
      auto item = new QTreeWidgetItem(ui->treeWidget);
      item->setText(0, cat);
      AdjustFont(item, 11, true);
      item->setData(0, Qt::UserRole, skipText);
      item->setFlags( item->flags() ^ Qt::ItemIsDragEnabled );
      _tree_view_category_items[ cat ] = item;
    }

    for (auto const &it : _tree_nodes_model)
    {
      const QString& ID = it.first;
      const TreeNodeModel& model = it.second;

      const QString& category = toStr(model.node_type);
      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent);
      item->setText(0, ID);
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
      auto item = new QTreeWidgetItem(parent);
      item->setText(0, ID);
      AdjustFont(item, 11, false);
      item->setData(0, Qt::UserRole, ID);
    }

    ui->treeWidget->expandAll();

    //Setup filtering
    connect(ui->lineEditFilter, &QLineEdit::textChanged, [&](const QString &text)
    {
      for (auto& topLvlItem : _tree_view_category_items)
      {
        for (int i = 0; i < topLvlItem.second->childCount(); ++i)
        {
          auto child = topLvlItem.second->child(i);
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
