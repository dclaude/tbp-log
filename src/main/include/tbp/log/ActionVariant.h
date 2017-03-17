#pragma once

#include "tbp/log/AsyncLogger.fwd.h"

namespace tbp
{
namespace log
{

/*
to avoid a dependency on boost for boost::variant
from http://stackoverflow.com/questions/24713833/using-a-union-with-unique-ptr
*/
template <typename SpscQueue, typename Allocator>
class ActionVariant
{
public:
   using AddMsg = AsyncLoggerAddMsg<SpscQueue, Allocator>;
   using RemoveMsg = AsyncLoggerRemoveMsg<SpscQueue>;
   //
   ~ActionVariant()
   {
      Destroy();
   }
   void Set(AddMsg val)
   {
      Destroy();
      new (&m_data.m_add) AddMsg(std::move(val));
      m_type = Type::Add;
   }
   void Set(RemoveMsg val)
   {
      Destroy();
      new (&m_data.m_remove) RemoveMsg(std::move(val));
      m_type = Type::Remove;
   }
   template <typename FUNC> void ApplyVisitor(FUNC func)
   {
      switch (m_type)
      {
         case Type::Add: 
            {
               func(m_data.m_add);
            }
            break;
         case Type::Remove: 
            {
               func(m_data.m_remove);
            }
            break;
      }
   }

private:
   enum class Type { Add, Remove };
   union Union
   {
      Union() { new (&m_add) AddMsg();}
      ~Union() {}
      //
      AddMsg m_add;
      RemoveMsg m_remove;
   };
   //
   void Destroy()
   {
      switch (m_type)
      {
         case Type::Add: 
            {
               m_data.m_add.~AddMsg(); 
            }
            break;
         case Type::Remove: 
            {
               m_data.m_remove.~RemoveMsg(); 
            }
            break;
      }
   }
   //
   Type m_type = Type::Add; // match the default ctor of Union
   Union m_data;

};

}
}

