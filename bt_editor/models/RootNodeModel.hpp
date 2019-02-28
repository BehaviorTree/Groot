#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include "BehaviorTreeNodeModel.hpp"

/*
class RootNodeModel : public BehaviorTreeDataModel
{

public:
  RootNodeModel();

  virtual ~RootNodeModel() = default;

public:

  unsigned int  nPorts(PortType portType) const override;

  virtual ConnectionPolicy portOutConnectionPolicy(PortIndex) const override;

  virtual const char* className() const final
  {
      return RootNodeModel::Name();
  }
  static const char* Name() { return "Root"; }

  static const TreeNodeModel& NodeModel()
  {
      static TreeNodeModel model = { Name(), NodeType::ROOT, {} };
      return model;
  }

  virtual NodeType nodeType() const final { return NodeType::ROOT; }
};
*/
