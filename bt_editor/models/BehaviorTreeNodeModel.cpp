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
#include <QJsonDocument>

const int MARGIN = 10;
const int DEFAULT_LINE_WIDTH  = 100;
const int DEFAULT_FIELD_WIDTH = 50;
const int DEFAULT_LABEL_WIDTH = 50;

BehaviorTreeDataModel::BehaviorTreeDataModel(const NodeModel &model):
    _params_widget(nullptr),
    _uid( GetUID() ),
    _model(model),
    _icon_renderer(nullptr),
    _style_caption_color( QtNodes::NodeStyle().FontColor ),
    _style_caption_alias( model.registration_ID )
{
    readStyle();
    _main_widget = new QFrame();
    _line_edit_name = new QLineEdit(_main_widget);
    _params_widget = new QFrame();

    _main_layout = new QVBoxLayout(_main_widget);
    _main_widget->setLayout( _main_layout );

    auto capt_layout = new QHBoxLayout();

    _caption_label = new QLabel();
    _caption_logo_left  = new QFrame();
    _caption_logo_right = new QFrame();
    _caption_logo_left->setFixedSize( QSize(0,20) );
    _caption_logo_right->setFixedSize( QSize(0,20) );
    _caption_label->setFixedHeight(20);

    _caption_logo_left->installEventFilter(this);

    QFont capt_font = _caption_label->font();
    capt_font.setPointSize(12);
    _caption_label->setFont(capt_font);

    capt_layout->addWidget(_caption_logo_left, 0, Qt::AlignRight);
    capt_layout->addWidget(_caption_label, 0, Qt::AlignHCenter );
    capt_layout->addWidget(_caption_logo_right, 0, Qt::AlignLeft);

    _main_layout->addLayout( capt_layout );
    _main_layout->addWidget( _line_edit_name );

    _main_layout->setMargin(0);
    _main_layout->setSpacing(2);

    //----------------------------
    _line_edit_name->setAlignment( Qt::AlignCenter );
    _line_edit_name->setText( _instance_name );
    _line_edit_name->setFixedWidth( DEFAULT_LINE_WIDTH );

    _main_widget->setAttribute(Qt::WA_NoSystemBackground);

    _line_edit_name->setStyleSheet("color: white; "
                                   "background-color: transparent;"
                                   "border: 0px;");

    //--------------------------------------
    _form_layout = new QFormLayout( _params_widget );
    _form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    _main_layout->addWidget(_params_widget);
    _params_widget->setStyleSheet("color: white;");

    _form_layout->setHorizontalSpacing(4);
    _form_layout->setVerticalSpacing(2);
    _form_layout->setContentsMargins(0, 0, 0, 0);

    PortDirection preferred_port_types[3] = { PortDirection::INPUT,
                                              PortDirection::OUTPUT,
                                              PortDirection::INOUT};

    for(int pref_index=0; pref_index < 3; pref_index++)
    {
        for(const auto& port_it: model.ports )
        {
            auto preferred_direction = preferred_port_types[pref_index];
            if( port_it.second.direction != preferred_direction )
            {
                continue;
            }

            QString description = port_it.second.description;
            QString label = port_it.first;
            if( preferred_direction == PortDirection::INPUT)
            {
                label.prepend("[IN] ");
                if( description.isEmpty())
                {
                    description="[INPUT]";
                }
                else{
                    description.prepend("[INPUT]: ");
                }
            }
            else if( preferred_direction == PortDirection::OUTPUT){
                label.prepend("[OUT] ");
                if( description.isEmpty())
                {
                    description="[OUTPUT]";
                }
                else{
                    description.prepend("[OUTPUT]: ");
                }
            }

            GrootLineEdit* form_field = new GrootLineEdit();
            form_field->setAlignment( Qt::AlignHCenter);
            form_field->setMaximumWidth(140);
            form_field->setText( port_it.second.default_value );

            connect(form_field, &GrootLineEdit::doubleClicked,
                    this, [this,form_field]()
                    { emit this->portValueDoubleChicked(form_field); });

            connect(form_field, &GrootLineEdit::lostFocus,
                    this, [this]()
                    { emit this->portValueDoubleChicked(nullptr); });

            QLabel* form_label  =  new QLabel( label, _params_widget );
            form_label->setStyleSheet("QToolTip {color: black;}");
            form_label->setToolTip( description );

            form_field->setMinimumWidth(DEFAULT_FIELD_WIDTH);

            _ports_widgets.insert( std::make_pair( port_it.first, form_field) );

            form_field->setStyleSheet("color: rgb(30,30,30); "
                                      "background-color: rgb(200,200,200); "
                                      "border: 0px; ");

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
    }
    _params_widget->adjustSize();

    capt_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    _main_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    _form_layout->setSizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    //--------------------------------------
    connect( _line_edit_name, &QLineEdit::editingFinished,
             this, [this]()
    {
        setInstanceName( _line_edit_name->text() );
    });
}

BehaviorTreeDataModel::~BehaviorTreeDataModel()
{

}

BT::NodeType BehaviorTreeDataModel::nodeType() const
{
    return _model.type;
}

void BehaviorTreeDataModel::initWidget()
{
    if( _style_icon.isEmpty() == false )
    {
        _caption_logo_left->setFixedWidth( 20 );
        _caption_logo_right->setFixedWidth( 1 );

        QFile file(_style_icon);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug()<<"file not opened: "<< _style_icon;
            file.close();
        }
        else {
            QByteArray ba = file.readAll();
            QByteArray new_color_fill = QString("fill:%1;").arg( _style_caption_color.name() ).toUtf8();
            ba.replace("fill:#ffffff;", new_color_fill);
            _icon_renderer =  new QSvgRenderer(ba, this);
        }
    }

    _caption_label->setText( _style_caption_alias );

    QPalette capt_palette = _caption_label->palette();
    capt_palette.setColor(_caption_label->backgroundRole(), Qt::transparent);
    capt_palette.setColor(_caption_label->foregroundRole(), _style_caption_color);
    _caption_label->setPalette(capt_palette);

    _caption_logo_left->adjustSize();
    _caption_logo_right->adjustSize();
    _caption_label->adjustSize();

    updateNodeSize();
}

unsigned int BehaviorTreeDataModel::nPorts(QtNodes::PortType portType) const
{
    if( portType == QtNodes::PortType::Out)
    {
        if( nodeType() == NodeType::ACTION || nodeType() == NodeType::CONDITION )
        {
            return 0;
        }
        else{
            return 1;
        }
    }
    else if( portType == QtNodes::PortType::In )
    {
        return (_model.registration_ID == "Root") ? 0 : 1;
    }
    return 0;
}

NodeDataModel::ConnectionPolicy BehaviorTreeDataModel::portOutConnectionPolicy(QtNodes::PortIndex) const
{
    return ( nodeType() == NodeType::DECORATOR || _model.registration_ID == "Root") ? ConnectionPolicy::One : ConnectionPolicy::Many;
}

void BehaviorTreeDataModel::updateNodeSize()
{
    int caption_width = _caption_label->width();
    caption_width += _caption_logo_left->width() + _caption_logo_right->width();
    int line_edit_width =  caption_width;

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
    line_edit_width = std::max( line_edit_width, label_colum_width + field_colum_width );
    _line_edit_name->setFixedWidth( line_edit_width);

    _params_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    _params_widget->adjustSize();

    _main_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    _main_widget->adjustSize();

    //----------------------------

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

void BehaviorTreeDataModel::readStyle()
{
    QFile style_file(":/NodesStyle.json");

    if (!style_file.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open NodesStyle.json");
        return;
    }

    QByteArray bytearray =  style_file.readAll();
    style_file.close();
    QJsonParseError error;
    QJsonDocument json_doc( QJsonDocument::fromJson( bytearray, &error ));

    if(json_doc.isNull()){
        qDebug()<<"Failed to create JSON doc: " << error.errorString();
        return;
    }
    if(!json_doc.isObject()){
        qDebug()<<"JSON is not an object.";
        return;
    }

    QJsonObject toplevel_object = json_doc.object();

    if(toplevel_object.isEmpty()){
        qDebug()<<"JSON object is empty.";
        return;
    }
    QString model_type_name( QString::fromStdString(toStr(_model.type)) );

    for (const auto& model_name: { model_type_name, _model.registration_ID} )
    {
        if( toplevel_object.contains(model_name) )
        {
            auto category_style = toplevel_object[ model_name ].toObject() ;
            if( category_style.contains("icon"))
            {
                _style_icon = category_style["icon"].toString();
            }
            if( category_style.contains("caption_color"))
            {
                _style_caption_color = category_style["caption_color"].toString();
            }
            if( category_style.contains("caption_alias"))
            {
                _style_caption_alias = category_style["caption_alias"].toString();
            }
        }
    }
}

const QString& BehaviorTreeDataModel::registrationName() const
{
    return _model.registration_ID;
}

const QString &BehaviorTreeDataModel::instanceName() const
{
    return _instance_name;
}

PortsMapping BehaviorTreeDataModel::getCurrentPortMapping() const
{
    PortsMapping out;

    for(const auto& it: _ports_widgets)
    {
        const auto& label = it.first;
        const auto& value = it.second;

        if(auto linedit = dynamic_cast<QLineEdit*>( value ) )
        {
            out.insert( std::make_pair( label, linedit->text() ) );
        }
        else if( auto combo = dynamic_cast<QComboBox*>( value ) )
        {
            out.insert( std::make_pair( label, combo->currentText() ) );
        }
    }
    return out;
}

QJsonObject BehaviorTreeDataModel::save() const
{
    QJsonObject modelJson;
    modelJson["name"]  = registrationName();
    modelJson["alias"] = instanceName();

    for (const auto& it: _ports_widgets)
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
    if( registrationName() != modelJson["name"].toString() )
    {
        throw std::runtime_error(" error restoring: different registration_name");
    }
    QString alias = modelJson["alias"].toString();
    setInstanceName( alias );

    for(auto it = modelJson.begin(); it != modelJson.end(); it++ )
    {
        if( it.key() != "alias" && it.key() != "name")
        {
            setPortMapping( it.key(), it.value().toString() );
        }
    }

}

void BehaviorTreeDataModel::lock(bool locked)
{
    _line_edit_name->setEnabled( !locked );

    for(const auto& it: _ports_widgets)
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

void BehaviorTreeDataModel::setPortMapping(const QString &port_name, const QString &value)
{
    auto it = _ports_widgets.find(port_name);
    if( it != _ports_widgets.end() )
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
        qDebug() << "error, label "<< port_name << " not found in the model";
    }
}

bool BehaviorTreeDataModel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Paint && obj == _caption_logo_left && _icon_renderer)
    {
        QPainter paint(_caption_logo_left);
        _icon_renderer->render(&paint);
    }
    return NodeDataModel::eventFilter(obj, event);
}


void BehaviorTreeDataModel::setInstanceName(const QString &name)
{
    _instance_name = name;
    _line_edit_name->setText( name );

    updateNodeSize();
    emit instanceNameChanged();
}


void BehaviorTreeDataModel::onHighlightPortValue(QString value)
{
    for( const auto& it:  _ports_widgets)
    {
        if( auto line_edit = dynamic_cast<QLineEdit*>(it.second) )
        {
            QString line_str = line_edit->text();
            if( !value.isEmpty() && line_str == value )
            {
                line_edit->setStyleSheet("color: rgb(30,30,30); "
                                         "background-color: #ffef0b; "
                                         "border: 0px; ");
            }
            else{
                line_edit->setStyleSheet("color: rgb(30,30,30); "
                                         "background-color: rgb(200,200,200); "
                                         "border: 0px; ");
            }
        }
    }
}

void GrootLineEdit::mouseDoubleClickEvent(QMouseEvent *ev)
{
    //QLineEdit::mouseDoubleClickEvent(ev);
    emit doubleClicked();
}

void GrootLineEdit::focusOutEvent(QFocusEvent *ev)
{
    QLineEdit::focusOutEvent(ev);
    emit lostFocus();
}
