#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include "bt_editor_base.h"

namespace Ui {
class SettingsDialog;
}

class ModelsRepositoryDialog : public QDialog
{
    Q_OBJECT

public:
    typedef std::map<QString, TreeNodeModel> ModelsByFile;

    explicit ModelsRepositoryDialog(TreeNodeModel* tree_node_models, QWidget *parent = 0);

    ~ModelsRepositoryDialog();

    static ModelsByFile LoadFromSettings();

    static bool parseFile(const QString& filename, ModelsByFile& models_by_file);

    static bool parseXML(const QString& filename, ModelsByFile& models_by_file, QString* error_message);

private slots:

    void on_buttonAddFile_clicked();

    void on_buttonRemoveFile_clicked();

    void checkSelections();

    void on_buttonBox_accepted();

    void on_listFiles_itemSelectionChanged();

private:
    Ui::SettingsDialog *ui;

    ModelsByFile _models_by_file;

    TreeNodeModel* _tree_node_models;
};



#endif // SETTINGS_DIALOG_H
