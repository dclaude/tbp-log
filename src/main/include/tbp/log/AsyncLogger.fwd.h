#pragma once

#include "tbp/common/OS.h"
#include <memory>

namespace tbp
{
namespace log
{

template <typename SpscQueue, typename Allocator>
struct AsyncLoggerAddMsg
{
   std::unique_ptr<SpscQueue> m_queue;
   common::ThreadId m_tid = 0;
   std::unique_ptr<Allocator> m_allocator;
};

template <typename SpscQueue>
struct AsyncLoggerRemoveMsg
{
   SpscQueue* m_queue = nullptr;
};

}
}

