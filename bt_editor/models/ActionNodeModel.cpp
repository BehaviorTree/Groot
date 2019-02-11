#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const NodeModel &model):
  BehaviorTreeDataModel(model )
{

}


unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}

std::pair<QString, QColor> ActionNodeModel::caption() const
{
    return {registrationName(),"#ddff55"};
}

void ActionNodeModel::setInstanceName(const QString &name)
{
    _line_edit_name->setHidden( name == registrationName() );
    BehaviorTreeDataModel::setInstanceName(name);
}

ConditionNodeModel::ConditionNodeModel(const NodeModel &model):
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
