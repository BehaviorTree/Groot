#include "SubtreeNodeModel.hpp"
#include <QLineEdit>

SubtreeNodeModel::SubtreeNodeModel(const QString &subtree_ID,
                                   const ParameterWidgetCreators& creators):
    BehaviorTreeNodeModel ("SubTree", subtree_ID, creators )
{
    auto style = this->nodeStyle();
    style.NormalBoundaryColor = QColor(255,210,0);
    this->setNodeStyle(style);

    _line_edit_name->setReadOnly(true);
    _line_edit_name->setAlignment(Qt::AlignCenter);
    QFontMetrics fm = _line_edit_name->fontMetrics();
    _line_edit_name->setFixedWidth(fm.boundingRect(subtree_ID).width() + 12);
    _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    _main_widget->adjustSize();
}

unsigned int SubtreeNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}
