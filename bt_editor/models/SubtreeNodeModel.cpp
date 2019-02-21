#include "SubtreeNodeModel.hpp"
#include <QLineEdit>
#include <QVBoxLayout>

SubtreeNodeModel::SubtreeNodeModel(const NodeModel &model):
    BehaviorTreeDataModel ( model ),
    _expanded(false)
{
    _line_edit_name->setReadOnly(true);
    _line_edit_name->setHidden(true);

    _expand_button = new QPushButton( _expanded ? "Collapse" : "Expand", _main_widget );
    _expand_button->setMaximumWidth(100);
    _main_layout->addWidget(_expand_button);
    _main_layout->setAlignment(_expand_button, Qt::AlignHCenter);

    _expand_button->setStyleSheet(
                "QPushButton{"
                "  color: black; background-color: white; "
                "  border: 0px rgb(115, 210, 22);"
                "  padding: 4px; border-radius: 3px;}\n"
                "QPushButton:disabled { color: #303030; background-color: #a0a0a0; }");
    _expand_button->setFlat(false);
    _expand_button->setFocusPolicy(Qt::NoFocus);
    _expand_button->adjustSize();

    connect( _expand_button, &QPushButton::clicked,
             this, [this]()
    {
        emit expandButtonPushed() ;
    });

    updateNodeSize();
}

void SubtreeNodeModel::setExpanded(bool expand)
{
    _expanded = expand;
    _expand_button->setText( _expanded ? "Collapse" : "Expand");
    _expand_button->adjustSize();
    _main_widget->adjustSize();
}

void SubtreeNodeModel::setInstanceName(const QString &name)
{
    _line_edit_name->setHidden( name == registrationName() );
    BehaviorTreeDataModel::setInstanceName(name);
}

QJsonObject SubtreeNodeModel::save() const
{
    QJsonObject modelJson;
    modelJson["name"]  = registrationName();
    modelJson["alias"] = instanceName();
    modelJson["expanded"] = _expanded;

    return modelJson;
}

void SubtreeNodeModel::restore(const QJsonObject &modelJson)
{
    if( registrationName() != modelJson["name"].toString() )
    {
        throw std::runtime_error(" error restoring: different registration_name");
    }
    QString alias = modelJson["alias"].toString();
    setInstanceName( alias );
    setExpanded( modelJson["expanded"].toBool() );
}
