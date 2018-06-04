#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"
#include <QtDebug>

ControlNodeModel::ControlNodeModel(const QString &ID, const ParameterWidgetCreators &parameters ):
    BehaviorTreeDataModel("Control", ID , parameters)
{
    _line_edit_name->setEnabled(true);
    _line_edit_name->setReadOnly(false);

    _line_edit_name->setStyleSheet("color: black; "
                                   "background-color: rgb(200,200,200);"
                                   "border: 0px;");
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

