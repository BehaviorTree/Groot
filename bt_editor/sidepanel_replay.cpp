#include "sidepanel_replay.h"
#include "ui_sidepanel_replay.h"

#include<QDir>
#include<QFile>
#include<QFileInfo>
#include<QFileDialog>
#include<QSettings>

#include "bt_editor_base.h"
#include "utils.h"
#include "BT_logger_generated.h"

SidepanelReplay::SidepanelReplay(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelReplay)
{
    ui->setupUi(this);
}

SidepanelReplay::~SidepanelReplay()
{
    delete ui;
}

void SidepanelReplay::on_pushButtonLoadLog_pressed()
{
    QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
    QString directory_path  = settings.value("SidepanelReplay.lastLoadDirectory",
                                             QDir::homePath() ).toString();

    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open Flow Scene"), directory_path,
                                                    tr("Flatbuffers log (*.fbl)"));
    if (!QFileInfo::exists(fileName)){
        return;
    }
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)){
        return;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("SidepanelReplay.lastLoadDirectory", directory_path);
    settings.sync();

    QByteArray content = file.readAll();
    //--------------------------------

    const char* buffer = reinterpret_cast<const char*>(content.data());

    size_t bt_header_size = flatbuffers::ReadScalar<uint32_t>(buffer);
    std::vector<char> fb_buffer( bt_header_size );

    memcpy( fb_buffer.data(), &buffer[4], bt_header_size);

    AbsBehaviorTree tree = BuildBehaviorTreeFromFlatbuffers( fb_buffer );

    loadBehaviorTree( tree );

    _transitions.reserve( (content.size() - 4 - bt_header_size) / 12 );

    for (size_t index = 4+bt_header_size; index < content.size(); index += 12)
    {
        Transition transition;
        const double t_sec  = flatbuffers::ReadScalar<uint32_t>( &buffer[index] );
        const double t_usec = flatbuffers::ReadScalar<uint32_t>( &buffer[index+4] );
        transition.timestamp = t_sec + t_usec* 0.000001;
        transition.uid = flatbuffers::ReadScalar<uint16_t>(&buffer[index+8]);
        transition.prev_status = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+10] ));
        transition.status      = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+11] ));

        _transitions.push_back(transition);
    }

    ui->tableWidget->setColumnCount(4);
    const size_t transitions_count = _transitions.size();

    ui->tableWidget->setRowCount( transitions_count );

    auto createStatusItem = [](NodeStatus status) -> QTableWidgetItem*
    {
        switch (status)
        {
        case NodeStatus::SUCCESS:{
            auto item = new QTableWidgetItem("SUCCESS");
            item->setForeground(QColor::fromRgb(77, 255, 77));
            return item;
        }
        case NodeStatus::FAILURE:{
            auto item = new QTableWidgetItem("FAILURE");
            item->setForeground(QColor::fromRgb(255, 0, 0));
            return item;
        }
        case NodeStatus::RUNNING:{
            auto item = new QTableWidgetItem("RUNNING");
            item->setForeground(QColor::fromRgb(235, 120, 66));
            return item;
        }
        case NodeStatus::IDLE:{
            auto item = new QTableWidgetItem("IDLE");
            item->setForeground(QColor::fromRgb(20, 20, 20));
            return item;
        }
        }
        return nullptr;
    };

    if(  transitions_count > 0)
    {
        const double first_timestamp = _transitions.front().timestamp;

        for(size_t row=0; row < transitions_count; row++)
        {
            auto& trans = _transitions[row];
            auto& node = tree.nodes[ trans.uid ];

            QString timestamp;
            timestamp.sprintf("%.3f", trans.timestamp - first_timestamp);

            auto timestamp_item = new QTableWidgetItem( timestamp );
            timestamp.sprintf("absolute time: %.3f", trans.timestamp);
            timestamp_item->setToolTip( timestamp );

            ui->tableWidget->setItem(row,0, timestamp_item );
            ui->tableWidget->setItem(row,1, new QTableWidgetItem( node.instance_name) );
            ui->tableWidget->setItem(row,2, createStatusItem( trans.prev_status) );
            ui->tableWidget->setItem(row,3, createStatusItem( trans.status) );
        }

        ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

        ui->tableWidget->verticalHeader()->minimumSectionSize();


        ui->label->setText( QString("of %1").arg(transitions_count) );
        ui->spinBox->setMinimum(1);
        ui->spinBox->setMaximum(transitions_count);
        ui->spinBox->setEnabled(true);

        ui->timeSlider->setMinimum(1);
        ui->timeSlider->setMaximum(transitions_count);
        ui->timeSlider->setEnabled(true);

        ui->pushButtonPlay->setEnabled(true);
    }
    else{
        ui->label->setText("of 0");
        ui->spinBox->setMinimum(0);
        ui->spinBox->setMaximum(0);
        ui->spinBox->setEnabled(false);

        ui->timeSlider->setMinimum(0);
        ui->timeSlider->setMaximum(0);
        ui->timeSlider->setEnabled(false);

        ui->pushButtonPlay->setEnabled(false);
    }
}

void SidepanelReplay::on_pushButtonPlay_toggled(bool checked)
{

}

void SidepanelReplay::on_spinBox_valueChanged(int value)
{
    if( ui->timeSlider->value() != value)
    {
        ui->timeSlider->setValue( value );
    }
    selectRowsBackground(value);
}

void SidepanelReplay::on_timeSlider_valueChanged(int value)
{
    if( ui->spinBox->value() != value)
    {
        ui->spinBox->setValue( value );
    }
    selectRowsBackground(value);
}

void SidepanelReplay::selectRowsBackground(int last_row)
{
    for (int row = 1; row < last_row; row++)
    {
        ui->tableWidget->item(row, 0)->setBackground( QColor::fromRgb(210, 210, 210) );
        ui->tableWidget->item(row, 1)->setBackground( QColor::fromRgb(210, 210, 210) );
        ui->tableWidget->item(row, 2)->setBackground( QColor::fromRgb(210, 210, 210) );
        ui->tableWidget->item(row, 3)->setBackground( QColor::fromRgb(210, 210, 210) );
    }
    for (int row = last_row; row < ui->tableWidget->rowCount(); row++)
    {
        ui->tableWidget->item(row, 0)->setBackground( QColor::fromRgb(255, 255, 255) );
        ui->tableWidget->item(row, 1)->setBackground( QColor::fromRgb(255, 255, 255) );
        ui->tableWidget->item(row, 2)->setBackground( QColor::fromRgb(255, 255, 255) );
        ui->tableWidget->item(row, 3)->setBackground( QColor::fromRgb(255, 255, 255) );
    }
}
