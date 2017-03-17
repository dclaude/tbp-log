#pragma once

namespace tbp
{
   namespace tools
   {
      class Config;
      class AffinityManager;
   }
}

namespace tbp
{
namespace log
{
namespace test
{

class Context
{
public:
   Context(const tools::Config& toolsConfig, const tools::AffinityManager& affinities);
   ~Context();
   //
   static Context& Get() { return *g_context; }
   const tools::Config& GetToolsConfig() const { return m_toolsConfig; }
   const tools::AffinityManager& GetAffinityManager() const { return m_affinities; }

private:
   static Context* g_context;
   //
   const tools::Config& m_toolsConfig;
   const tools::AffinityManager& m_affinities;

};

}
}
}

