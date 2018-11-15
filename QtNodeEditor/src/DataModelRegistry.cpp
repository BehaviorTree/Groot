#include "DataModelRegistry.hpp"

#include <QtCore/QFile>
#include <QDebug>

using QtNodes::DataModelRegistry;
using QtNodes::NodeDataModel;
using QtNodes::NodeDataType;
using QtNodes::TypeConverter;

std::unique_ptr<NodeDataModel>
DataModelRegistry::
create(QString const &modelName)
{
  auto it = _registeredItemCreators.find(modelName);

  if (it != _registeredItemCreators.end())
  {
    return it->second();
  }

  qDebug() << "DataModelRegistry::create : unable to create [" <<
              modelName << "]\nCandidates are:\n";
  for(auto& creator_it: _registeredItemCreators)
  {
      qDebug() << "   " << creator_it.first;
  }
  return nullptr;
}


DataModelRegistry::RegisteredModelCreatorsMap const &
DataModelRegistry::
registeredModelCreators() const
{
  return _registeredItemCreators;
}


DataModelRegistry::RegisteredModelsCategoryMap const &
DataModelRegistry::
registeredModelsCategoryAssociation() const
{
  return _registeredModelsCategory;
}


DataModelRegistry::CategoriesSet const &
DataModelRegistry::
categories() const
{
  return _categories;
}


TypeConverter
DataModelRegistry::
getTypeConverter(NodeDataType const & d1,
                 NodeDataType const & d2) const
{
  TypeConverterId converterId = std::make_pair(d1, d2);

  auto it = _registeredTypeConverters.find(converterId);

  if (it != _registeredTypeConverters.end())
  {
    return it->second;
  }

  return TypeConverter{};
}

std::unordered_set<QString> DataModelRegistry::registeredModelsByCategory(const QString &category)
{
  std::unordered_set<QString> names_in_category;
  for (auto& it: _registeredModelsCategory)
  {
    if( it.second == category)
    {
      names_in_category.insert(it.first);
    }
  }
  return names_in_category;
}
