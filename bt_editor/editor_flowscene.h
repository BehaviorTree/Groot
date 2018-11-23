#ifndef EDITOR_FLOWSCENE_H
#define EDITOR_FLOWSCENE_H

#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>
#include "bt_editor/bt_editor_base.h"

class EditorFlowScene : public QtNodes::FlowScene
{
public:
    EditorFlowScene(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                    QObject * parent = Q_NULLPTR);

    bool isLocked() const { return _editor_locked; }
    void lock(bool lock_editor) { _editor_locked = lock_editor; }

    QtNodes::Node& createNodeAtPos(const QString& ID, const QString& instance_name, QPointF scene_pos);

private:

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;

    void keyPressEvent( QKeyEvent * event ) override;

    bool _editor_locked;
    AbstractTreeNode _clipboard_node;
};

#endif // EDITOR_FLOWSCENE_H
