#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include "BehaviorTreeNodeModel.hpp"


class RootNodeModel : public BehaviorTreeDataModel
{

public:
  RootNodeModel();

  virtual ~RootNodeModel() = default;

public:

  unsigned int  nPorts(PortType portType) const override;

  virtual ConnectionPolicy portOutConnectionPolicy(PortIndex) const override;

  virtual const char* className() const override final
  {
    return "Root";
  }

  virtual NodeType nodeType() const override final { return NodeType::ROOT; }
};

