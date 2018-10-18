#include "SubtreeNodeModel.hpp"
#include <QLineEdit>
#include <QVBoxLayout>

SubtreeNodeModel::SubtreeNodeModel(const QString &subtree_ID,
                                   const ParameterWidgetCreators& creators):
    BehaviorTreeDataModel (subtree_ID, creators ),
    _renderer(nullptr)
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);

    _expand_button = new QPushButton("Expand", _main_widget );
    _expand_button->setMaximumWidth(100);
    _main_layout->addWidget(_expand_button);
    _main_layout->setAlignment(_expand_button, Qt::AlignHCenter);

    _expand_button->setStyleSheet("color: black; background-color: white; "
                                  "border: 0px rgb(115, 210, 22);"
                                  "padding: 4px; border-radius: 3px; ");
    _expand_button->setFlat(false);
    _expand_button->setFocusPolicy(Qt::NoFocus);

    connect( _expand_button, &QPushButton::clicked,
             this, &SubtreeNodeModel::expandButtonPushed );

    updateNodeSize();

    _renderer = new QSvgRenderer( QString(":/icons/svg/subtree.svg"), this );
}

SubtreeExpandedNodeModel::SubtreeExpandedNodeModel(
        const QString &subtree_ID,
        const ParameterWidgetCreators& creators):
    BehaviorTreeDataModel (subtree_ID, creators ),
    _renderer(nullptr)
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);

    _collapse_button = new QPushButton("Collapse", _main_widget );
    _main_layout->addWidget(_collapse_button);
    _main_layout->setAlignment(_collapse_button, Qt::AlignHCenter);

    _collapse_button->setStyleSheet("color: black; background-color: white; "
                                    "border: 0px rgb(115, 210, 22);"
                                    " margin: 2px; padding: 4px; border-radius: 3px; ");
    _collapse_button->setFlat(false);
    _collapse_button->setFocusPolicy(Qt::NoFocus);

    connect( _collapse_button, &QPushButton::clicked,
             this, &SubtreeExpandedNodeModel::collapseButtonPushed );

    updateNodeSize();

    _renderer = new QSvgRenderer( QString(":/icons/svg/subtree.svg") );
}

NodeDataModel::ConnectionPolicy SubtreeExpandedNodeModel::portOutConnectionPolicy(QtNodes::PortIndex) const
{
  return ConnectionPolicy::One;
}



