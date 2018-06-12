#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const ParameterWidgetCreators &parameters):
    BehaviorTreeDataModel("D:", decorator_ID, parameters ),
    _renderer(nullptr)
{
    _line_edit_name->setReadOnly(true);
    updateNodeSize();
}

RetryNodeModel::RetryNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( RetryNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
  _renderer = new QSvgRenderer( QString( ":/icons/svg/retry.svg") );
}

NegationNodeModel::NegationNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( NegationNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
  _renderer = new QSvgRenderer( QString( ":/icons/svg/exclaimation_mark.svg") );
  _main_layout->setSpacing(0);
}

RepeatNodeModel::RepeatNodeModel(const ParameterWidgetCreators &parameters):
  DecoratorNodeModel( RepeatNodeModel::Name(), parameters )
{
  _line_edit_name->setHidden(true);
  _renderer = new QSvgRenderer( QString( ":/icons/svg/repeat.svg") );
}

