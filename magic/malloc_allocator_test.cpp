#include "magic/malloc_allocator.hpp"

#include <deque>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace principia {
namespace magic {

// AllocatorIs<container, alloc>() returns true iff container's allocator is
// alloc.
template <typename T, typename Allocator>
constexpr bool AllocatorIs() {
  return std::is_same<typename T::allocator_type, Allocator>::value;
}

// Test that the default allocators for various classes are overridden.
TEST(PrincipiaMallocAllocatorTest, DefaultAllocators) {
  // STL
  EXPECT_TRUE((AllocatorIs<std::vector<int>, MallocAllocator<int>>()));
  EXPECT_TRUE((AllocatorIs<std::deque<int>, MallocAllocator<int>>()));
  EXPECT_TRUE((AllocatorIs<std::list<int>, MallocAllocator<int>>()));
  EXPECT_TRUE((AllocatorIs<std::set<int>, MallocAllocator<int>>()));
  EXPECT_TRUE((AllocatorIs<std::map<int, int>,
                           MallocAllocator<std::pair<const int, int>>>()));
  EXPECT_TRUE((AllocatorIs<std::multiset<int>, MallocAllocator<int>>()));
  EXPECT_TRUE((AllocatorIs<std::multimap<int, int>,
                           MallocAllocator<std::pair<const int, int>>>()));
}

}  // namespace magic
}  // namespace principia
