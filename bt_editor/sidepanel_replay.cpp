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
#include <QMessageBox>

#include "bt_editor_base.h"
#include "mainwindow.h"
#include "utils.h"


SidepanelReplay::SidepanelReplay(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelReplay),
    _prev_row(-1),
    _parent(parent)
{
    ui->setupUi(this);

    _table_model = new QStandardItemModel(0,4, this);

    _table_model->setHeaderData(0,Qt::Horizontal, "Time");
    _table_model->setHeaderData(1,Qt::Horizontal, "Node Name");
    _table_model->setHeaderData(2,Qt::Horizontal, "Previous");
    _table_model->setHeaderData(3,Qt::Horizontal, "Status");

    ui->tableView->setModel(_table_model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    _layout_update_timer = new QTimer(this);
    _layout_update_timer->setSingleShot(true);
    connect( _layout_update_timer, &QTimer::timeout, this, &SidepanelReplay::onTimerUpdate );


    _play_timer = new QTimer(this);
    _play_timer->setSingleShot(true);
    connect( _play_timer, &QTimer::timeout, this, &SidepanelReplay::onPlayUpdate );

    ui->tableView->installEventFilter(this);
}

SidepanelReplay::~SidepanelReplay()
{
    delete ui;
}

void SidepanelReplay::clear()
{
    _table_model->setColumnCount(4);
    _table_model->setRowCount(0);
}

void SidepanelReplay::updateTableModel(const AbsBehaviorTree& locaded_tree)
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
            item->setBackground(QColor::fromRgb(22, 255, 22));
        } break;
        case NodeStatus::FAILURE:{
            item = new QStandardItem("FAILURE");
            item->setBackground(QColor::fromRgb(255, 22, 22));
        } break;
        case NodeStatus::RUNNING:{
            item = new QStandardItem("RUNNING");
            item->setBackground(QColor::fromRgb(250, 160, 20));
        } break;
        case NodeStatus::IDLE:{
            item = new QStandardItem("IDLE");
            item->setBackground(QColor::fromRgb(222, 222, 222));
        } break;
        }
        item->setForeground(QColor::fromRgb(0, 0, 0));
        return item;
    };

    if(  transitions_count > 0)
    {
        double previous_timestamp = 0;
        const double first_timestamp = _transitions.front().timestamp;

        for(size_t row=0; row < transitions_count; row++)
        {
            auto& trans = _transitions[row];
            auto node  = locaded_tree.node( trans.index );

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
            rowData << new QStandardItem( node->instance_name );
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
    ui->spinBox->setMaximum( std::max(0 , (int)_timepoint.size()-1) );
    ui->spinBox->setEnabled( !_timepoint.empty() );
    ui->timeSlider->setValue( 0 );
    ui->timeSlider->setMaximum( std::max(0 , (int)_timepoint.size()-1) );
    ui->timeSlider->setEnabled( !_timepoint.empty() );
    ui->pushButtonPlay->setEnabled( !_timepoint.empty() );
}

void SidepanelReplay::on_LoadLog()
{
    QSettings settings;
    QString directory_path  = settings.value("SidepanelReplay.lastLoadDirectory",
                                             QDir::homePath() ).toString();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Flow Scene"), directory_path,
                                                    tr("Flatbuffers log (*.fbl)"));

    if (fileName.isEmpty() || !QFileInfo::exists(fileName))
    {
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
    loadLog( content );
}

void SidepanelReplay::loadLog(const QByteArray &content)
{
    const char* buffer = reinterpret_cast<const char*>(content.data());

    // how many bytes did we read off the disk (uoffset_t aka uint32_t)
    const auto read_bytes = content.size();

    // we need at least 4 bytes to read the bt_header_size
    if( read_bytes < 4 ) {
        QMessageBox::warning( this, "Log file is empty",
                             "Failed to load this file.\n"
                             "This Log file is empty");
        return;
    }
    
    // read the length of the header section from the file
    const size_t bt_header_size = flatbuffers::ReadScalar<uint32_t>(buffer);

    // if the length of the header goes past the end of the file, it is invalid
    if( (bt_header_size == 0) || (bt_header_size > read_bytes) ) {
        QMessageBox::warning( this, "Log file is corrupt",
                             "Failed to load this file.\n"
                             "This Log file corrupted or truncated");
        return;
    }

    flatbuffers::Verifier verifier( reinterpret_cast<const uint8_t*>(buffer+4),
                                   size_t(content.size() -4));

    bool valid_tree = Serialization::VerifyBehaviorTreeBuffer(verifier);
    if( ! valid_tree )
    {
        QMessageBox::warning( this, "Flatbuffer verification failed",
                             "Failed to load this file.\n"
                             "Its format is not compatible with the current one");
        return;
    }


    auto fb_behavior_tree = Serialization::GetBehaviorTree( &buffer[4] );


    auto res_pair = BuildTreeFromFlatbuffers( fb_behavior_tree );

    _loaded_tree  = res_pair.first;
    const auto& uid_to_index = res_pair.second;

    for (const auto& tree_node: _loaded_tree.nodes() )
    {
        const QString& ID = tree_node.model.registration_ID;
        if( BuiltinNodeModels().count( ID ) == 0)
        {
            emit addNewModel( tree_node.model );
        }
    }

    emit loadBehaviorTree( _loaded_tree, "BehaviorTree" );

    _transitions.clear();
    _transitions.reserve( (content.size() - 4 - bt_header_size) / 12 );

    int idle_counter = _loaded_tree.nodes().size();
    const int total_nodes = _loaded_tree.nodes().size();
    int nearest_restart_transition_index = 0;

    for (size_t offset = 4+bt_header_size; offset < content.size(); offset += 12)
    {
        Transition transition;
        const double t_sec  = flatbuffers::ReadScalar<uint32_t>( &buffer[offset] );
        const double t_usec = flatbuffers::ReadScalar<uint32_t>( &buffer[offset+4] );
        double timestamp = t_sec + t_usec* 0.000001;
        transition.timestamp = timestamp;
        const uint16_t uid = flatbuffers::ReadScalar<uint16_t>(&buffer[offset+8]);
        transition.index = uid_to_index.at(uid);
        transition.prev_status = convert(flatbuffers::ReadScalar<Serialization::NodeStatus>(&buffer[offset+10] ));
        transition.status      = convert(flatbuffers::ReadScalar<Serialization::NodeStatus>(&buffer[offset+11] ));
        transition.is_tree_restart = false;

        if(transition.index == 1 &&
                (transition.status == NodeStatus::RUNNING || transition.status == NodeStatus::IDLE) &&
                idle_counter >= total_nodes - 1){
            transition.is_tree_restart = true;
            nearest_restart_transition_index = _transitions.size();
        }

        if(transition.prev_status != NodeStatus::IDLE && transition.status == NodeStatus::IDLE)
            idle_counter++;
        else if(transition.prev_status == NodeStatus::IDLE && transition.status != NodeStatus::IDLE)
            idle_counter--;

        transition.nearest_restart_transition_index = nearest_restart_transition_index;

        _transitions.push_back(transition);
    }

    _timepoint.clear();
    _prev_row = -1;
    updateTableModel(_loaded_tree);


    // We need to lock the nodes after they are loaded
    auto main_win = dynamic_cast<MainWindow*>( _parent );
    main_win->lockEditing(true);
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
        // nothing to do
        return;
    }

    // disable section resize, otherwise it will be SUPER slow
    // We will refresh this in the callback of _layout_update_timer -> onTimerUpdate
    ui->tableView->horizontalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);

    const auto selected_color   = QColor::fromRgb(210, 210, 210);
    const auto unselected_color = QColor::fromRgb(255, 255, 255);

    for (int row = std::max(0,_prev_row); row <= current_row; row++)
    {
        _table_model->item(row, 0)->setBackground( selected_color );
        _table_model->item(row, 1)->setBackground( selected_color );
    }
    for (int row = current_row+1; row <= _prev_row; row++)
    {
        _table_model->item(row, 0)->setBackground( unselected_color );
        _table_model->item(row, 1)->setBackground( unselected_color );
    }

    // cancel the refresh of the layout refresh
    if( !_layout_update_timer->isActive() )
    {
        _layout_update_timer->stop();
    }
    _layout_update_timer->start(100);

    const QString bt_name("BehaviorTree");

    std::vector<std::pair<int, NodeStatus>>  node_status;
    for(size_t index = 0; index < _loaded_tree.nodes().size(); index++ )
    {
        node_status.push_back( { index, NodeStatus::IDLE} );
    }

    for (int t = _transitions[current_row].nearest_restart_transition_index; t <= current_row; t++)
    {
        auto& trans = _transitions[t];
        node_status.push_back( { trans.index, trans.status} );
    }

    emit changeNodeStyle( bt_name, node_status );

    _prev_row = current_row;
}

void SidepanelReplay::updatedSpinAndSlider(int row)
{
    auto it = std::upper_bound( _timepoint.begin(), _timepoint.end(), row,
                                []( int val, const std::pair<double,int>& a ) -> bool
    {
        return val < a.second;
    } );

    QSignalBlocker block_spin( ui->spinBox );
    QSignalBlocker block_Slider( ui->timeSlider );

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
        _next_row = std::max(0, _prev_row);
        onPlayUpdate();
    }
    else{
        ui->tableView->scrollTo( _table_model->index( _prev_row,0),
                                 QAbstractItemView::PositionAtCenter);
    }
}

void SidepanelReplay::onPlayUpdate()
{
    if( !ui->pushButtonPlay->isChecked() || _transitions.empty() )
    {
        return;
    }  

    using namespace std::chrono;
    const int LAST_ROW = _transitions.size()-1;

    _next_row = std::max(0, _next_row);
    _next_row = std::min(LAST_ROW, _next_row);

    const double TIME_DIFFERENCE_THRESHOLD = 0.01;

    // move forward as long as timestamp difference is small.
    while( _next_row < LAST_ROW -1 &&
           (_transitions[_next_row+1].timestamp - _transitions[_next_row].timestamp) < TIME_DIFFERENCE_THRESHOLD )
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

    const double prev_time = _transitions[_next_row].timestamp;
    const double next_time = _transitions[_next_row+1].timestamp;
    int delay_relative = (next_time - prev_time) * 1000;

    _next_row++;
    //qDebug() << "delay_relative " << delay_relative << " next_row " << _next_row << "\n";

    _play_timer->start(delay_relative);
}

void SidepanelReplay::on_lineEditFilter_textChanged(const QString &filter_text)
{
    for (int row=0; row < _table_model->rowCount(); row++ )
    {
        bool show = _table_model->item(row,1)->text().contains(filter_text, Qt::CaseInsensitive);

        if( show ){
            ui->tableView->showRow(row);
        }
        else{
            ui->tableView->hideRow(row);
        }
    }
}
