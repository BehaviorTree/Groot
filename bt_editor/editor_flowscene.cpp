#include "editor_flowscene.h"
#include <QMimeData>
#include <QStandardItemModel>
#include <QVariant>
#include <QGraphicsSceneDragDropEvent>

#include <nodes/Node>

EditorFlowScene::EditorFlowScene(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                                 QObject * parent):
  FlowScene(registry,parent),
  _editor_locked(false)
{

}

void EditorFlowScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
  if(event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")  )
  {
    event->setAccepted(true);
  }
}

void EditorFlowScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
  event->acceptProposedAction();
}

void EditorFlowScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    event->acceptProposedAction();
}

void EditorFlowScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
  if(!_editor_locked && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")  )
  {
    QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    while (!stream.atEnd())
    {
        int row, col;
        QMap<int,  QVariant> roleDataMap;
        stream >> row >> col >> roleDataMap;

        auto it = roleDataMap.find(0);
        if (it != roleDataMap.end() )
        {
          auto type = registry().create(it.value().toString());

          if (type)
          {
            QPointF pos = event->scenePos();
            this->createNode(std::move(type), pos);
          }
        }
    }
  }
  event->acceptProposedAction();
}
