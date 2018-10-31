#include "settings_dialog.h"
#include "ui_settings_dialog.h"

#include <QFileDialog>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings settings;
    restoreGeometry(settings.value("SettingsDialog.geometry").toByteArray());

    QStringList files   = settings.value("SettingsDialog.files").toStringList();

    for (int row = 0; row < files.size(); row++ )
    {
        ui->listFiles->addItem( files[row] );
    }

    checkSelections();
}

SettingsDialog::~SettingsDialog()
{
    QSettings settings;
    settings.setValue("SettingsDialog.geometry", saveGeometry());
    delete ui;
}

void SettingsDialog::on_checkBoxLoadStartup_toggled(bool checked)
{

}

void SettingsDialog::on_buttonAddFile_clicked()
{
    QSettings settings;
    QString directory_path  = settings.value("SettingsDialog.addFile",
                                             QDir::currentPath() ).toString();

    QFileDialog dialog(this);
    dialog.setDirectory(directory_path);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(trUtf8("XML or plugins (*.xml *.so)"));

    QStringList file_names;
    if (dialog.exec())
    {
        file_names = dialog.selectedFiles();
    }

    if (file_names.isEmpty()){
        return;
    }

    directory_path = QFileInfo(file_names.front()).absolutePath();
    settings.setValue("SettingsDialog.addFile", directory_path);

    for (const auto& file: file_names)
    {
        if( ui->listFiles->findItems(file, Qt::MatchExactly).empty() )
        {
            QListWidgetItem* item = new QListWidgetItem(file);
            ui->listFiles->addItem(item);
        }
    }
    ui->listFiles->sortItems();
}

void SettingsDialog::on_buttonRemoveFile_clicked()
{
    QList<QListWidgetItem*> items = ui->listFiles->selectedItems();
    if( items.count() == 1 )
    {
        QListWidgetItem* item = items.front();
        ui->listFiles->removeItemWidget(item);
        delete item;
    }
}

void SettingsDialog::checkSelections()
{
   ui->buttonRemoveFile->setEnabled(   ui->listFiles->selectedItems().count() > 0 );
}

void SettingsDialog::on_buttonBox_accepted()
{
    QSettings settings;

    QStringList files;
    for (int row = 0; row < ui->listFiles->count(); row++ )
    {
        files.append( ui->listFiles->item(row)->text() );
    }
    settings.setValue("SettingsDialog.files", files);
}

void SettingsDialog::on_listFiles_itemSelectionChanged()
{
    checkSelections();
}

void SettingsDialog::on_listFolders_itemSelectionChanged()
{
    checkSelections();
}
