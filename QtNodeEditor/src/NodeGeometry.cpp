#include "NodeGeometry.hpp"

#include <iostream>
#include <cmath>
#include <QDebug>

#include "PortType.hpp"
#include "NodeState.hpp"
#include "NodeDataModel.hpp"
#include "Node.hpp"
#include "NodeGraphicsObject.hpp"

#include "StyleCollection.hpp"

using QtNodes::NodeGeometry;
using QtNodes::NodeDataModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::PortLayout;
using QtNodes::Node;

NodeGeometry::
NodeGeometry(std::unique_ptr<NodeDataModel> const &dataModel)
  : _width(50)
  , _height(50)
  , _inputPortWidth(4)
  , _outputPortWidth(4)
  , _entryHeight(1)
  , _spacing(1)
  , _hovered(false)
  , _draggingPos(-1000, -1000)
  , _dataModel(dataModel)
  , _fontMetrics(QFont())
  , _boldFontMetrics(QFont())
  , _ports_layout(PortLayout::Vertical  )
{
  QFont f;
  f.setPointSize(12);
  _boldFontMetrics = QFontMetrics(f);
}

unsigned int
NodeGeometry::nSources() const
{
  return _dataModel->nPorts(PortType::Out);
}

unsigned int
NodeGeometry::nSinks() const
{
  return _dataModel->nPorts(PortType::In);
}

QRectF
NodeGeometry::
entryBoundingRect() const
{
  double const addon = 0.0;

  return QRectF(0 - addon,
                0 - addon,
                _entryWidth + 2 * addon,
                _entryHeight + 2 * addon);
}


QRectF
NodeGeometry::
boundingRect() const
{
  auto const &nodeStyle = StyleCollection::nodeStyle();

  double addon = 2 * nodeStyle.ConnectionPointDiameter;

  return QRectF(0 - addon,
                0 - addon,
                _width + 2 * addon,
                _height + 2 * addon);
}


void
NodeGeometry::
recalculateSize() const
{
  _entryHeight = _fontMetrics.height();

  {
    unsigned int maxNumOfEntries = std::max(nSinks(), nSources());
    unsigned int step = _entryHeight + _spacing;
    _height = step * maxNumOfEntries;
  }

  if (auto w = _dataModel->embeddedWidget())
  {
    _height = std::max(_height, w->height());
  }

  _inputPortWidth  = portWidth(PortType::In);
  _outputPortWidth = portWidth(PortType::Out);

  _width = _inputPortWidth +
           _outputPortWidth +
           2 * _spacing;

  if (auto w = _dataModel->embeddedWidget())
  {
    _width += w->width();
  }

  if (_dataModel->validationState() != NodeValidationState::Valid)
  {
    _width   = std::max(_width, (int)validationWidth());
    _height += validationHeight() + _spacing;
  }
}


void
NodeGeometry::
recalculateSize(QFont const & font) const
{
  QFontMetrics fontMetrics(font);
  QFont boldFont = font;
  boldFont.setPointSize(12);

  QFontMetrics boldFontMetrics(boldFont);

  if (_boldFontMetrics != boldFontMetrics)
  {
    _fontMetrics     = fontMetrics;
    _boldFontMetrics = boldFontMetrics;
    recalculateSize();
  }
}


QPointF
NodeGeometry::
portScenePosition(PortIndex index,
                  PortType portType,
                  QTransform const & t) const
{
  auto const connectionDiameter = StyleCollection::nodeStyle().ConnectionPointDiameter;

  if( _ports_layout == PortLayout::Horizontal)
  {
    unsigned int step = _entryHeight + _spacing;
    double totalHeight = step * index;
    totalHeight += step / 2.0;

    double x = (portType == PortType::Out) ? _width + connectionDiameter :
                                             - connectionDiameter;
    return t.map( QPointF(x, totalHeight) );
  }
  else
  {
    unsigned int nPorts = _dataModel->nPorts(portType);
    unsigned int step = _width / (nPorts + 1);
    double x = step * (index+1);

    double y = (portType == PortType::Out) ? _height + connectionDiameter :
                                             - connectionDiameter;
    return t.map( QPointF( x, y) );
  }
}


PortIndex
NodeGeometry::
checkHitScenePoint(PortType portType,
                   QPointF const scenePoint,
                   QTransform const & sceneTransform) const
{
  auto const &nodeStyle = StyleCollection::nodeStyle();

  PortIndex result = INVALID;

  if (portType == PortType::None)
    return result;

  double const tolerance = 2 * nodeStyle.ConnectionPointDiameter;

  unsigned int const nItems = _dataModel->nPorts(portType);

  for (unsigned int i = 0; i < nItems; ++i)
  {
    auto pp = portScenePosition(i, portType, sceneTransform);

    QPointF p = pp - scenePoint;
    auto    distance = std::sqrt(QPointF::dotProduct(p, p));

    if (distance < tolerance)
    {
      result = PortIndex(i);
      break;
    }
  }

  return result;
}


QRect
NodeGeometry::
resizeRect() const
{
  unsigned int rectSize = 4;

  return QRect(_width - rectSize,
               _height - rectSize,
               rectSize,
               rectSize);
}


QPointF
NodeGeometry::
widgetPosition() const
{
  if (auto w = _dataModel->embeddedWidget())
  {
    if (_dataModel->validationState() != NodeValidationState::Valid)
    {
      return QPointF(_spacing + portWidth(PortType::In),
                     ( _height - validationHeight() - _spacing - w->height()) / 2.0);
    }

    return QPointF(_spacing + portWidth(PortType::In),
                   ( _height - w->height()) / 2.0);
  }

  return QPointF();
}

unsigned int
NodeGeometry::
validationHeight() const
{
  QString msg = _dataModel->validationMessage();
  return _boldFontMetrics.boundingRect(msg).height();
}


unsigned int
NodeGeometry::
validationWidth() const
{
  QString msg = _dataModel->validationMessage();
  return _boldFontMetrics.boundingRect(msg).width();
}


QPointF
NodeGeometry::
calculateNodePositionBetweenNodePorts(PortIndex targetPortIndex, PortType targetPort, Node* targetNode, 
                                      PortIndex sourcePortIndex, PortType sourcePort, Node* sourceNode, 
                                      Node& newNode)
{
  //Calculating the nodes position in the scene. It'll be positioned half way between the two ports that it "connects". 
  //The first line calculates the halfway point between the ports (node position + port position on the node for both nodes averaged).
  //The second line offsets this coordinate with the size of the new node, so that the new nodes center falls on the originally
  //calculated coordinate, instead of it's upper left corner.
  auto converterNodePos = (sourceNode->nodeGraphicsObject().pos() + sourceNode->nodeGeometry().portScenePosition(sourcePortIndex, sourcePort) +
    targetNode->nodeGraphicsObject().pos() + targetNode->nodeGeometry().portScenePosition(targetPortIndex, targetPort)) / 2.0f;
  converterNodePos.setX(converterNodePos.x() - newNode.nodeGeometry().width() / 2.0f);
  converterNodePos.setY(converterNodePos.y() - newNode.nodeGeometry().height() / 2.0f);
  return converterNodePos;
}

void NodeGeometry::setPortLayout(QtNodes::PortLayout layout)
{
    _ports_layout = layout;
}


unsigned int
NodeGeometry::
portWidth(PortType portType) const
{
  unsigned width = 0;

  for (auto i = 0ul; i < _dataModel->nPorts(portType); ++i)
  {
    QString name = _dataModel->dataType(portType, i).name;
    width = std::max(unsigned(_fontMetrics.width(name)), width);
  }

  return width;
}
