#include "BehaviorTreeNodeModel.hpp"
#include <QBoxLayout>
#include <QFormLayout>
#include <QSizePolicy>
#include <QLineEdit>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFont>

BehaviorTreeDataModel::BehaviorTreeDataModel(const QString &label_name,
                                             const QString& registration_name,
                                             const ParameterWidgetCreators &creators):
    _params_widget(nullptr),
    _uid( GetUID() ),
    _registration_name(registration_name),
    _instance_name(registration_name)
{
    _main_widget = new QFrame();
    _label_ID = new QLabel();
    _line_edit_name = new QLineEdit();
    _params_widget = new QFrame();

    _main_layout = new QVBoxLayout();
    _main_widget->setLayout( _main_layout );

    _top_layout = new QHBoxLayout();
    _main_layout->addLayout(_top_layout);

    _top_layout->addWidget( _label_ID, 0 );
    _top_layout->addWidget( _line_edit_name, 1 );

    _main_layout->setMargin(0);
    _main_layout->setSpacing(6);

    _top_layout->setMargin(0);
    _top_layout->setSpacing(0);

    //----------------------------

    _label_ID->setHidden(true);
    _label_ID->setText( label_name );
    _label_ID->setAlignment(Qt::AlignCenter);

    _line_edit_name->setAlignment( Qt::AlignCenter );

    QFont font = _label_ID->font();
    font.setPointSize(10);
    font.setBold(true);
    _label_ID->setFont(font);

    QPalette palette = _label_ID->palette();
    palette.setColor(_label_ID->foregroundRole(), Qt::white);
    _label_ID->setPalette(palette);

    _main_widget->setAttribute(Qt::WA_NoSystemBackground);

    _main_widget->setStyleSheet("background-color: transparent; color: white; ");
    _line_edit_name->setStyleSheet("color: white; "
                                   "background-color: "
                                   "rgba(0,0,0,0); border: 0px;");

    //--------------------------------------

    if( !creators.empty() )
    {
        _main_layout->addWidget(_params_widget);
        _params_widget->setStyleSheet("color: white;");

        _form_layout = new QFormLayout( _params_widget );
        _form_layout->setHorizontalSpacing(4);
        _form_layout->setVerticalSpacing(0);
        _form_layout->setContentsMargins(0, 6, 0, 0);

        for(const auto& param_creator: creators )
        {
            const QString label = param_creator.label;
            QLabel* field_label =  new QLabel( label, _params_widget );
            QWidget* field_widget = param_creator.instance_factory();

            _params_map.insert( std::make_pair( label, field_widget) );

            field_widget->setStyleSheet("color: white; "
                                        "background-color: gray; "
                                        "border: 0px; "
                                        "padding: 0px 0px 0px 0px;");

            _form_layout->addRow( field_label, field_widget );

            auto paramUpdated = [this,label,field_widget]()
            {
                this->parameterUpdated(label,field_widget);
            };

            if(auto lineedit = dynamic_cast<QLineEdit*>( field_widget ) )
            {
                connect( lineedit, &QLineEdit::editingFinished, paramUpdated );
            }
            else if( auto combo = dynamic_cast<QComboBox*>( field_widget ) )
            {
                connect( combo, &QComboBox::currentTextChanged, paramUpdated);
            }

        }
        _params_widget->adjustSize();
        _params_widget->setLayout( _form_layout );
        _form_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    }

    _main_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    _top_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);

    connect( _line_edit_name, &QLineEdit::editingFinished,
             this, [this]()
    {
        setInstanceName( _line_edit_name->text() );
    });

    setInstanceName( _instance_name );
}

QtNodes::NodeDataType BehaviorTreeDataModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return NodeDataType {"", ""};
}

std::shared_ptr<QtNodes::NodeData> BehaviorTreeDataModel::outData(QtNodes::PortIndex)
{
    return nullptr;
}

QString BehaviorTreeDataModel::caption() const {
    return _registration_name;
}

const QString &BehaviorTreeDataModel::registrationName() const
{
    return _registration_name;
}

const QString &BehaviorTreeDataModel::instanceName() const
{
    return _instance_name;
}

std::vector<std::pair<QString, QString>> BehaviorTreeDataModel::getCurrentParameters() const
{
    std::vector<std::pair<QString, QString>> out;

    for(const auto& it: _params_map)
    {
        const auto& label = it.first;
        const auto& field_widget = it.second;

        if(auto linedit = dynamic_cast<QLineEdit*>( field_widget ) )
        {
            out.push_back( { label, linedit->text() } );
        }
        else if( auto combo = dynamic_cast<QComboBox*>( field_widget ) )
        {
            out.push_back( { label, combo->currentText() } );
        }
    }

    return out;
}

QJsonObject BehaviorTreeDataModel::save() const
{
    QJsonObject modelJson;
    modelJson["name"]  = registrationName();
    modelJson["alias"] = instanceName();

    for (const auto& it: _params_map)
    {
        if( auto linedit = dynamic_cast<QLineEdit*>(it.second)){
            modelJson[it.first] = linedit->text();
        }
        else if( auto combo = dynamic_cast<QComboBox*>(it.second)){
            modelJson[it.first] = combo->currentText();
        }
    }

    return modelJson;
}

void BehaviorTreeDataModel::restore(const QJsonObject &modelJson)
{
    if( _registration_name != modelJson["name"].toString() )
    {
        throw std::runtime_error(" error restoring: different registration_name");
    }
    QString alias = modelJson["alias"].toString();
    setInstanceName( alias );

    for(auto it = modelJson.begin(); it != modelJson.end(); it++ )
    {
        if( it.key() != "alias" && it.key() != "name")
        {
            setParameterValue( it.key(), it.value().toString() );
        }
    }

}

void BehaviorTreeDataModel::lock(bool locked)
{
    _line_edit_name->setEnabled( !locked );

    for(const auto& it: _params_map)
    {
        const auto& field_widget = it.second;

        if(auto lineedit = dynamic_cast<QLineEdit*>( field_widget  ))
        {
            lineedit->setReadOnly( locked );
        }
        else if(auto combo = dynamic_cast<QComboBox*>( field_widget ))
        {
            combo->setEnabled( !locked );
        }
    }
}

void BehaviorTreeDataModel::setParameterValue(const QString &label, const QString &value)
{
    auto it = _params_map.find(label);
    if( it != _params_map.end() )
    {
        if( auto lineedit = dynamic_cast<QLineEdit*>(it->second) )
        {
            lineedit->setText(value);
        }
        else if( auto combo = dynamic_cast<QLineEdit*>(it->second) )
        {
            combo->setText(value);
        }
    }
}

void BehaviorTreeDataModel::updateNodeSize()
{
    QFontMetrics fm = _line_edit_name->fontMetrics();
    const QString& txt = _line_edit_name->text();
    double new_width = std::max( 100, fm.boundingRect(txt).width() + 20);

    QFont f;
    f.setBold(true);
    QFontMetrics boldFontMetrics(f);
    double caption_width = boldFontMetrics.boundingRect( caption() ).width();
    if ( icon() )
    {
      caption_width += 40;
    }
    new_width = std::max( new_width, caption_width);

    _line_edit_name->setFixedWidth(new_width);
    emit embeddedWidgetSizeUpdated();
}

void BehaviorTreeDataModel::setInstanceName(const QString &name)
{
    _instance_name = name;
    _line_edit_name->setText( name );
    updateNodeSize();

    emit instanceNameChanged();
}



