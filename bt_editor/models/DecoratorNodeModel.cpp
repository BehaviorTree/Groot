#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(const TreeNodeModel &model):
    BehaviorTreeDataModel(model )
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);
}

std::pair<QString, QColor> DecoratorNodeModel::caption() const
{ return { "Decorator", QtNodes::NodeStyle().FontColor}; }

void DecoratorNodeModel::setInstanceName(const QString &name)
{
    _line_edit_name->setHidden( name == registrationName() );
    BehaviorTreeDataModel::setInstanceName(name);
}

RetryNodeModel::RetryNodeModel():
    DecoratorNodeModel( NodeModel() )
{
}

InverterNodeModel::InverterNodeModel():
    DecoratorNodeModel( NodeModel() )
{
}

RepeatNodeModel::RepeatNodeModel():
  DecoratorNodeModel( NodeModel() )
{
    _line_edit_name->setHidden(true);
}


BlackboardConditionModel::BlackboardConditionModel():
    DecoratorNodeModel( NodeModel() )
{
}

TimeoutModel::TimeoutModel():
    DecoratorNodeModel( NodeModel() )
{
}


ForceSuccess::ForceSuccess():
    DecoratorNodeModel( NodeModel() )
{
    _line_edit_name->setHidden(true);
}

ForceFailure::ForceFailure():
    DecoratorNodeModel( NodeModel() )
{
    _line_edit_name->setHidden(true);
}
