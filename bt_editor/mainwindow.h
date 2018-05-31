#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <nodes/Node>
#include <QTreeWidgetItem>
#include <QShortcut>
#include <QTimer>
#include <deque>
#include <thread>
#include <mutex>
#include <nodes/DataModelRegistry>

#include "graphic_container.h"
#include "XML_utilities.hpp"
#include "sidepanel_editor.h"
#include "sidepanel_replay.h"

#define ZMQ_FOUND 1

#ifdef ZMQ_FOUND
    #include "sidepanel_monitor.h"
#endif

namespace Ui {
class MainWindow;
}

namespace QtNodes{
class FlowView;
class FlowScene;
class Node;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(GraphicMode initial_mode, QWidget *parent = 0);
    ~MainWindow();

  void loadFromXML(const QString &xml_text);

private slots:

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_actionAuto_arrange_triggered();

    void onSceneChanged();

    virtual void closeEvent(QCloseEvent *event) override;

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

    void onPushUndo();

    void onUndoInvoked();

    void onRedoInvoked();

    void on_toolButtonReorder_pressed();

    void on_toolButtonCenterView_pressed();

    void on_loadBehaviorTree(AbsBehaviorTree& tree);

    void on_actionClear_triggered();

    void updateCurrentMode();

    void on_toolButtonLayout_clicked();

    void on_actionEditor_Mode_triggered();

    void on_actionMonitor_mode_triggered();

    void on_actionReplay_mode_triggered();

signals:
    void updateGraphic();

private:

    GraphicMode _current_mode;

    Ui::MainWindow *ui;

    void lockEditing(const bool locked);

    bool eventFilter(QObject *obj, QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;

    std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

    std::map<QString, GraphicContainer*> _tab_info;

    GraphicContainer* currentTabInfo();

    void createTab(const QString &name);

    std::mutex _mutex;

    std::deque<QByteArray> _undo_stack;
    std::deque<QByteArray> _redo_stack;
    QByteArray _current_state;
    QtNodes::PortLayout _current_layout;

    bool _undo_enabled;

    TreeNodeModels _tree_nodes_model;

    SidepanelEditor* _editor_widget;
    SidepanelReplay* _replay_widget;
#ifdef ZMQ_FOUND
    SidepanelMonitor* _monitor_widget;
#endif
    void refreshNodesLayout(QtNodes::PortLayout new_layout);
    void loadSceneFromYAML(QByteArray state);
};



#endif // MAINWINDOW_H
