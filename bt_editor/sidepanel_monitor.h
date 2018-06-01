#ifndef SIDEPANEL_MONITOR_H
#define SIDEPANEL_MONITOR_H

#include <QFrame>
#include <zmq.hpp>

#include "bt_editor_base.h"

namespace Ui {
class SidepanelMonitor;
}

class SidepanelMonitor : public QFrame
{
    Q_OBJECT

public:
    explicit SidepanelMonitor(QWidget *parent = 0);
    ~SidepanelMonitor();

    void clear();

public slots:

    void on_Connect();

private slots:

    void on_timer();

signals:
    void loadBehaviorTree( AbsBehaviorTree& tree );

    void connectionUpdate(bool connected);

private:
    Ui::SidepanelMonitor *ui;

    zmq::context_t _zmq_context;
    zmq::socket_t  _zmq_subscriber;

    bool _connected;
    std::string _connection_address_pub;
    std::string _connection_address_req;
    QTimer* _timer;
    int _msg_count;
    AbsBehaviorTree _loaded_tree;

    bool getTreeFromServer();

};

#endif // SIDEPANEL_MONITOR_H
