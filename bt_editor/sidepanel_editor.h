#ifndef SIDE_PANEL_EDITOR_H
#define SIDE_PANEL_EDITOR_H

#include <QFrame>
#include <QFile>
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
                             NodeModels& tree_nodes_model,
                             QWidget *parent = nullptr);
    ~SidepanelEditor();

    void updateTreeView();

    void clear();

public slots:
    void onRemoveModel(QString selected_name);

    void onReplaceModel(const QString &old_name, const NodeModel &new_model);


private slots:

    void on_paletteTreeWidget_itemSelectionChanged();

    void on_lineEditFilter_textChanged(const QString &arg1);

    void on_buttonAddNode_clicked();

    void onContextMenu(const QPoint &point);

    void on_buttonUpload_clicked();

    void on_buttonDownload_clicked();

    void on_buttonLock_toggled(bool checked);

signals:

    void addNewModel(const NodeModel &new_model);

    void modelRemoveRequested(QString ID);

    void nodeModelEdited(QString prev_ID, QString new_ID);

    void addSubtree(QString ID);

    void renameSubtree(QString prev_ID, QString new_ID);

    void destroySubtree(QString ID);

private:
    Ui::SidepanelEditor *ui;
    NodeModels &_tree_nodes_model;
    QtNodes::DataModelRegistry* _model_registry;
    std::map<QString, QTreeWidgetItem*> _tree_view_category_items;

    NodeModels importFromXML(QFile *file);

    NodeModels importFromSkills(const QString& filename);

};

#endif // NODE_PALETTE_H
