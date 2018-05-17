#include "BehaviorTreeNodeModel.hpp"
#include <QBoxLayout>
#include <QFormLayout>
#include <QSizePolicy>
#include <QLineEdit>
#include <QComboBox>
#include <QDebug>
#include <QFile>

BehaviorTreeNodeModel::BehaviorTreeNodeModel(const QString &label_name,
                                             const QString& registration_name,
                                             const ParameterWidgetCreators &creators):
    _params_widget(nullptr),
    _form_layout(nullptr),
    _registration_name(registration_name),
    _instance_name(registration_name)
{
  _main_widget = new QWidget;
  _main_widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  _label_ID = new QLabel( _main_widget );
  _label_ID->setHidden(true);
  _label_ID->setText( label_name );
  _label_ID->setAlignment(Qt::AlignCenter);
  _label_ID->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  _line_edit_name = new QLineEdit( _main_widget );
  _line_edit_name->setText( _instance_name );
  _line_edit_name->setAlignment(Qt::AlignCenter);
  _line_edit_name->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  QHBoxLayout *top_layout  = new QHBoxLayout(  );

  top_layout->addWidget( _label_ID,0 );
  top_layout->addWidget( _line_edit_name, 1 );
  top_layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

  QVBoxLayout *main_layout = new QVBoxLayout( _main_widget );
  main_layout->addLayout(top_layout);

  QFont font = _label_ID->font();
  font.setPointSize(10);
  font.setBold(true);
  _label_ID->setFont(font);

  QPalette palette = _label_ID->palette();
  palette.setColor(_label_ID->foregroundRole(), Qt::white);
  _label_ID->setPalette(palette);

  _main_widget->setAttribute(Qt::WA_NoSystemBackground);
  _main_widget->setLayout( main_layout );


  main_layout->setMargin(0);
  _main_widget->setStyleSheet("background-color: transparent; color: white; ");
  _line_edit_name->setStyleSheet("color: black; background-color: white");

  //--------------------------------------
  _params_widget = new QWidget( _main_widget );
  _form_layout = new QFormLayout( _params_widget );

  if( !creators.empty() )
  {
      main_layout->addWidget(_params_widget);
      _params_widget->setStyleSheet("color: white;");

      _form_layout->setHorizontalSpacing(2);
      _form_layout->setVerticalSpacing(2);
      _form_layout->setContentsMargins(0, 0, 0, 0);

      for(const auto& param_creator: creators )
      {
        const QString label = param_creator.label;
        QLabel* field_label =  new QLabel( label, _params_widget );
        QWidget* field_widget = param_creator.instance_factory();

        _params_map.insert( std::make_pair( label, field_widget) );

        field_widget->setStyleSheet("color: white; "
                                    "background-color: gray; "
                                    "border: 1px solid #FFFFFF; "
                                    "padding: 1px 0px 1px 3px;");

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
      main_layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
      _form_layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
      _params_widget->adjustSize();
  }

  _main_widget->adjustSize();

  connect( _line_edit_name, &QLineEdit::editingFinished,
           [this]()
  {
    if( _line_edit_name->text() != _instance_name)
    {
      _instance_name = _line_edit_name->text();
      this->instanceNameChanged();
    }
  } );

  emit adjustSize();
}

QtNodes::NodeDataType BehaviorTreeNodeModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return NodeDataType {"", ""};
}

std::shared_ptr<QtNodes::NodeData> BehaviorTreeNodeModel::outData(QtNodes::PortIndex)
{
    return nullptr;
}

QString BehaviorTreeNodeModel::caption() const {
  return _registration_name;
}

const QString &BehaviorTreeNodeModel::registrationName() const
{
  return _registration_name;
}

const QString &BehaviorTreeNodeModel::instanceName() const
{
  return _instance_name;
}

std::vector<std::pair<QString, QString>> BehaviorTreeNodeModel::getCurrentParameters() const
{
  std::vector<std::pair<QString, QString>> out;

  for(int row = 0; row < _form_layout->rowCount(); row++)
  {
    auto label_item = _form_layout->itemAt(row, QFormLayout::LabelRole);
    auto field_item  = _form_layout->itemAt(row, QFormLayout::FieldRole);
    if( label_item && field_item )
    {
      QLabel* label = static_cast<QLabel*>( label_item->widget() );

      if(auto linedit = dynamic_cast<QLineEdit*>( field_item->widget() ) )
      {
        out.push_back( { label->text(), linedit->text() } );
      }
      else if( auto combo = dynamic_cast<QComboBox*>( field_item->widget() ) )
      {
        out.push_back( { label->text(), combo->currentText() } );
      }
      break;
    }
  }
  return out;
}

QJsonObject BehaviorTreeNodeModel::save() const
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

void BehaviorTreeNodeModel::restore(const QJsonObject &modelJson)
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

void BehaviorTreeNodeModel::lock(bool locked)
{
  _line_edit_name->setEnabled( !locked );

  for(int row = 0; row < _form_layout->rowCount(); row++)
  {
    auto field_item  = _form_layout->itemAt(row, QFormLayout::FieldRole);

    if( field_item )
    {
      if(auto lineedit = dynamic_cast<QLineEdit*>( field_item->widget() ))
      {
        lineedit->setReadOnly( locked );
      }
      else if(auto combo = dynamic_cast<QComboBox*>( field_item->widget() ))
      {
        combo->setEnabled( !locked );
      }
    }
  }
}

void BehaviorTreeNodeModel::setParameterValue(const QString &label, const QString &value)
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

void BehaviorTreeNodeModel::setInstanceName(const QString &name)
{
  _instance_name = name;
  _line_edit_name->setText( name );
}



