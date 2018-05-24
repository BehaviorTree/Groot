#ifndef SIDEPANEL_REPLAY_H
#define SIDEPANEL_REPLAY_H

#include <chrono>
#include <QFrame>
#include <QTableWidgetItem>
#include <QStandardItemModel>
#include "bt_editor_base.h"


namespace Ui {
class SidepanelReplay;
}

class SidepanelReplay : public QFrame
{
    Q_OBJECT

public:
    explicit SidepanelReplay(QWidget *parent = 0);
    ~SidepanelReplay();

private slots:
    void on_pushButtonLoadLog_pressed();

    void on_pushButtonPlay_toggled(bool checked);

    void on_spinBox_valueChanged(int arg1);

    void on_timeSlider_valueChanged(int value);

    void on_tableView_clicked(const QModelIndex &index);

    void onTimerUpdate();

    void onPlayUpdate();

signals:
    void loadBehaviorTree( AbsBehaviorTree& tree );

private:

    bool eventFilter(QObject *object, QEvent *event) override;

    void loadFromFlatbuffers(const std::vector<int8_t>& serialized_description);

    void onRowChanged(int value);

    Ui::SidepanelReplay *ui;

    struct Transition{
        uint16_t uid;
        double timestamp;
        NodeStatus prev_status;
        NodeStatus status;
    };
    std::vector<Transition> _transitions;
    std::vector< std::pair<double,int>> _timepoint;

    int _prev_row;

    AbsBehaviorTree _loaded_tree;

    void updatedSpinAndSlider(int row);

    QStandardItemModel* _table_model;

    QTimer *_update_timer;

    QTimer *_play_timer;

    std::chrono::high_resolution_clock::time_point _initial_real_time;
    double _initial_relative_time;
    void updateTableModel();
};

#endif // SIDEPANEL_REPLAY_H
