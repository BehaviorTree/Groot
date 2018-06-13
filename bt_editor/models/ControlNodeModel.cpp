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

}

FallbackModel::FallbackModel(): ControlNodeModelBase()  {

}

SequenceStarModel::SequenceStarModel(): ControlNodeModelBase() {

}

IfThenElseModel::IfThenElseModel(): ControlNodeModelBase() {

}

