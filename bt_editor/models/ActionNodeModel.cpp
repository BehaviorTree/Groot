#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const TreeNodeModel &model):
  BehaviorTreeDataModel(model )
{
    _line_edit_name->setHidden(true);
}


unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}

ConditionNodeModel::ConditionNodeModel(const TreeNodeModel &model):
  BehaviorTreeDataModel( model )
{
    _line_edit_name->setHidden(true);
}


unsigned int ConditionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}


ActionFailure::ActionFailure(): ActionNodeModel( NodeModel() ){}

ActionSuccess::ActionSuccess(): ActionNodeModel( NodeModel() ){}
