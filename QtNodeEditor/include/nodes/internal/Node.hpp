#pragma once


#include <QtCore/QObject>
#include <QtCore/QUuid>

#include <QtCore/QJsonObject>

#include "PortType.hpp"

#include "Export.hpp"
#include "NodeState.hpp"
#include "NodeGeometry.hpp"
#include "NodeData.hpp"
#include "NodeGraphicsObject.hpp"
#include "ConnectionGraphicsObject.hpp"
#include "Serializable.hpp"
#include <memory.hpp>

namespace QtNodes
{

class Connection;
class ConnectionState;
class NodeGraphicsObject;
class NodeDataModel;

class NODE_EDITOR_PUBLIC Node
  : public QObject
  , public Serializable
{
  Q_OBJECT

public:
  /// NodeDataModel ownership is transferred to the Node.
  Node(detail::unique_qptr<NodeDataModel>&);

  virtual
  ~Node();

public:

  QJsonObject
  save() const override;

  void
  restore(QJsonObject const &json) override;

public:

  QUuid
  id() const;

  void reactToPossibleConnection(PortType,
                                 NodeDataType const &,
                                 QPointF const & scenePoint);

  void
  resetReactionToConnection();

public:

  NodeGraphicsObject const &
  nodeGraphicsObject() const;

  NodeGraphicsObject &
  nodeGraphicsObject();

  void
  setGraphicsObject(detail::unique_qptr<NodeGraphicsObject>&);

  NodeGeometry&
  nodeGeometry();

  NodeGeometry const&
  nodeGeometry() const;

  NodeState const &
  nodeState() const;

  NodeState &
  nodeState();

  NodeDataModel*
  nodeDataModel() const;

public slots: // data propagation

  /// Propagates incoming data to the underlying model.
  void
  propagateData(std::shared_ptr<NodeData> nodeData,
                PortIndex inPortIndex) const;

  /// Fetches data from model's OUT #index port
  /// and propagates it to the connection
  void
  onDataUpdated(PortIndex index);

  /// update the graphic part if the size of the embeddedwidget changes
  void
  onNodeSizeUpdated();

private:

  // addressing
  QUuid _uid;

  // data
  detail::unique_qptr<NodeDataModel> _nodeDataModel;
  NodeState _nodeState;

  // painting
  NodeGeometry _nodeGeometry;
  detail::unique_qptr<NodeGraphicsObject> _nodeGraphicsObject;
};
}
