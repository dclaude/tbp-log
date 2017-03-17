#pragma once

#include "tbp/log/CategoryId.h"
#include "tbp/log/ThreadLocalLogger.h"
#include "tbp/common/Backtrace.h"
#include "tbp/common/OS.h"
#include <map>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>

namespace tbp
{
namespace log
{

// from https://github.com/KjellKod/g3log
class SignalManager
{
public:
   SignalManager();
   ~SignalManager();
   //
   template <typename Sink> void InstallSignalHandler(CategoryId category);
   void ExitWithDefaultSignalHandler(common::SigNum signalNumber, bool doNotKill);

private:
   void Reset();
   void RestoreSignalHandler(common::SigNum signalNumber);
   std::string GetExitReason(common::SigNum signalNumber);
   template <typename Sink> static void SignalHandler(common::SigNum sigNum, siginfo_t*, void*) { m_global->OnSignal<Sink>(sigNum); }
   template <typename Sink> void OnSignal(common::SigNum signalNumber);
   //
   static SignalManager* m_global;
   CategoryId m_category = 0;
   std::map<common::SigNum, std::string> m_signals;
   std::atomic<std::size_t> m_signalCounter;
   std::atomic<bool> m_logged;

};

template <typename Sink>
void SignalManager::InstallSignalHandler(CategoryId category)
{
   Reset();
   m_global = this; // global variable needed for signal handler
   m_category = category;
   //
   struct sigaction action;
   memset(&action, 0, sizeof(action));
   sigemptyset(&action.sa_mask);
   action.sa_flags = SA_SIGINFO; // sigaction to use sa_sigaction
   action.sa_sigaction = &SignalManager::SignalHandler<Sink>;
   // do it verbose style - install all signal actions
   for (const auto& p : m_signals)
   {
      if (sigaction(p.first, &action, nullptr) < 0)
      {
         std::string error = "sigaction - " + p.second;
         perror(error.c_str());
      }
   }
}

template <typename Sink>
void SignalManager::OnSignal(common::SigNum signalNumber)
{
   using LocalLogger = ThreadLocalLogger<Sink>;
   // if more than once signal is received, only the first signal will "not" enter the infinite loop below
   if (m_signalCounter.fetch_add(1, std::memory_order_relaxed) != 0) // read-write-modify needed
   {
      while (true)
      {
         std::this_thread::sleep_for(std::chrono::seconds(1));
      }
   }
   //
   std::vector<std::string> stack = common::Backtrace(50);
   std::ostringstream oss;
   // dump stack: skip first frame, since that is here
   for (std::size_t frame = 1; frame < stack.size(); ++frame)
   {
      oss << "\t" << stack[frame] << std::endl;
   }
   std::string fatalReason = GetExitReason(signalNumber);
   //
   auto logger = LocalLogger::Get();
   logger->Log(m_category, Level::critical, signalNumber, "Received fatal signal[{}] signalNumber[{}]\n{}", 
         fatalReason, signalNumber, oss.str());
   //
   // wait to die
   while (!m_logged.load())
   {
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }
}

}
}

