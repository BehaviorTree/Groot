#include "custom_node_dialog.h"
#include "ui_custom_node_dialog.h"

#include <QTreeWidgetItem>
#include <QPushButton>
#include <QRegExpValidator>
#include <QSettings>
#include <QModelIndexList>

CustomNodeDialog::CustomNodeDialog(const NodeModels &models,
                                   QString to_edit,
                                   QWidget *parent):
    QDialog(parent),
    ui(new Ui::CustomNodeDialog),
    _models(models),
    _editing(false)
{
    ui->setupUi(this);
    setWindowTitle("Custom TreeNode Editor");

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    QSettings settings;
    restoreGeometry(settings.value("CustomNodeDialog/geometry").toByteArray());
    ui->tableWidget->horizontalHeader()->restoreState( settings.value("CustomNodeDialog/header").toByteArray() );

    if( to_edit.isEmpty() == false)
    {
        auto model_it = models.find(to_edit);
        if( model_it != models.end())
        {
            _editing =true;
            ui->lineEdit->setText( to_edit );

            const auto& model = model_it->second;
            for( const auto& port_it : model.ports )
            {
                int row = ui->tableWidget->rowCount();
                ui->tableWidget->setRowCount(row+1);

                ui->tableWidget->setItem(row,0, new QTableWidgetItem(port_it.first) );
                QComboBox* combo_direction = new QComboBox;
                combo_direction->addItem("Input");
                combo_direction->addItem("Output");
                combo_direction->addItem("In/Out");
                switch( port_it.second.direction )
                {
                case BT::PortDirection::INPUT : combo_direction->setCurrentIndex(0); break;
                case BT::PortDirection::OUTPUT: combo_direction->setCurrentIndex(1); break;
                case BT::PortDirection::INOUT : combo_direction->setCurrentIndex(2); break;
                }

                ui->tableWidget->setCellWidget(row,1, combo_direction );
                ui->tableWidget->setItem(row,2, new QTableWidgetItem(port_it.second.default_value) );
                ui->tableWidget->setItem(row,3, new QTableWidgetItem(port_it.second.description) );
            }

            if( model.type == NodeType::ACTION )
            {
                ui->comboBox->setCurrentIndex(0);
            }
            else if( model.type == NodeType::CONDITION )
            {
                ui->comboBox->setCurrentIndex(1);
            }
            else if( model.type == NodeType::SUBTREE )
            {
                ui->comboBox->setCurrentIndex(2);
            }
            else if( model.type == NodeType::DECORATOR)
            {
                ui->comboBox->setCurrentIndex(3);
            }
        }
    }

    connect( ui->tableWidget, &QTableWidget::cellChanged,
             this, &CustomNodeDialog::checkValid );

    connect( ui->lineEdit, &QLineEdit::textChanged,
             this, &CustomNodeDialog::checkValid );

    QRegExp rx("\\w+");
    _validator = new QRegExpValidator(rx, this);


    checkValid();
}

CustomNodeDialog::~CustomNodeDialog()
{
    delete ui;
}


NodeModel CustomNodeDialog::getTreeNodeModel() const
{
    QString ID = ui->lineEdit->text();
    NodeType type = NodeType::UNDEFINED;
    PortModels ports;

    switch( ui->comboBox->currentIndex() )
    {
    case 0: type = NodeType::ACTION; break;
    case 1: type = NodeType::CONDITION; break;
    case 2: type = NodeType::SUBTREE; break;
    case 3: type = NodeType::DECORATOR; break;
    }
    for (int row=0; row < ui->tableWidget->rowCount(); row++ )
    {
        const QString key       = ui->tableWidget->item(row,0)->text();
        auto combo = static_cast<QComboBox*>(ui->tableWidget->cellWidget(row,1));
        const QString direction = combo->currentText();

        PortModel port_model;
        port_model.direction = BT::convertFromString<PortDirection>(direction.toStdString());
        port_model.default_value =  ui->tableWidget->item(row,2)->text();
        port_model.description   =  ui->tableWidget->item(row,3)->text();
        ports.insert( {key, port_model} );
    }
    return { type, ID, ports };
}


void CustomNodeDialog::checkValid()
{
    bool valid = false;
    auto name = ui->lineEdit->text();
    int pos;

    if( name.toLower() == "root" )
    {
        ui->labelWarning->setText("The name 'root' is forbidden");
    }
    else if( name.isEmpty() )
    {
        ui->labelWarning->setText("The name cannot be empty");
    }
    else if( _validator->validate(name, pos) != QValidator::Acceptable)
    {
        ui->labelWarning->setText("Invalid name: use only letters, digits and underscores");
    }
    else if( _models.count( name ) > 0 && !_editing )
    {
        ui->labelWarning->setText("Another Node has the same name");
    }
    else {

        bool empty_param_name = false;
        bool invalid_param_name = false;
        std::set<QString> param_names;
        for (int row=0; row < ui->tableWidget->rowCount(); row++ )
        {
            auto param_name = ui->tableWidget->item(row,0)->text();
            if(param_name.isEmpty())
            {
                empty_param_name = true;
            }
            else if( _validator->validate(param_name, pos) != QValidator::Acceptable)
            {
                invalid_param_name = true;
            }
            else{
                param_names.insert(param_name);
            }
        }
        if( empty_param_name )
        {
           ui->labelWarning->setText("Empty NodeParameter key");
        }
        else if( invalid_param_name )
        {
            ui->labelWarning->setText("Invalid key: use only letters, digits and underscores");
        }
        else if( param_names.size() < ui->tableWidget->rowCount() )
        {
           ui->labelWarning->setText("Duplicated NodeParameter key");
        }
        else if( param_names.size() == ui->tableWidget->rowCount() )
        {
            valid = true;
        }
    }
    if(valid)
    {
        ui->labelWarning->setText("OK");
        ui->labelWarning->setStyleSheet("color: rgb(78, 154, 6)");
    }
    else{
        ui->labelWarning->setStyleSheet("color: rgb(204, 0, 0)");
    }
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( valid );
}

void CustomNodeDialog::closeEvent(QCloseEvent *)
{
    QSettings settings;
    settings.setValue("CustomNodeDialog/geometry", saveGeometry());
    settings.setValue("CustomNodeDialog/header", ui->tableWidget->horizontalHeader()->saveState() );
}

void CustomNodeDialog::on_buttonBox_clicked(QAbstractButton *)
{
    QSettings settings;
    settings.setValue("CustomNodeDialog/geometry", saveGeometry());
    settings.setValue("CustomNodeDialog/header", ui->tableWidget->horizontalHeader()->saveState() );
}

void CustomNodeDialog::on_tableWidget_itemSelectionChanged()
{
    QModelIndexList selected_rows = ui->tableWidget->selectionModel()->selectedRows();
    ui->pushButtonRemove->setEnabled( selected_rows.count() != 0);
}

void CustomNodeDialog::on_pushButtonAdd_pressed()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row+1);

    ui->tableWidget->setItem(row,0, new QTableWidgetItem( "key_name" ));
    QComboBox* combo_direction = new QComboBox;

    combo_direction->addItem("Input");
    combo_direction->addItem("Output");
    combo_direction->addItem("In/Out");

    ui->tableWidget->setCellWidget(row, 1, combo_direction);
    ui->tableWidget->setItem(row,2, new QTableWidgetItem());
    ui->tableWidget->setItem(row,3, new QTableWidgetItem());

    checkValid();
}

void CustomNodeDialog::on_pushButtonRemove_pressed()
{
    auto selected = ui->tableWidget->selectionModel()->selectedRows();
    for( const auto& index: selected)
    {
        ui->tableWidget->removeRow( index.row() );
    }
    checkValid();
}
