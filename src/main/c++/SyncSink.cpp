#include "tbp/log/SyncSink.h"
#include "tbp/log/Config.h"
#include "tbp/log/SignalManager.h"

using std::string;

namespace tbp
{
namespace log
{

SyncSink::SyncSink(const Config& config, common::ThreadId tid, SignalManager* signals)
   : m_fileWriter(config, tid), m_tid(tid), m_signals(signals)
{
}

void SyncSink::ExitWithDefaultSignalHandler(common::SigNum signalNumber)
{
   if (m_signals)
   {
      m_signals->ExitWithDefaultSignalHandler(signalNumber, false);
   }
}

}
}

