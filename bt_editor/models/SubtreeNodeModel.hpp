#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"
#include <QPushButton>

class SubtreeNodeModel : public BehaviorTreeDataModel
{
  Q_OBJECT
public:

    SubtreeNodeModel(const QString& subtree_ID,
                     const ParameterWidgetCreators &creators);

    ~SubtreeNodeModel() = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual const char* className() const final
    {
      return "SubTree";
    }

   QString caption() const override {
        return "SubTree";
    }

    virtual NodeType nodeType() const final { return NodeType::SUBTREE; }

signals:
   void numberOfPortsChanged(int portsIn, int portsOut);

private:
   bool _expand;

   QPushButton* _expand_button;

};


#endif // SUBTREE_NODEMODEL_HPP
