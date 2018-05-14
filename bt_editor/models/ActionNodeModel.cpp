#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const QString &action_ID,
                                 const ParameterWidgetCreators &creators):
  BehaviorTreeNodeModel("Action", action_ID, creators )
{
  init();
}

void ActionNodeModel::init()
{
  _line_edit_name->setReadOnly(true);
  _line_edit_name->setAlignment(Qt::AlignCenter);
  QFontMetrics fm = _line_edit_name->fontMetrics();
  const QString& txt = _line_edit_name->text();
  _line_edit_name->setFixedWidth(fm.boundingRect(txt).width() + 12);
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
  _main_widget->adjustSize();
}

void ActionNodeModel::setInstanceName(const QString &name)
{
  BehaviorTreeNodeModel::setInstanceName(name);
  init();
}

unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}



