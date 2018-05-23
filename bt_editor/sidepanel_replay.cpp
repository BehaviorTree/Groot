#include "sidepanel_replay.h"
#include "ui_sidepanel_replay.h"

#include<QDir>
#include<QFile>
#include<QFileInfo>
#include<QFileDialog>
#include<QSettings>
#include <QKeyEvent>

#include "bt_editor_base.h"
#include "utils.h"
#include "BT_logger_generated.h"

SidepanelReplay::SidepanelReplay(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelReplay),
    _prev_row(0)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    ui->tableWidget->installEventFilter(this);
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

    _loaded_tree = BuildBehaviorTreeFromFlatbuffers( fb_buffer );

    loadBehaviorTree( _loaded_tree );

    _transitions.clear();
    _transitions.reserve( (content.size() - 4 - bt_header_size) / 12 );

    _timepoint.clear();


    for (size_t index = 4+bt_header_size; index < content.size(); index += 12)
    {
        Transition transition;
        const double t_sec  = flatbuffers::ReadScalar<uint32_t>( &buffer[index] );
        const double t_usec = flatbuffers::ReadScalar<uint32_t>( &buffer[index+4] );
        double timestamp = t_sec + t_usec* 0.000001;
        transition.timestamp = timestamp;
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
        double previous_timestamp = 0;

        const double first_timestamp = _transitions.front().timestamp;
        _timepoint.clear();

        for(size_t row=0; row < transitions_count; row++)
        {
            auto& trans = _transitions[row];
            auto& node = _loaded_tree.nodes[ trans.uid ];

            QString timestamp;
            timestamp.sprintf("%.3f", trans.timestamp - first_timestamp);

            auto timestamp_item = new QTableWidgetItem( timestamp );
            timestamp.sprintf("absolute time: %.3f", trans.timestamp);
            timestamp_item->setToolTip( timestamp );

            ui->tableWidget->setItem(row,1, new QTableWidgetItem( node.instance_name) );
            ui->tableWidget->setItem(row,2, createStatusItem( trans.prev_status) );
            ui->tableWidget->setItem(row,3, createStatusItem( trans.status) );

            if(  (trans.timestamp - previous_timestamp) >= 0.001 )
            {
                _timepoint.push_back( {trans.timestamp, row}  );
                previous_timestamp = trans.timestamp;

                auto font = timestamp_item->font();
                font.setBold(true);
                timestamp_item->setFont(font);
            }

            ui->tableWidget->setItem(row,0, timestamp_item );
        }

        ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        ui->tableWidget->verticalHeader()->minimumSectionSize();


        ui->label->setText( QString("of %1").arg(_timepoint.size()) );
        ui->spinBox->setMaximum(_timepoint.size());
        ui->spinBox->setEnabled(true);

        ui->timeSlider->setMaximum(_timepoint.size());
        ui->timeSlider->setEnabled(true);

        ui->pushButtonPlay->setEnabled(true);
    }
    else{
        ui->label->setText("of 0");
        ui->spinBox->setMaximum(0);
        ui->spinBox->setEnabled(false);

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

    int row = _timepoint[value].second;
    auto item = ui->tableWidget->item(row, 0);
    ui->tableWidget->scrollToItem( item, QAbstractItemView::PositionAtCenter  );

    onValueChanged( row );
}

void SidepanelReplay::on_timeSlider_valueChanged(int value)
{
    if( ui->spinBox->value() != value)
    {
        ui->spinBox->setValue( value );
    }

    int row = _timepoint[value].second;
    auto item = ui->tableWidget->item(row, 0);
    ui->tableWidget->scrollToItem( item, QAbstractItemView::PositionAtCenter  );

    onValueChanged( row );
}

void SidepanelReplay::onValueChanged(int current_row)
{
    current_row = std::min( current_row, ui->tableWidget->rowCount() -1 );
    current_row = std::max( current_row, 0 );

    int first = 0;
    if( _prev_row < current_row )
    {
        first = _prev_row;
    }
    {
        ui->tableWidget->horizontalHeader()->setSectionResizeMode (QHeaderView::Fixed);
        ui->tableWidget->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);

        for (int row = first; row <= current_row; row++)
        {
            ui->tableWidget->item(row, 0)->setBackground( QColor::fromRgb(210, 210, 210) );
            ui->tableWidget->item(row, 1)->setBackground( QColor::fromRgb(210, 210, 210) );
            ui->tableWidget->item(row, 2)->setBackground( QColor::fromRgb(210, 210, 210) );
            ui->tableWidget->item(row, 3)->setBackground( QColor::fromRgb(210, 210, 210) );
        }
        for (int row = current_row+1; row < ui->tableWidget->rowCount(); row++)
        {
            ui->tableWidget->item(row, 0)->setBackground( QColor::fromRgb(255, 255, 255) );
            ui->tableWidget->item(row, 1)->setBackground( QColor::fromRgb(255, 255, 255) );
            ui->tableWidget->item(row, 2)->setBackground( QColor::fromRgb(255, 255, 255) );
            ui->tableWidget->item(row, 3)->setBackground( QColor::fromRgb(255, 255, 255) );
        }

        ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    }

    for (auto& it: _loaded_tree.nodes)
    {
        it.second.status = NodeStatus::IDLE;
    }

    for (int index = 0; index <= current_row; index++)
    {
        auto& trans = _transitions[index];
        _loaded_tree.nodes[ trans.uid ].status = trans.status;
    }

    for (auto& it: _loaded_tree.nodes)
    {
        auto& node = it.second.corresponding_node;
        node->nodeDataModel()->setNodeStyle( getStyleFromStatus( it.second.status ) );
        node->nodeGraphicsObject().update();
    }

    _prev_row = current_row;
}


void SidepanelReplay::updatedSpinAndSlider(int row)
{
    auto it = std::upper_bound( _timepoint.begin(), _timepoint.end(), row,
                                []( int val, const std::pair<double,int>& a ) -> bool
    {
        return val < a.second;
    } );

    QSignalBlocker block1( ui->spinBox );
    QSignalBlocker block2( ui->timeSlider );

    int index = (it - _timepoint.begin()) -1;
    index = std::min( index, static_cast<int>(_timepoint.size()) -1 );
    index = std::max( index, 0 );

    ui->spinBox->setValue(index);
    ui->timeSlider->setValue(index);
}

void SidepanelReplay::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
    onValueChanged( item->row() );
    updatedSpinAndSlider(item->row());
}

bool SidepanelReplay::eventFilter(QObject *object, QEvent *event)
{
    if( object == ui->tableWidget)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *key_event = static_cast<QKeyEvent *>(event);

            int next_row = -1;
            if( key_event->key() ==  Qt::Key_Down)
            {
                next_row = _prev_row +1;
            }
            else if( key_event->key() ==  Qt::Key_Up)
            {
                next_row = _prev_row -1;
            }

            if( next_row >= 0)
            {
                onValueChanged( next_row);
                updatedSpinAndSlider( next_row );
                ui->tableWidget->scrollToItem( ui->tableWidget->item(next_row, 0),
                                               QAbstractItemView::EnsureVisible  );
            }

            return true;
        }
    }
    else{
        return false;
    }
}
