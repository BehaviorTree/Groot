#ifndef EDITOR_FLOWSCENE_H
#define EDITOR_FLOWSCENE_H

#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>

class EditorFlowScene : public QtNodes::FlowScene
{
public:
    EditorFlowScene(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                    QObject * parent = Q_NULLPTR);

    bool isLocked() const { return _editor_locked; }
    void lock(bool lock_editor) { _editor_locked = lock_editor; }

private:

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;

    bool _editor_locked;
};

#endif // EDITOR_FLOWSCENE_H
