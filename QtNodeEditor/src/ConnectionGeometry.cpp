#include "ConnectionGeometry.hpp"

#include <cmath>

#include "StyleCollection.hpp"

using QtNodes::ConnectionGeometry;
using QtNodes::PortType;
using QtNodes::PortLayout;

ConnectionGeometry::
ConnectionGeometry()
  : _in(0, 0)
  , _out(0, 0)
  //, _animationPhase(0)
  , _lineWidth(3.0)
  , _hovered(false)
  , _ports_layout( PortLayout::Horizontal )
{ }

QPointF const&
ConnectionGeometry::
getEndPoint(PortType portType) const
{
  Q_ASSERT(portType != PortType::None);

  return (portType == PortType::Out ?
          _out :
          _in);
}


void
ConnectionGeometry::
setEndPoint(PortType portType, QPointF const& point)
{
  switch (portType)
  {
    case PortType::Out:
      _out = point;
      break;

    case PortType::In:
      _in = point;
      break;

    default:
      break;
  }
}


void
ConnectionGeometry::
moveEndPoint(PortType portType, QPointF const &offset)
{
  switch (portType)
  {
    case PortType::Out:
      _out += offset;
      break;

    case PortType::In:
      _in += offset;
      break;

    default:
      break;
  }
}


QRectF
ConnectionGeometry::
boundingRect() const
{
  auto points = pointsC1C2();

  QRectF basicRect = QRectF(_out, _in).normalized();

  QRectF c1c2Rect = QRectF(points.first, points.second).normalized();

  auto const &connectionStyle =
    StyleCollection::connectionStyle();

  float const diam = connectionStyle.pointDiameter();

  QRectF commonRect = basicRect.united(c1c2Rect);

  QPointF const cornerOffset(diam, diam);

  commonRect.setTopLeft(commonRect.topLeft() - cornerOffset);
  commonRect.setBottomRight(commonRect.bottomRight() + 2 * cornerOffset);

  return commonRect;
}


std::pair<QPointF, QPointF>
ConnectionGeometry::
pointsC1C2() const
{
  double distance = ( _ports_layout == PortLayout::Horizontal ) ?
        (_in.x() - _out.x()) :
        (_in.y() - _out.y());

  double defaultOffset = 50;

  double minimum = qMin(defaultOffset, std::abs(distance));
  double offset = 0;
  double ratio1 = 0.5;

  if (distance <= 0)
  {
    offset = -minimum;
    ratio1 = 1.0;
  }

  QPointF c1,c2;
  if( _ports_layout == PortLayout::Horizontal )
  {
    c1 = QPointF(_out.x() + minimum * ratio1,
                 _out.y() + offset);

    c2 = QPointF(_in.x() - minimum * ratio1,
                 _in.y() + offset);
  }
  else
  {
    c1 = QPointF(_out.x() + offset,
                 _out.y() + minimum * ratio1);

    c2 = QPointF(_in.x() + offset,
                 _in.y() - minimum * ratio1);
  }
  return std::make_pair(c1, c2);
}

void ConnectionGeometry::setPortLayout(QtNodes::PortLayout layout)
{
  _ports_layout = layout;
}
