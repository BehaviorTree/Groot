#include "ActionNodeModel.hpp"


unsigned int ActionNodeModel::nPorts(QtNodes::PortType portType) const
{
    return (portType==PortType::In) ? 1:0;
}
