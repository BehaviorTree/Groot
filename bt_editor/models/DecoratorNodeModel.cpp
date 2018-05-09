#include "DecoratorNodeModel.hpp"
#include <QLineEdit>

DecoratorNodeModel::DecoratorNodeModel(QString decorator_ID,
                                       const ParameterWidgetCreators &parameters):
    BehaviorTreeNodeModel("Decorator", decorator_ID, parameters ),
    name_(decorator_ID)
{
    auto style = this->nodeStyle();
    style.NormalBoundaryColor = QColor(255,210,0);
    this->setNodeStyle(style);

    _line_edit_name->setReadOnly(true);
    _line_edit_name->setAlignment(Qt::AlignCenter);
    QFontMetrics fm = _line_edit_name->fontMetrics();
    _line_edit_name->setFixedWidth(fm.boundingRect(decorator_ID).width() + 12);
    _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
}
