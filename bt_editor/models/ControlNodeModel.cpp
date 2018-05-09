#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"

ControlNodeModel::ControlNodeModel(const QString &ID, const ParameterWidgetCreators &parameters ):
    BehaviorTreeNodeModel("Control", ID , parameters)
{
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
