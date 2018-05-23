#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"
#include <QtDebug>

ControlNodeModel::ControlNodeModel(const QString &ID, const ParameterWidgetCreators &parameters ):
    BehaviorTreeDataModel("Control", ID , parameters)
{

}

void ControlNodeModel::init()
{
  QFontMetrics fm = _line_edit_name->fontMetrics();
  const QString& txt = _line_edit_name->text();
  const int new_width = std::max( 120, fm.boundingRect(txt).width() + 12 );
  _line_edit_name->setFixedWidth(new_width);
  _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
  _main_widget->adjustSize();
}


void ControlNodeModel::setInstanceName(const QString &name)
{
  BehaviorTreeDataModel::setInstanceName(name);
  init();
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

