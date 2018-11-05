#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const TreeNodeModel &model):
    BehaviorTreeDataModel(decorator_ID, model )
{
    _line_edit_name->setReadOnly(true);
}

RetryNodeModel::RetryNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( RetryNodeModel::Name(), model )
{
  _line_edit_name->setHidden(true);
}

InverterNodeModel::InverterNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( InverterNodeModel::Name(), model )
{
  _line_edit_name->setHidden(true);
}

RepeatNodeModel::RepeatNodeModel(const TreeNodeModel& model):
  DecoratorNodeModel( RepeatNodeModel::Name(), model )
{
  _line_edit_name->setHidden(true);
}

