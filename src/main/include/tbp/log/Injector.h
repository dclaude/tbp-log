#pragma once

#include "tbp/common/OS.h"
#include "tbp/common/Definitions.h"
#include "tbp/common/TypeTraits.h"
#include <memory>
#include <string>

namespace tbp
{
   namespace log
   {
      class Config;
      class FileWriter;
      class SyncSink;
   }
}

namespace tbp
{
namespace log
{

class Injector
{
public:
   MOCK_VIRTUAL ~Injector();
   //
   MOCK_VIRTUAL std::unique_ptr<FileWriter> CreateFileWriter(const log::Config& config, common::ThreadId tid) const;
   /*
   the method below is enabled, if Sink is "not" derived from a class listed "after" the first template parameter of is_derived_of_any (log::SyncSink, log::SyncSink1, ...)
   this is needed to have an OnNewLoggerSink() implementation for the user-defined Sink types (for instance AsyncSink is template on the TypeId which can be user-defined, ...)
   */
   template <typename Sink> typename std::enable_if<common::static_not<common::is_derived_of_any<Sink, SyncSink /*, SyncSink1, ... */>>::value, void>::type OnNewLoggerSink(Sink& /*sink*/) const {}
   MOCK_VIRTUAL void OnNewLoggerSink(SyncSink& /*sink*/) const {}
   //
   // tools::Injector must only be used for types than can be mock in unittests

};

}
}

