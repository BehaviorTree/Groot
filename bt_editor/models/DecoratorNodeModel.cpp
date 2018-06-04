#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const ParameterWidgetCreators &parameters):
    BehaviorTreeDataModel("D:", decorator_ID, parameters )
{
    _line_edit_name->setReadOnly(true);
    updateNodeSize();
}


