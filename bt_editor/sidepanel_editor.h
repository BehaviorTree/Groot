#ifndef SIDE_PANEL_EDITOR_H
#define SIDE_PANEL_EDITOR_H

#include <QFrame>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include "XML_utilities.hpp"

namespace Ui {
class SidepanelEditor;
}

class SidepanelEditor : public QFrame
{
    Q_OBJECT

public:
    explicit SidepanelEditor(TreeNodeModels& tree_nodes_model, QWidget *parent = 0);
    ~SidepanelEditor();

    void updateTreeView();

private slots:

    void on_treeWidget_itemSelectionChanged();

    void on_lineEditFilter_textChanged(const QString &arg1);

    void on_parametersTableWidget_itemSelectionChanged();

    void on_parameterChanged(int row, int col);

private:
    Ui::SidepanelEditor *ui;
    TreeNodeModels &_tree_nodes_model;

    std::map<QString, QTreeWidgetItem*> _tree_view_category_items;

};

#endif // NODE_PALETTE_H
