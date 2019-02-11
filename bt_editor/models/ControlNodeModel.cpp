#include "ControlNodeModel.hpp"
#include "ControlNodeModel.hpp"
#include <QtDebug>

ControlNodeModel::ControlNodeModel(const BT_NodeModel &model ):
    BehaviorTreeDataModel(model)
{
    _line_edit_name->setEnabled(true);
    _line_edit_name->setReadOnly(false);

    _line_edit_name->setStyleSheet("color: rgb(30,30,30); "
                                   "background-color: rgb(180,180,180);"
                                   "border: 0px;");
}


SequenceModel::SequenceModel(): ControlNodeModel( NodeModel() )
{
}

FallbackModel::FallbackModel(): ControlNodeModel( NodeModel() )
{
}

SequenceStarModel::SequenceStarModel(): ControlNodeModel( NodeModel() )
{
}

IfThenElseModel::IfThenElseModel(): ControlNodeModel( NodeModel() )
{
}


FallbackStarModel::FallbackStarModel(): ControlNodeModel( NodeModel() )
{
}
