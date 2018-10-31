#ifndef CUSTOM_NODE_DIALOG_H
#define CUSTOM_NODE_DIALOG_H

#include "bt_editor_base.h"
#include <QDialog>
#include <QValidator>

namespace Ui {
class CustomNodeDialog;
}

class CustomNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomNodeDialog(const TreeNodeModels& models, QString to_edit = QString(), QWidget *parent = 0);
    ~CustomNodeDialog();

    std::pair<QString, TreeNodeModel> getTreeNodeModel() const;

private slots:
    void on_toolButtonAdd_pressed();

    void on_toolButtonRemove_pressed();

    void checkValid();

private:
    Ui::CustomNodeDialog *ui;
    const TreeNodeModels &_models;
    QValidator *_validator;
    bool _editing;
};

#endif // CUSTOM_NODE_DIALOG_H
