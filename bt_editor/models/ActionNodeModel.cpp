#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const QString &action_ID,
                                 const ParameterWidgetCreators &creators):
  BehaviorTreeDataModel(action_ID, creators )
{
    _line_edit_name->setHidden(true);
}


unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}

ConditionNodeModel::ConditionNodeModel(const QString &action_ID,
                                 const ParameterWidgetCreators &creators):
  BehaviorTreeDataModel(action_ID, creators )
{
    _line_edit_name->setHidden(true);
}


unsigned int ConditionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}



