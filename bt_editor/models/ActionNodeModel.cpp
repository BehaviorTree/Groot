#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const QString &action_ID):
  BehaviorTreeNodeModel("Action", action_ID, ParameterWidgetCreators() ),
  name_(action_ID)
{
  _line_edit_name->setReadOnly(true);
  _line_edit_name->setAlignment(Qt::AlignCenter);
  QFontMetrics fm = _line_edit_name->fontMetrics();
  _line_edit_name->setFixedWidth(fm.boundingRect(action_ID).width() + 12);
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
}

unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}
