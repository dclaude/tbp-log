#pragma once

#include "tbp/log/Level.h"
#include "tbp/log/CategoryId.h"
#include <string>

namespace tbp
{
namespace log
{

class Category
{
public:
   Category(std::string label, Level initialLevel) : m_label(std::move(label)), m_initialLevel(initialLevel) {}
   //
   const std::string& GetLabel() const { return m_label; }
   Level GetInitialLevel() const { return m_initialLevel; }

private:
   std::string m_label;
   const Level m_initialLevel;

};

}
}

