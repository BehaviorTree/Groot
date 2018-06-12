#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"
#include <QtDebug>

ControlNodeModel::ControlNodeModel(const QString &ID, const ParameterWidgetCreators &parameters ):
    BehaviorTreeDataModel("Control", ID , parameters),
    _renderer(nullptr)
{
    _line_edit_name->setEnabled(true);
    _line_edit_name->setReadOnly(false);

    _line_edit_name->setStyleSheet("color: black; "
                                   "background-color: rgb(200,200,200);"
                                   "border: 0px;");
}


SequenceModel::SequenceModel(): ControlNodeModelBase()
{
  _renderer = new QSvgRenderer( QString(":/icons/svg/arrow_right.svg") );
}

FallbackModel::FallbackModel(): ControlNodeModelBase()  {
  _renderer = new QSvgRenderer( QString(":/icons/svg/question_mark.svg") );
}

SequenceStarModel::SequenceStarModel(): ControlNodeModelBase() {
  _renderer = new QSvgRenderer( QString(":/icons/svg/arrow_right.svg") );
}

IfThenElseModel::IfThenElseModel(): ControlNodeModelBase() {

}

