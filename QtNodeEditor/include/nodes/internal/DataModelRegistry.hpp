#pragma once

#include <set>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QtCore/QString>
#include <QDebug>

#include "NodeDataModel.hpp"
#include "TypeConverter.hpp"
#include "Export.hpp"
#include "QStringStdHash.hpp"
#include "memory.hpp"

namespace QtNodes
{

inline
bool
operator<(QtNodes::NodeDataType const & d1,
          QtNodes::NodeDataType const & d2)
{
  return d1.id < d2.id;
}


/// Class uses map for storing models (name, model)
class NODE_EDITOR_PUBLIC DataModelRegistry
{

public:

  using RegistryItemPtr     = std::unique_ptr<NodeDataModel>;
  using RegistryItemCreator = std::function<RegistryItemPtr()>;
  using RegisteredModelCreatorsMap = std::unordered_map<QString, RegistryItemCreator>;
  using RegisteredModelsCategoryMap = std::unordered_map<QString, QString>;
  using CategoriesSet = std::set<QString>;

  using RegisteredTypeConvertersMap = std::map<TypeConverterId, TypeConverter>;

  DataModelRegistry()  = default;
  ~DataModelRegistry() = default;

  DataModelRegistry(DataModelRegistry const &) = delete;
  DataModelRegistry(DataModelRegistry &&)      = default;

  DataModelRegistry&operator=(DataModelRegistry const &) = delete;
  DataModelRegistry&operator=(DataModelRegistry &&)      = default;

public:

  bool isRegistered(QString const & ID) const
  {
      return _registeredItemCreators.find(ID) != _registeredItemCreators.end();
  }

  void registerModel(QString const &category,
                     RegistryItemCreator creator,
                     QString ID = "");

  template<typename ModelType>
  void registerModel(QString const &category)
  {
    RegistryItemCreator creator = [](){ return detail::make_unique<ModelType>(); };

    QString const name = ModelType::Name();

    _registeredItemCreators[name] = std::move(creator);
    _categories.insert(category);
    _registeredModelsCategory[name] = category;
  }

  void registerTypeConverter(TypeConverterId const & id,
                             TypeConverter typeConverter)
  {
    _registeredTypeConverters[id] = std::move(typeConverter);
  }

  void unregisterModel(const QString& name)
  {
      _registeredItemCreators.erase(name);
      _registeredModelsCategory.erase(name);
  }

  std::unique_ptr<NodeDataModel>create(QString const &modelName);

  RegisteredModelCreatorsMap const &registeredModelCreators() const;

  RegisteredModelsCategoryMap const &registeredModelsCategoryAssociation() const;

  CategoriesSet const &categories() const;

  TypeConverter getTypeConverter(NodeDataType const & d1,
                                 NodeDataType const & d2) const;

  std::unordered_set<QString> registeredModelsByCategory(const QString& category);

private:

  RegisteredModelsCategoryMap _registeredModelsCategory;

  CategoriesSet _categories;

  RegisteredModelCreatorsMap _registeredItemCreators;

  RegisteredTypeConvertersMap _registeredTypeConverters;
};



inline void
DataModelRegistry::
    registerModel(QString const &category,
                  RegistryItemCreator creator,
                  QString name)
{
  if( name.isEmpty() )
  {
      RegistryItemPtr prototypeInstance = creator();
      name = prototypeInstance->name();
  }

  _registeredItemCreators[name] = std::move(creator);
  _categories.insert(category);
  _registeredModelsCategory[name] = category;

}

}
