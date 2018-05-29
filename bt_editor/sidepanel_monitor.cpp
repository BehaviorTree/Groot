#include "sidepanel_monitor.h"
#include "ui_sidepanel_monitor.h"
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
#include <QDebug>

#include "utils.h"
#include "BT_logger_generated.h"

SidepanelMonitor::SidepanelMonitor(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelMonitor),
    _zmq_context(1),
    _zmq_subscriber(_zmq_context, ZMQ_SUB),
    _connected(false),
    _msg_count(0)
{
    ui->setupUi(this);
    _timer = new QTimer(this);

    connect( _timer, &QTimer::timeout, this, &SidepanelMonitor::on_timer );
}

SidepanelMonitor::~SidepanelMonitor()
{
    delete ui;
}

void SidepanelMonitor::on_pushButtonConnect_pressed()
{
    if( !_connected)
    {
        QString address = ui->lineEdit->text();
        if( address.isEmpty() )
        {
            address = ui->lineEdit->placeholderText();
            ui->lineEdit->setText(address);
        }
        bool failed = false;
        if( !address.isEmpty() )
        {
            _connection_address = "tcp://" + address.toStdString();
            try{
                _zmq_subscriber.connect( _connection_address.c_str() );
                _connected = true;
                int timeout_ms = 1;
                _zmq_subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
                _zmq_subscriber.setsockopt(ZMQ_RCVTIMEO,&timeout_ms, sizeof(int) );

                _timer->start(25);
            }
            catch(zmq::error_t& err)
            {
                failed = true;
                _connected = false;
            }
        }
        else {
            failed = true;
        }

        if( !failed )
        {
            ui->lineEdit->setDisabled(true);
            ui->pushButtonConnect->setText("Disconnect");
            //   ui->pushButtonConnect->setStyleSheet("background-color:  rgba(252, 175, 62, 255)");
        }
        else{
            QMessageBox::warning(this,
                                 tr("ZeroMQ connection"),
                                 tr("Was not able to connect to [%1]\n").arg(_connection_address.c_str()),
                                 QMessageBox::Close);
        }
    }
    else{
        _zmq_subscriber.disconnect( _connection_address );
        _connected = false;
        ui->lineEdit->setDisabled(false);

        _timer->stop();
        // ui->pushButtonConnect->setStyleSheet("background-color:  rgba(0, 0, 0, 0)");
    }
}

void SidepanelMonitor::on_timer()
{
    if( !_connected ) return;

    zmq::message_t msg;
    try{
        bool received = _zmq_subscriber.recv(&msg);
        if( received )
        {
            _msg_count++;
            ui->labelCount->setText( QString("Messages received: %1").arg(_msg_count) );

            const char* buffer = reinterpret_cast<const char*>(msg.data());

            size_t bt_header_size = flatbuffers::ReadScalar<uint32_t>(buffer);
            std::vector<char> fb_buffer( bt_header_size );
            memcpy( fb_buffer.data(), &buffer[4], bt_header_size);

            auto tree = BuildBehaviorTreeFromFlatbuffers( fb_buffer );

            bool trees_are_similar = true;
            if( tree.root_node_uid == _loaded_tree.root_node_uid &&
                (_loaded_tree.nodes.size() - tree.nodes.size()) <=1 )
            {
                for (auto& it: tree.nodes)
                {
                    int16_t uid = it.first;
                    if( _loaded_tree.nodes.find(uid) == _loaded_tree.nodes.end() )
                    {
                        trees_are_similar = false;
                        break;
                    }
                }
            }
            else{
                trees_are_similar = false;
            }
            if( !trees_are_similar )
            {
                _loaded_tree = std::move(tree);
                loadBehaviorTree( _loaded_tree );
                 qDebug() << "loadBehaviorTree";
            }
            else{
                // copy status only
                for (auto it: tree.nodes)
                {
                    const int16_t uid = it.first;
                    _loaded_tree.nodes[uid].status = it.second.status;
                }

                uint32_t num_transitions = flatbuffers::ReadScalar<uint32_t>( &buffer[4+bt_header_size] );

                for(size_t t=0; t<num_transitions; t++)
                {
                    size_t index = 8 + bt_header_size + 12*t;

                    const double t_sec  = flatbuffers::ReadScalar<uint32_t>( &buffer[index] );
                    const double t_usec = flatbuffers::ReadScalar<uint32_t>( &buffer[index+4] );
                    double timestamp = t_sec + t_usec* 0.000001;
                    uint16_t uid = flatbuffers::ReadScalar<uint16_t>(&buffer[index+8]);
                    NodeStatus prev_status = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+10] ));
                    NodeStatus status      = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+11] ));

                    _loaded_tree.nodes[uid].status = status;
                }
            }
            // update the graphic part
            for (auto& it: _loaded_tree.nodes)
            {
                auto& node = it.second.corresponding_node;
                if( node )
                {
                    node->nodeDataModel()->setNodeStyle( getStyleFromStatus( it.second.status ) );
                    node->nodeGraphicsObject().update();
                }
            }
        }
    }
    catch( zmq::error_t& err)
    {
        qDebug() << "ZMQ receive failed ";
    }
}
