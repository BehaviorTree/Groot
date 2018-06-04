#include "SubtreeNodeModel.hpp"
#include <QLineEdit>

SubtreeNodeModel::SubtreeNodeModel(const QString &subtree_ID,
                                   const ParameterWidgetCreators& creators):
    BehaviorTreeDataModel ("SubTree", subtree_ID, creators )
{
    _line_edit_name->setReadOnly(true);
    updateNodeSize();
}

unsigned int SubtreeNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}
