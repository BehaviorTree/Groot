#ifndef EDITOR_FLOWSCENE_H
#define EDITOR_FLOWSCENE_H

#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>

class EditorFlowScene : public QtNodes::FlowScene
{
public:
    EditorFlowScene(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                    QObject * parent = Q_NULLPTR);

private:

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
};

#endif // EDITOR_FLOWSCENE_H
