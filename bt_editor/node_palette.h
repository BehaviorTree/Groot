#ifndef NODE_PALETTE_H
#define NODE_PALETTE_H

#include <QFrame>
#include <QTreeWidgetItem>
#include "XmlParsers.hpp"

namespace Ui {
class NodePalette;
}

class NodePalette : public QFrame
{
    Q_OBJECT

public:
    explicit NodePalette(TreeNodeModels& tree_nodes_model, QWidget *parent = 0);
    ~NodePalette();

    void updateTreeView();

private:
    Ui::NodePalette *ui;
    TreeNodeModels &_tree_nodes_model;

    std::map<QString, QTreeWidgetItem*> _tree_view_category_items;

};

#endif // NODE_PALETTE_H
