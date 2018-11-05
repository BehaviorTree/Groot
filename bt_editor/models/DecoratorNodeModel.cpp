#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const TreeNodeModel &model):
    BehaviorTreeDataModel(decorator_ID, model )
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);
}

RetryNodeModel::RetryNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( RetryNodeModel::Name(), model )
{
}

InverterNodeModel::InverterNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( InverterNodeModel::Name(), model )
{
}

RepeatNodeModel::RepeatNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( RepeatNodeModel::Name(), model )
{
}


BlackboardConditionModel::BlackboardConditionModel(const TreeNodeModel &model):
    DecoratorNodeModel( RepeatNodeModel::Name(), model )
{
}

TimeoutModel::TimeoutModel(const TreeNodeModel &model):
    DecoratorNodeModel( RepeatNodeModel::Name(), model )
{
}


ForceSuccess::ForceSuccess(): DecoratorNodeModel( Name(), TreeNodeModel()){}

ForceFailure::ForceFailure(): DecoratorNodeModel( Name(), TreeNodeModel()){}
