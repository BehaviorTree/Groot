#include "model_repository_dialog.h"
#include "ui_settings_dialog.h"
#include "XML_utilities.hpp"

#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QDomDocument>

ModelsRepositoryDialog::ModelsRepositoryDialog(TreeNodeModel* tree_node_models, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    _tree_node_models(tree_node_models)
{
    ui->setupUi(this);

    QSettings settings;

    restoreGeometry(settings.value("ModelsRepositoryDialog.geometry").toByteArray());
    ui->checkBoxAutoload->setChecked( settings.value("ModelsRepositoryDialog.autoload", false ).toBool() );

    QStringList files = settings.value("ModelsRepositoryDialog.files").toStringList();
    for (int row = 0; row < files.size(); row++ )
    {
        if( parseFile( files[row], _models_by_file ) )
        {
            ui->listFiles->addItem( files[row] );
        }
    }

    ui->tableNodesPerFile->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableNodesPerFile->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    checkSelections();
}

ModelsRepositoryDialog::~ModelsRepositoryDialog()
{
    QSettings settings;
    settings.setValue("ModelsRepositoryDialog.geometry", saveGeometry());
    delete ui;
}

ModelsRepositoryDialog::ModelsByFile ModelsRepositoryDialog::LoadFromSettings()
{
    ModelsByFile models_by_file;
    QSettings settings;
    QStringList files = settings.value("ModelsRepositoryDialog.files").toStringList();
    for (int row = 0; row < files.size(); row++ )
    {
        parseFile( files[row], models_by_file );
    }
    return models_by_file;
}


void ModelsRepositoryDialog::on_buttonAddFile_clicked()
{
    QSettings settings;
    QString directory_path  = settings.value("ModelsRepositoryDialog.addFile",
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
    settings.setValue("ModelsRepositoryDialog.addFile", directory_path);

    for (const auto& file: file_names)
    {
        if( parseFile(file, _models_by_file) )
        {
            if( ui->listFiles->findItems(file, Qt::MatchExactly).empty() )
            {
                QListWidgetItem* item = new QListWidgetItem(file);
                ui->listFiles->addItem(item);
            }
        }
    }
    ui->listFiles->sortItems();
}

void ModelsRepositoryDialog::on_buttonRemoveFile_clicked()
{
    QList<QListWidgetItem*> items = ui->listFiles->selectedItems();
    if( items.count() == 1 )
    {
        QListWidgetItem* item = items.front();
        _models_by_file.erase(item->text());
        ui->listFiles->removeItemWidget(item);
        delete item;
    }
}

void ModelsRepositoryDialog::checkSelections()
{
   ui->buttonRemoveFile->setEnabled(   ui->listFiles->selectedItems().count() > 0 );
}

void ModelsRepositoryDialog::on_buttonBox_accepted()
{  
    QSettings settings;

    QStringList files;
    for (int row = 0; row < ui->listFiles->count(); row++ )
    {
        files.append( ui->listFiles->item(row)->text() );
    }
    settings.setValue("ModelsRepositoryDialog.files", files);
    settings.setValue("ModelsRepositoryDialog.autoload", ui->checkBoxAutoload->isChecked() );
}

void ModelsRepositoryDialog::on_listFiles_itemSelectionChanged()
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
        auto models = _models_by_file.at(selected_filename);

        for (const auto& it: models )
        {
            const auto& ID = it.first;
            const auto& model = it.second;
            if( BuiltinNodeModel().count(ID))
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


bool ModelsRepositoryDialog::parseXML(const QString &filename,
                              ModelsByFile& models_by_file,
                              QString* error_message)
{
    TreeNodeModel models;
    QDomDocument doc;
    doc.LoadFile( filename.toStdString().c_str() );

    if (doc.Error())
    {
        (*error_message) = ("The XML was not correctly loaded");
        return false;
    }

    auto strEqual = [](const char* str1, const char* str2) -> bool {
        return strcmp(str1, str2) == 0;
    };

    QDomElement* xml_root = doc.documentElement();
    if (!xml_root || !strEqual(xml_root->Name(), "root"))
    {
        (*error_message) = ("The XML must have a root node called <root>");
        return false;
    }

    auto meta_root = xml_root.firstChildElement("TreeNodesModel");

    if (!meta_root)
    {
        (*error_message) = ("Expecting <TreeNodesModel> under <root>");
        return false;
    }

    for( QDomElement node = meta_root.firstChildElement();
         node != nullptr;
         node = node.nextSiblingElement() )
    {
        models.insert( buildTreeNodeModel(node, true) );
    }
    models_by_file.insert( std::make_pair(filename, models) );

    return true;
}

bool ModelsRepositoryDialog::parseFile(const QString &filename, ModelsByFile &models_by_file)
{
    bool parse_success = false;
    QString error_message;
    QFileInfo file_info(filename);

    if( !file_info.isReadable() )
    {
        error_message = "File not readable";
    }
    else if( file_info.suffix() == "xml" )
    {
        parse_success = parseXML(filename, models_by_file, &error_message);
    }
    else{
        error_message = "Can't be parsed";
    }

    if( !parse_success )
    {
        QMessageBox::warning(nullptr, "Error parsing file", error_message);
    }
    return parse_success;
}
