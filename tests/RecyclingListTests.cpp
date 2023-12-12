#include "gtest/gtest.h"
#include "Utility/RecyclingList.hpp"

#include <string>

using namespace rw;

TEST(RecyclingListTests, SimpleTest)
{
	RecyclingList<std::string> list;
	ASSERT_FALSE(list.contains(0));
	ASSERT_FALSE(list.contains(1));

	ASSERT_EQ(list.emplace("hello"), 0);
	ASSERT_TRUE(list.contains(0));
	ASSERT_FALSE(list.contains(1));

	ASSERT_EQ(list.emplace("world"), 1);
	ASSERT_TRUE(list.contains(0));
	ASSERT_TRUE(list.contains(1));

	list.erase(0);
	ASSERT_FALSE(list.contains(0));
	ASSERT_TRUE(list.contains(1));

	ASSERT_EQ(list.emplace("new"), 0);
	ASSERT_TRUE(list.contains(0));
	ASSERT_TRUE(list.contains(1));

	list.erase(1);
	list.erase(0);
	ASSERT_FALSE(list.contains(0));
	ASSERT_FALSE(list.contains(1));
}

TEST(RecyclingListTests, LoopTest)
{
	RecyclingList<std::string> list;

	for (size_t i = 0; i < 10; ++i)
	{
		ASSERT_EQ(list.emplace(std::to_string(i)), i);
	}

	for (size_t i : { 1, 7, 3, 0, 5, 2, 8, 6, 9, 4 })
	{
		list.erase(i);
		list.for_each([](auto& item) { std::cout << item << ' '; });
		std::cout << std::endl;
	}

	for (size_t i = 0; i < 16; ++i)
	{
		ASSERT_EQ(list.emplace(std::to_string(i)), i);
	}

	list.for_each([](auto& item) { std::cout << item << ' '; });
	std::cout << std::endl;
}
