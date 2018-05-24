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

    _table_model->setColumnCount(4);
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
        _timepoint.clear();

        for(size_t row=0; row < transitions_count; row++)
        {
            auto& trans = _transitions[row];
            auto& node = _loaded_tree.nodes[ trans.uid ];

            QString timestamp;
            timestamp.sprintf("%.3f", trans.timestamp - first_timestamp);

            auto timestamp_item = new QStandardItem( timestamp );
            timestamp.sprintf("absolute time: %.3f", trans.timestamp);
            timestamp_item->setToolTip( timestamp );

            if(  (trans.timestamp - previous_timestamp) >= 0.001 )
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

        qDebug() << _table_model->rowCount();

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

    int prev_row = _prev_row;

    ui->tableView->horizontalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);

    const auto selected_color   = QColor::fromRgb(210, 210, 210);
    const auto unselected_color = QColor::fromRgb(255, 255, 255);

    for (int row = prev_row; row <= current_row; row++)
    {
        _table_model->item(row, 0)->setBackground( selected_color );
        _table_model->item(row, 1)->setBackground( selected_color );
        _table_model->item(row, 2)->setBackground( selected_color );
        _table_model->item(row, 3)->setBackground( selected_color );
    }
    for (int row = current_row+1; row <= prev_row; row++)
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
        _initial_real_time      = std::chrono::high_resolution_clock::now();
        _initial_relative_time  = _transitions[ _prev_row ].timestamp;

        size_t row = _prev_row;

        while( row < _transitions.size()-1 &&
               (_transitions[row+1].timestamp - _initial_relative_time) < 0.01 )
        {
            row++;
        }

        qDebug() << "START: row " << row << " time " << _transitions[row].timestamp - _transitions.front().timestamp;


        double next_time  = _transitions[ row ].timestamp;
        int delay_ms = (next_time - _initial_relative_time) * 1000;

        _play_timer->start( delay_ms );
    }
    else{
        if( !_play_timer->isActive() )
        {
            const QSignalBlocker blocker( _play_timer );
            _play_timer->stop();
        }

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

    auto now = high_resolution_clock::now();
    double relative_time = _initial_relative_time + 0.001*(double) duration_cast<milliseconds>( now - _initial_real_time).count();
    size_t row = _prev_row;

    while( row < LAST_ROW &&
           (_transitions[row+1].timestamp - relative_time) < 0.001 )
    {
        row++;
    }

    row = std::min( row, LAST_ROW);

    qDebug() << " row " << row << " tiemstamp " << relative_time - _transitions.front().timestamp;
    onRowChanged( row );
    updatedSpinAndSlider( row );

    if( row == LAST_ROW)
    {
        ui->pushButtonPlay->setChecked(false);
        return;
    }

    size_t next_row = row+1 ;

    while( next_row < LAST_ROW &&
           (_transitions[next_row].timestamp - relative_time) < 0.01 )
    {
        next_row++;
    }

    double next_time  = _transitions[ next_row ].timestamp;
    int delay_relative = (next_time - _initial_relative_time) * 1000;
    auto deadline =  _initial_real_time + std::chrono::milliseconds((int)(delay_relative));

    int delay_real = duration_cast<milliseconds>( deadline - high_resolution_clock::now() ).count();

    delay_real = std::max( 0, delay_real );

    qDebug() << "delay_relative " << delay_relative << " next_row " << next_row << "  delay_real " << delay_real << "\n";
    _play_timer->start(delay_real);
}
