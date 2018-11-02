#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include "bt_editor_base.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private slots:

    void on_buttonAddFile_clicked();

    void on_buttonRemoveFile_clicked();

    void checkSelections();

    void on_buttonBox_accepted();

    void on_listFiles_itemSelectionChanged();

private:
    Ui::SettingsDialog *ui;

    bool parseFile(const QString& filename);

    bool parseXML(const QString& filename, QString* error_message);

    static std::map<QString, TreeNodeModels> _models_per_file;
};



#endif // SETTINGS_DIALOG_H
