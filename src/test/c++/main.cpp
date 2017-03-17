#include "tbp/tools/Config.h"
#include "tbp/tools/AsyncLogger.h"
#include "tbp/tools/ScopedThread.h"
#include "tbp/tools/AffinityManager.h"
#include "test/Context.h"
#include <gmock/gmock.h>
#include <iostream>
#include <string>

namespace tbp
{
namespace log
{

int MainFunction(int argc, char** argv)
{
   testing::InitGoogleMock(&argc, argv);
   //
   if (argc != 2)
   {
      std::cerr << "wrong number of arguments" << std::endl;
      return 1;
   }
   std::string outputDir = argv[1];
   log::Config logConfig(outputDir, "log");
   tools::Config toolsConfig(logConfig);
   tools::AffinityManager affinities(toolsConfig);
   affinities.Fill();
   test::Context context(toolsConfig, affinities);
   //
   int res = 0;
   try
   {
      res = RUN_ALL_TESTS();
   }
   catch (std::exception& e)
   {
      std::cerr << "exception in main: " << e.what() << std::endl;
      res = 1;
   }
   return res;
}

}
}

int main(int argc, char** argv) 
{
   return tbp::log::MainFunction(argc, argv);
}

