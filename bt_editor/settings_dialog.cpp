#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "XML_utilities.hpp"

#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>

std::map<QString, TreeNodeModels> SettingsDialog::_models_per_file;

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

    ui->tableNodesPerFile->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableNodesPerFile->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    checkSelections();
}

SettingsDialog::~SettingsDialog()
{
    QSettings settings;
    settings.setValue("SettingsDialog.geometry", saveGeometry());
    delete ui;
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
        QFileInfo file_info(file);

        if( file_info.suffix() == "xml" )
        {
            if(  parseFile(file) )
            {
                if( ui->listFiles->findItems(file, Qt::MatchExactly).empty() )
                {
                    QListWidgetItem* item = new QListWidgetItem(file);
                    ui->listFiles->addItem(item);
                }
            }
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
        _models_per_file.erase(item->text());
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

    auto table = ui->tableNodesPerFile;
    while(table->rowCount() > 0)
    {
        ui->tableNodesPerFile->removeRow(0);
    }

    auto selected_items =  ui->listFiles->selectedItems();
    if( selected_items.count() == 1)
    {
        auto selected_filename = selected_items.front()->text();

        if( _models_per_file.count( selected_filename) == 0)
        {
            bool ret = parseFile(selected_filename);
            if( !ret )
            {
                return;
            }
        }
        auto models = _models_per_file.at(selected_filename);

        for (const auto& it: models )
        {
            const auto& ID = it.first;
            const auto& model = it.second;
            if( BuiltinNodeModels().count(ID))
            {
                continue;
            }
            int row = table->rowCount();
            table->setRowCount( row+1 );
            table->setItem(row, 0, new QTableWidgetItem( it.first ));
            table->setItem(row, 1, new QTableWidgetItem( toStr(model.node_type) ));
        }
        table->sortByColumn(0, Qt::AscendingOrder);
    }
}


bool SettingsDialog::parseXML(const QString &filename, QString* error_message)
{
    using namespace tinyxml2;
    TreeNodeModels models;
    XMLDocument doc;
    doc.LoadFile( filename.toStdString().c_str() );

    if (doc.Error())
    {
        (*error_message) = ("The XML was not correctly loaded");
        return false;
    }

    auto strEqual = [](const char* str1, const char* str2) -> bool {
        return strcmp(str1, str2) == 0;
    };

    const tinyxml2::XMLElement* xml_root = doc.RootElement();
    if (!xml_root || !strEqual(xml_root->Name(), "root"))
    {
        (*error_message) = ("The XML must have a root node called <root>");
        return false;
    }

    auto meta_root = xml_root->FirstChildElement("TreeNodesModel");

    if (!meta_root)
    {
        (*error_message) = ("Expecting <TreeNodesModel> under <root>");
        return false;
    }

    for( const XMLElement* node = meta_root->FirstChildElement();
         node != nullptr;
         node = node->NextSiblingElement() )
    {
        models.insert( buildTreeNodeModel(node, true) );
    }
    _models_per_file.insert( std::make_pair(filename, models) );

    return true;
}

bool SettingsDialog::parseFile(const QString &filename)
{
    bool parse_success = false;
    QString error_message;
    QFileInfo file_info(filename);

    if( file_info.suffix() == "xml" )
    {
        parse_success = parseXML(filename, &error_message);
    }
    else{
        error_message = "Can't be parsed";
    }

    if( !parse_success )
    {
        QMessageBox::warning(this, "Error parsing file", error_message);
    }
    return parse_success;
}
