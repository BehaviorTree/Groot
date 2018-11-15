#include "editor_flowscene.h"
#include <QMimeData>
#include <QStandardItemModel>
#include <QVariant>
#include <QGraphicsSceneDragDropEvent>
#include <QKeyEvent>
#include <QCursor>
#include "models/BehaviorTreeNodeModel.hpp"
#include <QGraphicsView>

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

void EditorFlowScene::keyPressEvent(QKeyEvent *event)
{
    auto selected_items = selectedItems();
    if( selected_items.size() == 1 && event->key() == Qt::Key_C &&
            event->modifiers() == Qt::ControlModifier)
    {
        auto node_item = dynamic_cast<QtNodes::NodeGraphicsObject*>( selected_items.front() );
        if( !node_item ) return;

        QtNodes::Node& selected_node = node_item->node();
        auto node_model = dynamic_cast<BehaviorTreeDataModel*>( selected_node.nodeDataModel() );
        if( !node_model ) return;

        qDebug() << "copy: ";
        _clipboard_node_name = node_model->registrationName();
    }
    else if( event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier &&
            registry().isRegistered( _clipboard_node_name ) )
    {
        qDebug() << "paste: ";
        auto views_ = views();
        QGraphicsView* view = views_.front();
        auto mouse_pos = view->viewport()->mapFromGlobal( QCursor::pos() );
        auto scene_pos = view->mapToScene( mouse_pos );
        auto new_node = registry().create( _clipboard_node_name );
        if (new_node)
        {
            this->createNode(std::move(new_node), scene_pos);
        }
    }
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
                auto new_node = registry().create(it.value().toString());

                if (new_node)
                {
                    QPointF pos = event->scenePos();
                    this->createNode(std::move(new_node), pos);
                }
            }
        }
    }
    event->acceptProposedAction();
}
