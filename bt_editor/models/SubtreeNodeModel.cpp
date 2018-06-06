#include "SubtreeNodeModel.hpp"
#include <QLineEdit>
#include <QVBoxLayout>

SubtreeNodeModel::SubtreeNodeModel(const QString &subtree_ID,
                                   const ParameterWidgetCreators& creators):
    BehaviorTreeDataModel ("SubTree", subtree_ID, creators ),
    _expand(false)
{
    _line_edit_name->setReadOnly(true);

    auto vlayout = dynamic_cast<QVBoxLayout*>(_main_widget->layout());

    _expand_button = new QPushButton("Expand", _main_widget );
    vlayout->addWidget(_expand_button);

    _expand_button->setStyleSheet("color: black; background-color: white; border: 1px orange;");
    _expand_button->setFlat(false);
    _expand_button->setFocusPolicy(Qt::NoFocus);

    connect( _expand_button, &QPushButton::clicked,
             this, [this]()
    {
      _expand = !_expand;
      emit numberOfPortsChanged(1, (_expand ? 1: 0));
      updateNodeSize();
    } );

    updateNodeSize();
}

unsigned int SubtreeNodeModel::nPorts(QtNodes::PortType portType) const
{
  return (portType==PortType::In) ? 1: (_expand ? 1: 0);
}

