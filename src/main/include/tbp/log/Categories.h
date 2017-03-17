#pragma once

#include "tbp/log/Category.h"
#include <map>

namespace tbp
{
namespace log
{

class Categories
{
public:
   CategoryId AddCategory(const Category& category);
   //
   std::size_t GetSize() const { return m_categories.size(); }
   template <typename FUNC> void ForEach(FUNC func) const;
   const Category* FindCategory(const std::string& categoryLabel) const;
   CategoryId FindCategoryId(const Category& category) const;

private:
   struct CategoryData
   {
      CategoryData(const Category& cat, CategoryId id) : m_category(cat), m_id(id) {}
      Category m_category;
      CategoryId m_id = 0;
   };
   std::map<std::string, CategoryData> m_categories;

};

template <typename FUNC>
inline void Categories::ForEach(FUNC func) const
{
   for (const auto& p : m_categories)
   {
      const CategoryData& data = p.second;
      func(data.m_id, data.m_category);
   }
}

}
}

