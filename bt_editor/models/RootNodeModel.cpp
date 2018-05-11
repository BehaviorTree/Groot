#include "RootNodeModel.hpp"
#include <QLineEdit>
#include <QDebug>

RootNodeModel::RootNodeModel():
    BehaviorTreeNodeModel ( "Root", "Root", ParameterWidgetCreators() )
{
    _line_edit_name->setHidden(true);
    _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    _main_widget->adjustSize();
}

unsigned int RootNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 0:1;
}


NodeDataModel::ConnectionPolicy RootNodeModel::portOutConnectionPolicy(QtNodes::PortIndex) const
{
    return ConnectionPolicy::One;
}
