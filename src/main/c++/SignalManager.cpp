#include "tbp/log/SignalManager.h"
#include <unistd.h>

namespace tbp
{
namespace log
{

SignalManager* SignalManager::m_global = nullptr;

SignalManager::SignalManager() : m_signalCounter(0), m_logged(false)
{
   m_signals = {
      { SIGABRT, "SIGABRT" },
      { SIGFPE, "SIGFPE" },
      { SIGILL, "SIGILL" },
      { SIGSEGV, "SIGSEGV" },
      { SIGTERM, "SIGTERM" },
   };
}

SignalManager::~SignalManager()
{
   Reset();
}

void SignalManager::Reset()
{
   if (m_global)
   {
      for (const auto& p : m_signals)
      {
         RestoreSignalHandler(p.first);
      }
      m_global = nullptr;
   }
}

std::string SignalManager::GetExitReason(common::SigNum signalNumber)
{
   auto iter = m_signals.find(signalNumber);
   if (iter != m_signals.end())
   {
      return iter->second;
   }
   std::ostringstream oss;
   oss << "unknown signalNumber[" << signalNumber << "]";
   return oss.str();
}

void SignalManager::RestoreSignalHandler(common::SigNum signalNumber)
{
   struct sigaction action;
   memset(&action, 0, sizeof(action));
   action.sa_handler = SIG_DFL; // take default action for the signal
   sigaction(signalNumber, &action, NULL);
}

void SignalManager::ExitWithDefaultSignalHandler(common::SigNum signalNumber, bool doNotKill)
{
   m_logged.store(true);
   if (!doNotKill)
   {
      RestoreSignalHandler(signalNumber);
      kill(getpid(), signalNumber);
      exit(signalNumber);
   }
}

}
}

