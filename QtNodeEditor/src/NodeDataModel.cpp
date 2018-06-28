#include "NodeDataModel.hpp"

#include "StyleCollection.hpp"

using QtNodes::NodeDataModel;
using QtNodes::NodeStyle;

NodeDataModel::
NodeDataModel()
  : _nodeStyle(StyleCollection::nodeStyle())
{
    // Derived classes can initialize specific style here
}

QSize NodeDataModel::captionSize() const
{
    if (!captionVisible())
    {
        return QSize(0,0);
    }

    QString name = caption().first;
    QFont f;
    f.setPointSize(12);
    f.setBold(true);
    QFontMetrics metrics(f);
    auto rect = metrics.boundingRect(name);
    QSize size(rect.width(), rect.height());

    if( icon() )
    {
        rect.setWidth( rect.width() + 30 );
        rect.setHeight( std::max( rect.height(), 24));
    }
    return size;
}


QJsonObject
NodeDataModel::
save() const
{
  QJsonObject modelJson;

  modelJson["name"] = name();

  return modelJson;
}


NodeStyle const&
NodeDataModel::
nodeStyle() const
{
  return _nodeStyle;
}


void
NodeDataModel::
setNodeStyle(NodeStyle const& style)
{
  _nodeStyle = style;
}
