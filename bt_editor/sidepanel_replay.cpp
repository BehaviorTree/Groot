#include "sidepanel_replay.h"
#include "ui_sidepanel_replay.h"

#include <QDir>
#include <QFile>
#include <QtConcurrent/QtConcurrentRun>
#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>
#include <QKeyEvent>
#include <QStandardItem>
#include <QModelIndex>
#include <QTimer>

#include "bt_editor_base.h"
#include "utils.h"
#include "BT_logger_generated.h"

SidepanelReplay::SidepanelReplay(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelReplay),
    _prev_row(0)
{
    ui->setupUi(this);

    _table_model = new QStandardItemModel(0,4, this);

    _table_model->setHeaderData(0,Qt::Horizontal, "Time");
    _table_model->setHeaderData(1,Qt::Horizontal, "Node Name");
    _table_model->setHeaderData(2,Qt::Horizontal, "Previous");
    _table_model->setHeaderData(3,Qt::Horizontal, "Status");

    ui->tableView->setModel(_table_model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    _update_timer = new QTimer(this);
    _update_timer->setSingleShot(true);
    connect( _update_timer, &QTimer::timeout, this, &SidepanelReplay::onTimerUpdate );


    _play_timer = new QTimer(this);
    _play_timer->setSingleShot(true);
    connect( _play_timer, &QTimer::timeout, this, &SidepanelReplay::onPlayUpdate );

    ui->tableView->installEventFilter(this);
}

SidepanelReplay::~SidepanelReplay()
{
    delete ui;
}

void SidepanelReplay::updateTableModel()
{
    _table_model->setColumnCount(4);
    _table_model->setRowCount(0);

    const size_t transitions_count = _transitions.size();

    auto createStatusItem = [](NodeStatus status) -> QStandardItem*
    {
        QStandardItem* item = nullptr;
        switch (status)
        {
        case NodeStatus::SUCCESS:{
            item = new QStandardItem("SUCCESS");
            item->setForeground(QColor::fromRgb(77, 255, 77));
        } break;
        case NodeStatus::FAILURE:{
            item = new QStandardItem("FAILURE");
            item->setForeground(QColor::fromRgb(255, 0, 0));
        } break;
        case NodeStatus::RUNNING:{
            item = new QStandardItem("RUNNING");
            item->setForeground(QColor::fromRgb(235, 120, 66));
        } break;
        case NodeStatus::IDLE:{
            item = new QStandardItem("IDLE");
            item->setForeground(QColor::fromRgb(20, 20, 20));
        } break;
        }
        auto font = item->font();
        font.setBold(true);
        item->setFont(font);
        return item;
    };

    if(  transitions_count > 0)
    {
        double previous_timestamp = 0;
        const double first_timestamp = _transitions.front().timestamp;

        for(size_t row=0; row < transitions_count; row++)
        {
            auto& trans = _transitions[row];
            auto& node = _loaded_tree.nodes[ trans.uid ];

            QString timestamp;
            timestamp.sprintf("%.3f", trans.timestamp - first_timestamp);

            auto timestamp_item = new QStandardItem( timestamp );
            timestamp.sprintf("absolute time: %.3f", trans.timestamp);
            timestamp_item->setToolTip( timestamp );

            if(  (trans.timestamp - previous_timestamp) >= 0.001 || row == transitions_count-1)
            {
                _timepoint.push_back( {trans.timestamp, row}  );
                previous_timestamp = trans.timestamp;

                auto font = timestamp_item->font();
                font.setBold(true);
                timestamp_item->setFont(font);
            }

            QList<QStandardItem *> rowData;
            rowData << timestamp_item;
            rowData << new QStandardItem( node.instance_name );
            rowData << createStatusItem( trans.prev_status );
            rowData << createStatusItem( trans.status );
            _table_model->appendRow(rowData);
        }

        emit _table_model->dataChanged( _table_model->index(0,0), _table_model->index( transitions_count-1, 3) );

        ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        ui->tableView->verticalHeader()->minimumSize();
    }

    ui->label->setText( QString("of %1").arg( _timepoint.size() ) );

    ui->spinBox->setValue(0);
    ui->spinBox->setMaximum( _timepoint.size()-1 );
    ui->spinBox->setEnabled( !_timepoint.empty() );
    ui->timeSlider->setValue( 0 );
    ui->timeSlider->setMaximum( _timepoint.size()-1 );
    ui->timeSlider->setEnabled( !_timepoint.empty() );
    ui->pushButtonPlay->setEnabled( !_timepoint.empty() );
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

    _timepoint.clear();
    _prev_row = 0;
    updateTableModel();
}


void SidepanelReplay::on_spinBox_valueChanged(int value)
{
    if( ui->timeSlider->value() != value)
    {
        ui->timeSlider->setValue( value );
    }

    int row = _timepoint[value].second;

    ui->tableView->scrollTo( _table_model->index(row,0), QAbstractItemView::PositionAtCenter  );

    onRowChanged( row );
}

void SidepanelReplay::on_timeSlider_valueChanged(int value)
{
    if( ui->spinBox->value() != value)
    {
        ui->spinBox->setValue( value );
    }

    int row = _timepoint[value].second;
    ui->tableView->scrollTo( _table_model->index(row,0), QAbstractItemView::PositionAtCenter);

    onRowChanged( row );
}

void SidepanelReplay::onRowChanged(int current_row)
{
    current_row = std::min( current_row, _table_model->rowCount() -1 );
    current_row = std::max( current_row, 0 );

    if( _prev_row == current_row)
    {
        return;
    }

    ui->tableView->horizontalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);

    const auto selected_color   = QColor::fromRgb(210, 210, 210);
    const auto unselected_color = QColor::fromRgb(255, 255, 255);

    for (int row = _prev_row; row <= current_row; row++)
    {
        _table_model->item(row, 0)->setBackground( selected_color );
        _table_model->item(row, 1)->setBackground( selected_color );
        _table_model->item(row, 2)->setBackground( selected_color );
        _table_model->item(row, 3)->setBackground( selected_color );
    }
    for (int row = current_row+1; row <= _prev_row; row++)
    {
        _table_model->item(row, 0)->setBackground( unselected_color );
        _table_model->item(row, 1)->setBackground( unselected_color );
        _table_model->item(row, 2)->setBackground( unselected_color );
        _table_model->item(row, 3)->setBackground( unselected_color );
    }

    if( !_update_timer->isActive() )
    {
        _update_timer->stop();
    }
    _update_timer->start(100);

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


bool SidepanelReplay::eventFilter(QObject *object, QEvent *event)
{
    if( object == ui->tableView)
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

            if( next_row >= 0 && next_row < _table_model->rowCount() )
            {
                onRowChanged( next_row);
                updatedSpinAndSlider( next_row );
                ui->tableView->scrollTo( _table_model->index(next_row,0),
                                         QAbstractItemView::EnsureVisible);
            }
            return true;
        }
    }
    return false;
}

void SidepanelReplay::on_tableView_clicked(const QModelIndex &index)
{
    // disable during play
    if( !ui->pushButtonPlay->isChecked())
    {
        onRowChanged( index.row() );
        updatedSpinAndSlider( index.row() );
    }
}

void SidepanelReplay::onTimerUpdate()
{
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
}

void SidepanelReplay::on_pushButtonPlay_toggled(bool checked)
{
    ui->timeSlider->setEnabled( !checked );
    ui->spinBox->setEnabled( !checked );

    if(checked)
    {
        _next_row =_prev_row;
        onPlayUpdate();
    }
    else{
        ui->tableView->scrollTo( _table_model->index( _prev_row,0),
                                 QAbstractItemView::PositionAtCenter);
    }
}

void SidepanelReplay::onPlayUpdate()
{
    if( !ui->pushButtonPlay->isChecked())
    {
        return;
    }

    using namespace std::chrono;
    const size_t LAST_ROW = _transitions.size()-1;

    while( _next_row < LAST_ROW -1 &&
           (_transitions[_next_row+1].timestamp - _transitions[_next_row].timestamp) < 0.001 )
    {
        _next_row++;
    }

    onRowChanged( _next_row );
    updatedSpinAndSlider( _next_row );
    ui->tableView->scrollTo( _table_model->index(_next_row,0), QAbstractItemView::EnsureVisible  );

    if( _next_row == LAST_ROW)
    {
        ui->pushButtonPlay->setChecked(false);
        return;
    }

    double prev_timestamp = _transitions[_next_row].timestamp;

    //qDebug() << " row " << _next_row << " timwstamp " << _transitions[_next_row].timestamp - _transitions.front().timestamp;

    _next_row++;
    while( _next_row < LAST_ROW &&
           (_transitions[_next_row].timestamp - prev_timestamp) < 0.01 )
    {
        _next_row++;
    }

    int delay_relative = (_transitions[ _next_row ].timestamp - prev_timestamp) * 1000;

    //qDebug() << "delay_relative " << delay_relative << " next_row " << _next_row << "\n";

    _play_timer->start(delay_relative);
}
