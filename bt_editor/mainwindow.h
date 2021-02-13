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
#include "models/SubtreeNodeModel.hpp"

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

    enum SubtreeExpandOption{ SUBTREE_EXPAND,
                              SUBTREE_COLLAPSE,
                              SUBTREE_CHANGE,
                              SUBTREE_REFRESH};

public:
    explicit MainWindow(GraphicMode initial_mode, QWidget *parent = nullptr);
    ~MainWindow() override;

    void loadFromXML(const QString &xml_text);

    QString saveToXML() const ;

    GraphicContainer* currentTabInfo();

    GraphicContainer *getTabByName(const QString& name);

    void clearTreeModels();

    const NodeModels &registeredModels() const;

    void resetTreeStyle(AbsBehaviorTree &tree);

    GraphicMode getGraphicMode(void) const;

public slots:

    void onAutoArrange();

    void onSceneChanged();

    void onPushUndo();

    void onUndoInvoked();

    void onRedoInvoked();

    void onConnectionUpdate(bool connected);

    void onRequestSubTreeExpand(GraphicContainer& container,
                                QtNodes::Node& node);

    void onAddToModelRegistry(const NodeModel& model);

    void onDestroySubTree(const QString &ID);

    void onModelRemoveRequested(QString ID);

    virtual void closeEvent(QCloseEvent *event) override;

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

    void on_toolButtonReorder_pressed();

    void on_toolButtonCenterView_pressed();

    void onCreateAbsBehaviorTree(const AbsBehaviorTree &tree,
                                 const QString &bt_name,
                                 bool secondary_tabs = true);

    void onChangeNodesStatus(const QString& bt_name, const std::vector<std::pair<int, NodeStatus>>& node_status);

    void on_toolButtonLayout_clicked();

    void on_actionEditor_mode_triggered();

    void on_actionMonitor_mode_triggered();

    void on_actionReplay_mode_triggered();

    void on_tabWidget_currentChanged(int index);

    void onActionClearTriggered(bool create_new);

    void on_actionClear_triggered();

    void onTreeNodeEdited(QString prev_ID, QString new_ID);

    void onTabCustomContextMenuRequested(const QPoint &pos);

    void onTabRenameRequested(int tab_index, QString new_name = QString());

    void onTabSetMainTree(int tab_index);

signals:
    void updateGraphic();

private slots:
    void on_actionAbout_triggered();

    void on_actionReportIssue_triggered();

public:

    void lockEditing(const bool locked);

private:

    void updateCurrentMode();

    bool eventFilter(QObject *obj, QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;

    GraphicContainer* createTab(const QString &name);

    void refreshNodesLayout(QtNodes::PortLayout new_layout);

    void refreshExpandedSubtrees();

    void streamElementAttributes(QXmlStreamWriter &stream, const QDomElement &element) const;

    QString xmlDocumentToString(const QDomDocument &document) const;

    void recursivelySaveNodeCanonically(QXmlStreamWriter &stream, const QDomNode &parent_node) const;

    struct SavedState
    {
        QString main_tree;
        QString current_tab_name;
        QTransform view_transform;
        QRectF view_area;
        std::map<QString, QByteArray> json_states;
        bool operator ==( const SavedState& other) const;
        bool operator !=( const SavedState& other) const { return !( *this == other); }
    };

    void loadSavedStateFromJson(SavedState state);

    QtNodes::Node *subTreeExpand(GraphicContainer& container,
                       QtNodes::Node &node,
                       SubtreeExpandOption option);

    Ui::MainWindow *ui;

    GraphicMode _current_mode;

    std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

    std::map<QString, GraphicContainer*> _tab_info;

    std::mutex _mutex;

    std::deque<SavedState> _undo_stack;
    std::deque<SavedState> _redo_stack;
    SavedState _current_state;
    QtNodes::PortLayout _current_layout;

    NodeModels _treenode_models;

    QString _main_tree;

    SidepanelEditor* _editor_widget;
    SidepanelReplay* _replay_widget;
#ifdef ZMQ_FOUND
    SidepanelMonitor* _monitor_widget;
#endif
    
    MainWindow::SavedState saveCurrentState();
    void clearUndoStacks();
};






#endif // MAINWINDOW_H
