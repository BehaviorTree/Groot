#include "RootNodeModel.hpp"
#include <QLineEdit>
#include <QDebug>

RootNodeModel::RootNodeModel():
    BehaviorTreeDataModel ( "Root", "Root", ParameterWidgetCreators() )
{
    _line_edit_name->setHidden(true);
    _main_widget->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    _main_widget->adjustSize();
    setUID( std::numeric_limits<uint16_t>::max() );
}

unsigned int RootNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 0:1;
}


NodeDataModel::ConnectionPolicy RootNodeModel::portOutConnectionPolicy(QtNodes::PortIndex) const
{
    return ConnectionPolicy::One;
}
