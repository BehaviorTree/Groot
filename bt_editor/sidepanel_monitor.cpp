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

void SidepanelMonitor::clear()
{
    if( _connected ) this->on_pushButtonConnect_clicked();
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

            const uint32_t header_size = flatbuffers::ReadScalar<uint32_t>( buffer );
            const uint32_t num_transitions = flatbuffers::ReadScalar<uint32_t>( &buffer[4+header_size] );

            for(size_t index = 4; index < header_size +4; index +=3 )
            {
                uint16_t uid = flatbuffers::ReadScalar<uint16_t>(&buffer[index]);
                NodeStatus status = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+2] ));
                _loaded_tree.nodes[uid].status = status;
            }

            for(size_t t=0; t<num_transitions; t++)
            {
                size_t index = 8 + header_size + 12*t;

                const double t_sec  = flatbuffers::ReadScalar<uint32_t>( &buffer[index] );
                const double t_usec = flatbuffers::ReadScalar<uint32_t>( &buffer[index+4] );
                double timestamp = t_sec + t_usec* 0.000001;
                uint16_t uid = flatbuffers::ReadScalar<uint16_t>(&buffer[index+8]);
                NodeStatus prev_status = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+10] ));
                NodeStatus status      = convert(flatbuffers::ReadScalar<BT_Serialization::Status>(&buffer[index+11] ));

                _loaded_tree.nodes[uid].status = status;
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

bool SidepanelMonitor::getTreeFromServer()
{
    try{
        zmq::message_t request(0);
        zmq::message_t reply;

        zmq::socket_t  zmq_client( _zmq_context, ZMQ_REQ );
        zmq_client.connect( _connection_address_req.c_str() );

        int timeout_ms = 1000;
        zmq_client.setsockopt(ZMQ_RCVTIMEO,&timeout_ms, sizeof(int) );

        zmq_client.send(request);

        bool received = zmq_client.recv(&reply);
        if( ! received )
        {
            return false;
        }

        const char* buffer = reinterpret_cast<const char*>(reply.data());
        _loaded_tree = BuildBehaviorTreeFromFlatbuffers( buffer );
        loadBehaviorTree( _loaded_tree );
    }
    catch( zmq::error_t& err)
    {
        qDebug() << "ZMQ client receive failed: " << err.what();
        return false;
    }
    return true;
}

void SidepanelMonitor::on_pushButtonConnect_clicked()
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
            _connection_address_pub = "tcp://" + address.toStdString() + std::string(":1666");
            _connection_address_req = "tcp://" + address.toStdString() + std::string(":1667");
            try{
                _zmq_subscriber.connect( _connection_address_pub.c_str() );

                int timeout_ms = 1;
                _zmq_subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
                _zmq_subscriber.setsockopt(ZMQ_RCVTIMEO,&timeout_ms, sizeof(int) );

                if( !getTreeFromServer() )
                {
                    failed = true;
                    _connected = false;
                    _zmq_subscriber.disconnect( _connection_address_pub );
                }
            }
            catch(zmq::error_t& err)
            {
                failed = true;
            }
        }
        else {
            failed = true;
        }

        if( !failed )
        {
            _connected = true;
            ui->lineEdit->setDisabled(true);
            _timer->start(20);
            //   ui->pushButtonConnect->setStyleSheet("background-color:  rgba(252, 175, 62, 255)");
        }
        else{
            QMessageBox::warning(this,
                                 tr("ZeroMQ connection"),
                                 tr("Was not able to connect to [%1]\n").arg(_connection_address_pub.c_str()),
                                 QMessageBox::Close);
        }
    }
    else{
        _zmq_subscriber.disconnect( _connection_address_pub );
        _connected = false;
        ui->lineEdit->setDisabled(false);
        _timer->stop();
        // ui->pushButtonConnect->setStyleSheet("background-color:  rgba(0, 0, 0, 0)");
    }
}
