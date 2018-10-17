#include "BehaviorTreeNodeModel.hpp"
#include <QBoxLayout>
#include <QFormLayout>
#include <QSizePolicy>
#include <QLineEdit>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QApplication>

const int MARGIN = 10;
const int DEFAULT_LINE_WIDTH  = 100;
const int DEFAULT_FIELD_WIDTH = 50;
const int DEFAULT_LABEL_WIDTH = 50;

BehaviorTreeDataModel::BehaviorTreeDataModel(const QString &label_name,
                                             const QString& registration_name,
                                             const ParameterWidgetCreators &creators):
    _params_widget(nullptr),
    _uid( GetUID() ),
    _registration_name(registration_name),
    _instance_name(registration_name)
{
    _main_widget = new QFrame();
    _line_edit_name = new QLineEdit(_main_widget);
    _params_widget = new QFrame();

    _main_layout = new QVBoxLayout(_main_widget);
    _main_widget->setLayout( _main_layout );

    _main_layout->addWidget( _line_edit_name );

    _main_layout->setMargin(0);
    _main_layout->setSpacing(2);

    //----------------------------

    _line_edit_name->setAlignment( Qt::AlignCenter );
    _line_edit_name->setText( _instance_name );
    _line_edit_name->setFixedWidth( DEFAULT_LINE_WIDTH );

    _main_widget->setAttribute(Qt::WA_NoSystemBackground);

    _main_widget->setStyleSheet("background-color: transparent; color: white; ");
    _line_edit_name->setStyleSheet("color: white; "
                                   "background-color: transparent;"
                                   "border: 0px;");

    //--------------------------------------
    _form_layout = new QFormLayout( _params_widget );
    _form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    if( !creators.empty() )
    {
        _main_layout->addWidget(_params_widget);
        _params_widget->setStyleSheet("color: white;");

        _form_layout->setHorizontalSpacing(4);
        _form_layout->setVerticalSpacing(2);
        _form_layout->setContentsMargins(0, 0, 0, 0);


        for(const auto& param_creator: creators )
        {
            const QString label = param_creator.label;
            QLabel* form_label  =  new QLabel( label, _params_widget );
            QWidget* form_field = param_creator.instance_factory();

            form_field->setFixedWidth(DEFAULT_FIELD_WIDTH);

            form_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

            _params_map.insert( std::make_pair( label, form_field) );

            form_field->setStyleSheet("color: rgb(30,30,30); "
                                        "background-color: rgb(180,180,180); "
                                        "border: 0px; "
                                        "padding: 0px 0px 0px 0px;");

            _form_layout->addRow( form_label, form_field );

            auto paramUpdated = [this,label,form_field]()
            {
                this->parameterUpdated(label,form_field);
            };

            if(auto lineedit = dynamic_cast<QLineEdit*>( form_field ) )
            {
                connect( lineedit, &QLineEdit::editingFinished, this, paramUpdated );
                connect( lineedit, &QLineEdit::editingFinished,
                         this, &BehaviorTreeDataModel::updateNodeSize);
            }
            else if( auto combo = dynamic_cast<QComboBox*>( form_field ) )
            {
                connect( combo, &QComboBox::currentTextChanged, this, paramUpdated);
            }

        }
        _params_widget->adjustSize();
    }

    _main_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    _form_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);

    connect( _line_edit_name, &QLineEdit::editingFinished,
             this, [this]()
    {
        setInstanceName( _line_edit_name->text() );
    });
}
void BehaviorTreeDataModel::updateNodeSize()
{
    if( registrationName() == "Sequence")
    {
        qDebug() << "---";
    }

    int line_edit_width =  0;
    if( _line_edit_name->isHidden() == false)
    {
        QFontMetrics fm = _line_edit_name->fontMetrics();
        const QString& txt = _line_edit_name->text();
        int text_width = fm.boundingRect(txt).width();
        line_edit_width = std::max( line_edit_width, text_width + MARGIN);
    }

    //----------------------------
    int field_colum_width = DEFAULT_LABEL_WIDTH;
    int label_colum_width = 0;

    for(int row = 0; row< _form_layout->rowCount(); row++)
    {
        auto label_widget = _form_layout->itemAt(row, QFormLayout::LabelRole)->widget();
        auto field_widget = _form_layout->itemAt(row, QFormLayout::FieldRole)->widget();
        if(auto field_line_edit = dynamic_cast<QLineEdit*>(field_widget))
        {
            QFontMetrics fontMetrics = field_line_edit->fontMetrics();
            QString text = field_line_edit->text();
            int text_width = fontMetrics.boundingRect(text).width();
            field_colum_width = std::max( field_colum_width, text_width + MARGIN);
        }
        label_colum_width = std::max(label_colum_width, label_widget->width());
    }

    field_colum_width = std::max( field_colum_width,
                                  line_edit_width - label_colum_width - _form_layout->spacing());

    for(int row = 0; row< _form_layout->rowCount(); row++)
    {
        auto field_widget = _form_layout->itemAt(row, QFormLayout::FieldRole)->widget();
        if(auto field_line_edit = dynamic_cast<QLineEdit*>(field_widget))
        {
            field_line_edit->setFixedWidth( field_colum_width );
        }
    }
    _params_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    _params_widget->adjustSize();

    //----------------------------
    line_edit_width = std::max( line_edit_width, label_colum_width + field_colum_width );
    _line_edit_name->setFixedWidth( line_edit_width);

    if( registrationName() == "Sequence")
    {
        qDebug() << "line_edit_width " << line_edit_width;
        qDebug() << "label_colum_width " << label_colum_width;
        qDebug() << "field_colum_width " << field_colum_width;
        qDebug() << "---";
    }
    emit embeddedWidgetSizeUpdated();
}

QtNodes::NodeDataType BehaviorTreeDataModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return NodeDataType {"", ""};
}

std::shared_ptr<QtNodes::NodeData> BehaviorTreeDataModel::outData(QtNodes::PortIndex)
{
    return nullptr;
}

std::pair<QString, QColor> BehaviorTreeDataModel::caption() const {
    return {_registration_name, QtNodes::NodeStyle().FontColor };
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
        else if( auto combo = dynamic_cast<QComboBox*>(it->second) )
        {
            int index = combo->findText(value);
            if( index == -1 ){
                qDebug() << "error, combo value "<< value << " not found";
            }
            else{
                combo->setCurrentIndex(index);
            }
        }
    }
    else{
        qDebug() << "error, label "<< label << " not found in the model";
    }
}

void BehaviorTreeDataModel::setInstanceName(const QString &name)
{
    _instance_name = name;
    _line_edit_name->setText( name );
    updateNodeSize();

    emit instanceNameChanged();
}



