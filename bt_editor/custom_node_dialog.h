#ifndef CUSTOM_NODE_DIALOG_H
#define CUSTOM_NODE_DIALOG_H

#include "bt_editor_base.h"
#include <QDialog>
#include <QValidator>
#include <QAbstractButton>

namespace Ui {
class CustomNodeDialog;
}

class CustomNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomNodeDialog(const NodeModels& models, QString to_edit = QString(), QWidget *parent = nullptr);

    ~CustomNodeDialog() override;

    NodeModel getTreeNodeModel() const;

private slots:
    void on_pushButtonAdd_pressed();

    void on_pushButtonRemove_pressed();

    void checkValid();

    virtual void closeEvent(QCloseEvent *) override;

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_tableWidget_itemSelectionChanged();

    void on_comboBox_currentIndexChanged(const QString &node_type);

  private:
    Ui::CustomNodeDialog *ui;
    const NodeModels &_models;
    QValidator *_validator;
    bool _editing;
};

#endif // CUSTOM_NODE_DIALOG_H
