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

private slots:
    void on_pushButtonConnect_pressed();

    void on_timer();

signals:
    void loadBehaviorTree( AbsBehaviorTree& tree );

private:
    Ui::SidepanelMonitor *ui;

    zmq::context_t _zmq_context;
    zmq::socket_t  _zmq_subscriber;
    bool _connected;
    std::string _connection_address;
    QTimer* _timer;
    int _msg_count;
    AbsBehaviorTree _loaded_tree;

};

#endif // SIDEPANEL_MONITOR_H
