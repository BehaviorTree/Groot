#include "editor_flowscene.h"
#include <QMimeData>
#include <QStandardItemModel>
#include <QVariant>
#include <QGraphicsSceneDragDropEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QApplication>
#include "models/BehaviorTreeNodeModel.hpp"
#include <QGraphicsView>

#include <nodes/Node>

EditorFlowScene::EditorFlowScene(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                                 QObject * parent):
    FlowScene(registry,parent),
    _editor_locked(false)
{

}

QtNodes::Node &EditorFlowScene::createNodeAtPos(const QString &ID, const QString &instance_name, QPointF scene_pos)
{
    auto node_model = registry().create(ID);
    if( !node_model )
    {
        char buffer[250];
        sprintf(buffer, "No registered model with ID: [%s]",
                ID.toStdString().c_str() );
        throw std::runtime_error( buffer );
    }
    auto bt_model = dynamic_cast<BehaviorTreeDataModel*>( node_model.get() );
    bt_model->setInstanceName( instance_name );
    bt_model->initWidget();
    auto& node_qt = createNode(std::move(node_model));
    setNodePosition(node_qt, scene_pos);

    return node_qt;
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

    for( const auto& it: nodes())
    {
        const auto& node = it.second;
        auto line_edits = node->nodeDataModel()->embeddedWidget()->findChildren<QLineEdit*>();
        for(auto line_edit: line_edits )
        {
            if( line_edit->hasFocus() )
            {
                // Do not swallow the keyPressEvent, you are editing a QLineEdit
                QGraphicsScene::keyPressEvent(event);
                return;
            }
        }
    }

    const QString& registration_ID = _clipboard_node.model.registration_ID;

    auto selected_items = selectedItems();
    if( selected_items.size() == 1 &&
        event->key() == Qt::Key_C &&
        event->modifiers() == Qt::ControlModifier)
    {
        auto node_item = dynamic_cast<QtNodes::NodeGraphicsObject*>( selected_items.front() );
        if( !node_item ) return;

        QtNodes::Node& selected_node = node_item->node();
        auto node_model = dynamic_cast<BehaviorTreeDataModel*>( selected_node.nodeDataModel() );
        if( !node_model ) return;

        _clipboard_node.model = node_model->model();
        _clipboard_node.instance_name  = node_model->instanceName();
    }
    else if( event->key() == Qt::Key_V &&
             event->modifiers() == Qt::ControlModifier &&
             registry().isRegistered( registration_ID  ) )
    {
        auto views_ = views();
        QGraphicsView* view = views_.front();
        auto mouse_pos = view->viewport()->mapFromGlobal( QCursor::pos() );
        auto scene_pos = view->mapToScene( mouse_pos );

        createNodeAtPos( registration_ID,
                        _clipboard_node.instance_name,
                        scene_pos );
    }
    else{
        QGraphicsScene::keyPressEvent(event);
    }
}


void EditorFlowScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if(!_editor_locked && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")  )
    {
        QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QPointF scene_pos = event->scenePos();

        while (!stream.atEnd())
        {
            int row, col;
            QMap<int,  QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

            auto it = roleDataMap.find(0);
            if (it != roleDataMap.end() )
            {
                const auto& ID = it.value().toString();
                createNodeAtPos( ID, ID, scene_pos);
            }
        }
    }
    event->acceptProposedAction();
}
