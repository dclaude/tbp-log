#include "tbp/log/Categories.h"
#include "tbp/common/ConfigurationException.h"
#include <sstream>

namespace tbp
{
namespace log
{

CategoryId Categories::AddCategory(const Category& category)
{
   const auto& label = category.GetLabel();
   auto iter = m_categories.find(label);
   if (iter != m_categories.end())
   {
      std::ostringstream oss;
      oss << "Categories::AddCategory label[" << label << "] already defined";
      throw common::ConfigurationException(oss.str());
   }
   CategoryId id = m_categories.size() + 1;
   CategoryData data(category, id);
   m_categories.emplace(label, data);
   return id;
}

const Category* Categories::FindCategory(const std::string& categoryLabel) const
{
   auto iter = m_categories.find(categoryLabel);
   return iter == m_categories.end() ? nullptr : &iter->second.m_category;
}

CategoryId Categories::FindCategoryId(const Category& category) const
{
   auto iter = m_categories.find(category.GetLabel());
   return iter == m_categories.end() ? 0 : iter->second.m_id;
}

}
}

