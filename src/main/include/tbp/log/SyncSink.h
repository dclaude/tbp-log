#pragma once

#include "tbp/log/Level.h"
#include "tbp/log/FileWriter.h"
#include "tbp/common/OS.h"
#include "tbp/common/Definitions.h"
#include "tbp/common/Compiler.h"
#include <time.h>
#include <fstream>
#include <ctime>

namespace tbp
{
namespace log
{

class Config;
class SignalManager;

// log synchronously into one file per thread
class SyncSink
{
public:
   SyncSink(const Config& config, common::ThreadId tid, SignalManager* signals);
   MOCK_NPERF_VIRTUAL ~SyncSink() {}
   SyncSink(SyncSink&&) = default;
   SyncSink& operator=(SyncSink&&) = default;
   //
   template <typename... Args> void Log(const Category& category, Level level, const timespec& now, common::SigNum signal, const char* fmt, Args&&... args);

MOCK_PROTECTED:
   MOCK_NPERF_VIRTUAL void OnWrite(const fmt::MemoryWriter& /*writer*/) const {}
   MOCK_VIRTUAL void ExitWithDefaultSignalHandler(common::SigNum signalNumber);

private:
   //
   FileWriter m_fileWriter;
   common::ThreadId m_tid;
   SignalManager* m_signals = nullptr;

};

template <typename... Args> 
inline void SyncSink::Log(const Category& category, Level level, const timespec& now, common::SigNum signal, const char* fmt, Args&&... args)
{
   m_fileWriter.WriteHeader(now, m_tid, level, category);
   auto& writer = m_fileWriter.GetWriter();
   writer.write(fmt, std::forward<Args>(args)...);
   //
   OnWrite(writer);
   m_fileWriter.WriteToFile();
   if (unlikely(signal))
   {
      ExitWithDefaultSignalHandler(signal);
   }
}

}
}

