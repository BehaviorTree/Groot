#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const ParameterWidgetCreators &parameters):
    BehaviorTreeDataModel(decorator_ID, parameters )
{
    _line_edit_name->setReadOnly(true);
}

RetryNodeModel::RetryNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( RetryNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
}

NegationNodeModel::NegationNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( NegationNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
}

RepeatNodeModel::RepeatNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( RepeatNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
}

