#include "SubtreeNodeModel.hpp"

SubtreeNodeModel::SubtreeNodeModel(const QString &subtree_ID,
                                   const ParameterWidgetCreators& creators):
    BehaviorTreeNodeModel ("SubTree", subtree_ID, creators ),
    name_(subtree_ID)
{
    auto style = this->nodeStyle();
    style.NormalBoundaryColor = QColor(255,210,0);
    this->setNodeStyle(style);
}

unsigned int SubtreeNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}
