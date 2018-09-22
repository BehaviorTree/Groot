#pragma once

#include <QObject>
#include <memory>
#include <utility>


namespace detail {

#if (!defined(_MSC_VER) && (__cplusplus < 201300)) || \
    ( defined(_MSC_VER) && (_MSC_VER < 1800)) 
//_MSC_VER == 1800 is Visual Studio 2013, which is already somewhat C++14 compilant, 
// and it has make_unique in it's standard library implementation
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#else
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
      return std::make_unique<T>(std::forward<Args>(args)...);
    }
#endif

/*!
  Objects that inherit from `QObject` shall not be disposed of by calling
  `delete` but shall invoke the method `::deleteLater`.
  \sa https://stackoverflow.com/questions/41402152/stdunique-ptr-and-qobjectdeletelater
*/
struct QObjectDeleter {
  void operator()(QObject* o) {
    if (o)
      o->deleteLater();
  }
};

template <typename T>
using unique_qptr = std::unique_ptr<T, QObjectDeleter>;

}
