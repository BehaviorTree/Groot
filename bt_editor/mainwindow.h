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
    explicit MainWindow(GraphicMode initial_mode, QWidget *parent = nullptr);
    ~MainWindow();

    void loadFromXML(const QString &xml_text);

private slots:

    void onAutoArrange();

    void onSceneChanged();

    void onPushUndo();

    void onUndoInvoked();

    void onRedoInvoked();

    void onConnectionUpdate(bool connected);

private slots:
    virtual void closeEvent(QCloseEvent *event) override;

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

    void on_toolButtonReorder_pressed();

    void on_toolButtonCenterView_pressed();

    void onLoadAbsBehaviorTree(AbsBehaviorTree& tree, QString bt_name);

    void on_actionClear_triggered(bool create_new = true);

    void on_toolButtonLayout_clicked();

    void on_actionEditor_mode_triggered();

    void on_actionMonitor_mode_triggered();

    void on_actionReplay_mode_triggered();

    void on_tabWidget_currentChanged(int index);

private:

    void updateCurrentMode();

    void lockEditing(const bool locked);

    bool eventFilter(QObject *obj, QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;

    GraphicContainer* currentTabInfo();

    GraphicContainer *getTabByName(const QString& name);

    GraphicContainer* createTab(const QString &name);

    void refreshNodesLayout(QtNodes::PortLayout new_layout);

    void loadSceneFromYAML(QByteArray state);

signals:
    void updateGraphic();

private:

    GraphicMode _current_mode;

    Ui::MainWindow *ui;

    std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

    std::map<QString, GraphicContainer*> _tab_info;

    std::mutex _mutex;

    std::deque<QByteArray> _undo_stack;
    std::deque<QByteArray> _redo_stack;
    QByteArray _current_state;
    QtNodes::PortLayout _current_layout;

    TreeNodeModels _tree_nodes_model;

    SidepanelEditor* _editor_widget;
    SidepanelReplay* _replay_widget;
#ifdef ZMQ_FOUND
    SidepanelMonitor* _monitor_widget;
#endif

    std::map<QString,AbsBehaviorTree> _abstract_trees;

};



#endif // MAINWINDOW_H
