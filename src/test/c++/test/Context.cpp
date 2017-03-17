#include "test/Context.h"

namespace tbp
{
namespace log
{
namespace test
{

Context* Context::g_context = nullptr;

Context::Context(const tools::Config& toolsConfig, const tools::AffinityManager& affinities) : m_toolsConfig(toolsConfig), m_affinities(affinities)
{
   g_context = this;
}

Context::~Context()
{
   g_context = nullptr;
}

}
}
}


