#include "tbp/log/Decoder.h"
#include "tbp/log/DefaultTypes.h"
#include "tbp/log/Encoder.h"
#include "tbp/log/BufferAllocator.h"
#include "tbp/tools/spsc/Queue1.h"
#include "test/UserDefinedLoggable.h"
#include <gtest/gtest.h>
#include <string>

using tbp::log::test::UserDefinedLoggable;
using std::string;

namespace tbp
{
namespace log
{

namespace
{

using Allocator = BufferAllocator<tools::spsc::Queue1<char*>>;

}

TEST(EncoderTest, DefaultTypeId)
{
   struct S
   {
      string m_s = "test14";
      double m_d = 2.;
      int m_i = 1;
   };
   S s;
   Encoder<DefaultTypeId, Allocator> e;
   Allocator allocator({ 64 }, 10);
   Buffer<Allocator> b = e.Encode(allocator, s.m_i, s.m_d, s.m_s);
   b.Reset();
   Decoder<DefaultTypeId, Allocator> d1(b);
   std::size_t nbFields = 0;
   while (d1.HasNext())
   {
      auto p = d1.Next();
      auto& buffer = *p.second;
      switch (p.first)
      {
         case DefaultTypeId::INT:
            {
               int v = 0;
               Type<int, DefaultTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v, s.m_i);
               ++nbFields;
            }
            break;
         case DefaultTypeId::DOUBLE:
            {
               double v = 0.;
               Type<double, DefaultTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v, s.m_d);
               ++nbFields;
            }
            break;
         case DefaultTypeId::STRING:
            {
               string v;
               Type<string, DefaultTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v, s.m_s);
               ++nbFields;
            }
            break;
         case DefaultTypeId::UINT64:
         case DefaultTypeId::INT64:
         case DefaultTypeId::NONE:
            {
               assert(false);
            }
            break;
      }
   };
   EXPECT_EQ(d1.HasNext(), false);
   EXPECT_EQ(nbFields, 3U);
   b.Recycle(allocator);
}

namespace
{

/*
api user must provide this enum if he wants to use user-defined types
the user is not forced to use all the enumerate of DefaultTypeId
for instance here STRING is not in the enum
*/
enum class MyTypeId : std::uint8_t
{
   NONE,
   INT,
   DOUBLE,
   USER_DEFINED_LOGGABLE,
};

}

/*
api user must provide this class for each user-defined type
template specialization must be in namespace tbp::tools::log
*/
template <typename Allocator>
struct Type<UserDefinedLoggable, MyTypeId, Allocator>
{
   static MyTypeId Id() { return MyTypeId::USER_DEFINED_LOGGABLE; }
   static constexpr std::size_t Sizeof(const UserDefinedLoggable& v) { return sizeof(v); }
   static void Encode(Buffer<Allocator>& buffer, const UserDefinedLoggable& v)
   {
      v.Encode(buffer);
   }
   static void Decode(Buffer<Allocator>& buffer, UserDefinedLoggable& v)
   {
      v.Decode(buffer);
   }
};

TEST(EncoderTest, MyTypeId)
{
   struct S
   {
      S() : m_u(2012, 12, 9) {}
      UserDefinedLoggable m_u;
      double m_d = 3.;
      int m_i = 1;
   };
   S s;
   Encoder<MyTypeId, Allocator> e;
   Allocator allocator({ 64 }, 10);
   Buffer<Allocator> b = e.Encode(allocator, s.m_i, s.m_d, s.m_u);
   b.Reset();
   Decoder<MyTypeId, Allocator> d2(b);
   std::size_t nbFields = 0;
   while (d2.HasNext())
   {
      auto p = d2.Next();
      auto& buffer = *p.second;
      switch (p.first)
      {
         case MyTypeId::INT:
            {
               int v = 0;
               Type<int, MyTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v, s.m_i);
               ++nbFields;
            }
            break;
         case MyTypeId::DOUBLE:
            {
               double v = 0.;
               Type<double, MyTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v, s.m_d);
               ++nbFields;
            }
            break;
         case MyTypeId::USER_DEFINED_LOGGABLE:
            {
               UserDefinedLoggable v;
               Type<UserDefinedLoggable, MyTypeId, Allocator>::Decode(buffer, v);
               EXPECT_EQ(v.Equals(s.m_u), true);
               ++nbFields;
            }
            break;
         case MyTypeId::NONE:
            {
               assert(false);
            }
            break;
     }
   };
   EXPECT_EQ(d2.HasNext(), false);
   EXPECT_EQ(nbFields, 3);
   b.Recycle(allocator);
}

}
}

