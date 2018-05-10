#include "ActionNodeModel.hpp"
#include <QLineEdit>

ActionNodeModel::ActionNodeModel(const QString &action_ID):
  BehaviorTreeNodeModel("Action", action_ID, ParameterWidgetCreators() )
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
}

void ActionNodeModel::setInstanceName(const QString &name)
{
  BehaviorTreeNodeModel::setInstanceName(name);

  QFontMetrics fm = _line_edit_name->fontMetrics();
  const QString& txt = _line_edit_name->text();
  _line_edit_name->setFixedWidth(fm.boundingRect(txt).width() + 12);
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
}

unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}


void ActionNodeModel::restore(const QJsonObject &modelJson)
{
  _line_edit_name->setText( modelJson["alias"].toString() );
  init();
}
