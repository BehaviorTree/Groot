#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const TreeNodeModel &model):
    BehaviorTreeDataModel(decorator_ID, model )
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);
}

RetryNodeModel::RetryNodeModel():
  DecoratorNodeModel( Name(), NodeModel() )
{
}

InverterNodeModel::InverterNodeModel():
  DecoratorNodeModel( Name(), TreeNodeModel() )
{
}

RepeatNodeModel::RepeatNodeModel():
  DecoratorNodeModel( Name(), NodeModel() )
{
    _line_edit_name->setHidden(true);
}


BlackboardConditionModel::BlackboardConditionModel():
    DecoratorNodeModel( Name(), NodeModel() )
{
}

TimeoutModel::TimeoutModel():
    DecoratorNodeModel( Name(), NodeModel() )
{
}


ForceSuccess::ForceSuccess():
    DecoratorNodeModel( Name(), TreeNodeModel())
{
    _line_edit_name->setHidden(true);
}

ForceFailure::ForceFailure():
    DecoratorNodeModel( Name(), TreeNodeModel())
{
    _line_edit_name->setHidden(true);
}
