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
    explicit SidepanelEditor(QtNodes::DataModelRegistry* registry,
                             TreeNodeModels& tree_nodes_model,
                             QWidget *parent = 0);
    ~SidepanelEditor();

    void updateTreeView();

    void clear();

private slots:

    void on_treeWidget_itemSelectionChanged();

    void on_lineEditFilter_textChanged(const QString &arg1);

    void on_buttonAddNode_clicked();

    void onContextMenu(const QPoint &point);

    void on_buttonUpload_clicked();

    void on_buttonDownload_clicked();

    void on_buttonLock_toggled(bool checked);

private:
    Ui::SidepanelEditor *ui;
    TreeNodeModels &_tree_nodes_model;
    QtNodes::DataModelRegistry* _model_registry;
    std::map<QString, QTreeWidgetItem*> _tree_view_category_items;

    void addNewModel(const QString& name, const TreeNodeModel &new_model);
};

#endif // NODE_PALETTE_H
