#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include "BehaviorTreeNodeModel.hpp"


class RootNodeModel : public BehaviorTreeNodeModel
{

public:
  RootNodeModel();

  virtual ~RootNodeModel() = default;

public:

  unsigned int  nPorts(PortType portType) const override;

  virtual ConnectionPolicy portOutConnectionPolicy(PortIndex) const override;
};

