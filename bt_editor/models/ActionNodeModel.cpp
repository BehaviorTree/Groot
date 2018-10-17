#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const QString &action_ID,
                                 const ParameterWidgetCreators &creators):
  BehaviorTreeDataModel("A:", action_ID, creators )
{
    _line_edit_name->setReadOnly(true);
}


unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}

ConditionNodeModel::ConditionNodeModel(const QString &action_ID,
                                 const ParameterWidgetCreators &creators):
  BehaviorTreeDataModel("A:", action_ID, creators )
{
    _line_edit_name->setReadOnly(true);
}


unsigned int ConditionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}



