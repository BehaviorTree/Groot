#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const ParameterWidgetCreators &parameters):
    BehaviorTreeDataModel("D:", decorator_ID, parameters )
{
    _line_edit_name->setReadOnly(true);
    updateNodeSize();
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
  _main_layout->setSpacing(0);
}

RepeatNodeModel::RepeatNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( RepeatNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
}

