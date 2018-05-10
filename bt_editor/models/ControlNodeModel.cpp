#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"

ControlNodeModel::ControlNodeModel(const QString &ID, const ParameterWidgetCreators &parameters ):
    BehaviorTreeNodeModel("Control", ID , parameters)
{
  QFontMetrics fm = _line_edit_name->fontMetrics();
  _line_edit_name->setFixedWidth( std::max( 100, fm.boundingRect( instanceName() ).width() + 12) );
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

  connect(_line_edit_name, &QLineEdit::textEdited, this, &ControlNodeModel::setInstanceName);
}

void ControlNodeModel::setInstanceName(const QString &name)
{
  BehaviorTreeNodeModel::setInstanceName(name);

  QFontMetrics fm = _line_edit_name->fontMetrics();
  const QString& txt = _line_edit_name->text();
  const int new_width = std::max( 100, fm.boundingRect(txt).width() + 12);
  _line_edit_name->setFixedWidth( new_width  );
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

  emit adjustSize();
}


SequenceModel::SequenceModel(): ControlNodeModelBase()
{
  setLabelImage(":/icons/arrow_sequence.png");
}

FallbackModel::FallbackModel(): ControlNodeModelBase()  {
  setLabelImage(":/icons/question_mark.png");
}

SequenceStarModel::SequenceStarModel(): ControlNodeModelBase() {
  setLabelImage(":/icons/arrow_sequence_star.png");
}


IfThenElseModel::IfThenElseModel(): ControlNodeModelBase() {
  _label_ID->setText( name() );
}

