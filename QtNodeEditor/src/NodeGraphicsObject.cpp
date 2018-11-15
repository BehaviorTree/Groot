#include "NodeGraphicsObject.hpp"

#include <iostream>
#include <cstdlib>

#include <QtWidgets/QtWidgets>
#include <QtWidgets/QGraphicsEffect>

#include "ConnectionGraphicsObject.hpp"
#include "ConnectionState.hpp"

#include "FlowScene.hpp"
#include "NodePainter.hpp"

#include "Node.hpp"
#include "NodeDataModel.hpp"
#include "NodeConnectionInteraction.hpp"

#include "StyleCollection.hpp"

using QtNodes::NodeGraphicsObject;
using QtNodes::Node;
using QtNodes::FlowScene;

NodeGraphicsObject::
NodeGraphicsObject(FlowScene &scene,
                   Node& node)
  : _scene(scene)
  , _node(node)
  , _locked(false)
  , _double_clicked(false)
  , _proxyWidget(nullptr)
{
  _scene.addItem(this);

  setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

  setCacheMode( QGraphicsItem::DeviceCoordinateCache );

  auto const &nodeStyle = node.nodeDataModel()->nodeStyle();

  {
    auto effect = new QGraphicsDropShadowEffect;
    effect->setOffset(2, 2);
    effect->setBlurRadius(5);
    effect->setColor(nodeStyle.ShadowColor);

    setGraphicsEffect(effect);
  }

  setOpacity(nodeStyle.Opacity);

  setAcceptHoverEvents(true);

  setZValue(0);

  updateEmbeddedQWidget();

}


NodeGraphicsObject::
~NodeGraphicsObject()
{
  _scene.removeItem(this);
}


Node&
NodeGraphicsObject::
node()
{
  return _node;
}


Node const&
NodeGraphicsObject::
node() const
{
  return _node;
}

void
NodeGraphicsObject::
updateEmbeddedQWidget()
{
  NodeGeometry & geom = _node.nodeGeometry();

  if( _proxyWidget )
  {
    _scene.removeItem(_proxyWidget);
    _proxyWidget->deleteLater();
  }

  if (auto w = _node.nodeDataModel()->embeddedWidget())
  {
    _proxyWidget = new QGraphicsProxyWidget(this);

    _proxyWidget->setWidget(w);

    _proxyWidget->setPreferredWidth(5);

    geom.recalculateSize();

    _proxyWidget->setPos(geom.widgetPosition());

    update();

    _proxyWidget->setOpacity(1.0);
    _proxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
  }
}


QRectF
NodeGraphicsObject::
boundingRect() const
{
  return _node.nodeGeometry().boundingRect();
}


void
NodeGraphicsObject::
setGeometryChanged()
{
  prepareGeometryChange();
}


void
NodeGraphicsObject::
moveConnections() const
{
  NodeState const & nodeState = _node.nodeState();

  for(PortType portType: {PortType::In, PortType::Out})
  {
    auto const & connectionEntries =
        nodeState.getEntries(portType);

    for (auto const & connections : connectionEntries)
    {
      for (auto & con : connections)
        con.second->connectionGraphicsObject().move();
    }
  };
}

void NodeGraphicsObject::lock(bool locked)
{
  _locked = locked;
  setFlag(QGraphicsItem::ItemIsMovable, !locked);
  setFlag(QGraphicsItem::ItemIsFocusable, !locked);
  setFlag(QGraphicsItem::ItemIsSelectable, !locked);
}


void
NodeGraphicsObject::
paint(QPainter * painter,
      QStyleOptionGraphicsItem const* option,
      QWidget* )
{
  painter->setClipRect(option->exposedRect);

  NodePainter::paint(painter, _node, _scene);
}


QVariant
NodeGraphicsObject::
itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionChange && scene())
  {
    moveConnections();
  }

  return QGraphicsItem::itemChange(change, value);
}


void
NodeGraphicsObject::
mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  if(_locked) return;

  // deselect all other items after this one is selected
  if (!isSelected() && event->modifiers() != Qt::ControlModifier)
  {
    _scene.clearSelection();
  }

  for(PortType portToCheck: {PortType::In, PortType::Out})
  {
    NodeGeometry & nodeGeometry = _node.nodeGeometry();

    // TODO do not pass sceneTransform
    int portIndex = nodeGeometry.checkHitScenePoint(portToCheck,
                                                    event->scenePos(),
                                                    sceneTransform());

    if (portIndex != INVALID)
    {
      NodeState const & nodeState = _node.nodeState();

      std::unordered_map<QUuid, Connection*> connections =
          nodeState.connections(portToCheck, portIndex);

      // start dragging existing connection
      if (!connections.empty() && portToCheck == PortType::In)
      {
        auto con = connections.begin()->second;

        NodeConnectionInteraction interaction(_node, *con, _scene);

        interaction.disconnect(portToCheck);
      }
      else // initialize new Connection
      {
        if (portToCheck == PortType::Out)
        {
          const auto outPolicy = _node.nodeDataModel()->portOutConnectionPolicy(portIndex);
          if (!connections.empty() &&
              outPolicy == NodeDataModel::ConnectionPolicy::One)
          {
            _scene.deleteConnection( *connections.begin()->second );
          }

          // todo add to FlowScene
          auto connection = _scene.createConnection(portToCheck,
                                                    _node,
                                                    portIndex);

          _node.nodeState().setConnection(portToCheck,
                                          portIndex,
                                          *connection);

          connection->connectionGraphicsObject().grabMouse();
        }
      }
    }
  }

  auto pos     = event->pos();
  auto & geom  = _node.nodeGeometry();
  auto & state = _node.nodeState();

  if (_node.nodeDataModel()->resizable() &&
      geom.resizeRect().contains(QPoint(pos.x(),
                                        pos.y())))
  {
    state.setResizing(true);
  }
  _press_pos = event->screenPos();
}


void
NodeGraphicsObject::
mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  auto & geom  = _node.nodeGeometry();
  auto & state = _node.nodeState();

  if (state.resizing())
  {
    auto diff = event->pos() - event->lastPos();

    if (auto w = _node.nodeDataModel()->embeddedWidget())
    {
      prepareGeometryChange();

      auto oldSize = w->size();

      oldSize += QSize(diff.x(), diff.y());

      w->setFixedSize(oldSize);

      _proxyWidget->setMinimumSize(oldSize);
      _proxyWidget->setMaximumSize(oldSize);
      _proxyWidget->setPos(geom.widgetPosition());

      geom.recalculateSize();
      update();

      moveConnections();

      event->accept();
    }
  }
  else
  {
    QGraphicsObject::mouseMoveEvent(event);

    if (event->lastPos() != event->pos())
      moveConnections();

    event->ignore();
  }

  QRectF r = scene()->sceneRect();

  r = r.united(mapToScene(boundingRect()).boundingRect());

  scene()->setSceneRect(r);
}


void
NodeGraphicsObject::
mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if( _double_clicked )
  {
    event->setModifiers(event->modifiers() | Qt::ControlModifier);
    _double_clicked = false;
    setSelected(true);
    return;
  }
  else{
    if (!isSelected() && event->modifiers() != Qt::ControlModifier)
    {
      _scene.clearSelection();
    }
  }


  auto & state = _node.nodeState();

  state.setResizing(false);

  QGraphicsObject::mouseReleaseEvent(event);

  // position connections precisely after fast node move
  moveConnections();

  if( (event->screenPos() - _press_pos).manhattanLength() > 20 )
  {
    _scene.nodeMoved(_node, pos());
  }

}


void
NodeGraphicsObject::
hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
  // bring all the colliding nodes to background
  QList<QGraphicsItem *> overlapItems = collidingItems();

  for (QGraphicsItem *item : overlapItems)
  {
    if (item->zValue() > 0.0)
    {
      item->setZValue(0.0);
    }
  }
  // bring this node forward
  setZValue(1.0);

  _node.nodeGeometry().setHovered(true);
  update();
  _scene.nodeHovered(node(), event->screenPos());
  event->accept();
}


void
NodeGraphicsObject::
hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
  _node.nodeGeometry().setHovered(false);
  update();
  _scene.nodeHoverLeft(node());
  event->accept();
}


void
NodeGraphicsObject::
hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
  auto pos    = event->pos();
  auto & geom = _node.nodeGeometry();


  if (_node.nodeDataModel()->resizable() &&
      geom.resizeRect().contains(QPoint(pos.x(), pos.y())))
  {
    setCursor(QCursor(Qt::SizeFDiagCursor));
  }
  else
  {
    setCursor(QCursor());
  }

  event->accept();
}


void
NodeGraphicsObject::
mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsItem::mouseDoubleClickEvent(event);
  _double_clicked = true;
  emit _scene.nodeDoubleClicked(node());
}

void
NodeGraphicsObject::
contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  _scene.nodeContextMenu(node(), mapToScene(event->pos()));
}
