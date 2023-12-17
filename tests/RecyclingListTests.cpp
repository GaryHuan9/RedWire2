#include "Utility/RecyclingList.hpp"

#include "gtest/gtest.h"

#include <string>
#include <random>
#include <unordered_set>

using namespace rw;

class RecyclingListTests : public testing::Test
{
protected:
	void SetUp() override
	{
		Test::SetUp();
		assert_contents();
	}

	void assert_contents() const
	{
		size_t count = 0;

		for (size_t i = 0; i < reference.size(); ++i)
		{
			Index index(i);
			ASSERT_EQ(list.contains(index), !reference[index].empty());
			if (reference[index].empty()) continue;

			++count;
			ASSERT_EQ(list[index], reference[index]);
		}

		ASSERT_EQ(list.size(), count);

		size_t current = 0;

		auto assert_index = [&](Index index)
		{
			while (current < reference.size() && reference[current].empty()) ++current;

			ASSERT_EQ(current, index);
			ASSERT_EQ(reference[current], list[index]);
			++current;
		};

		list.for_each_index(assert_index);

		while (current < reference.size() && reference[current].empty()) ++current;
		ASSERT_EQ(current, reference.size());
	}

	void emplace(const std::string& value)
	{
		assert(!value.empty());

		auto iterator = std::find(reference.begin(), reference.end(), "");
		Index index(std::distance(reference.begin(), iterator));

		if (iterator == reference.end()) reference.push_back(value);
		else reference[index] = value;

		ASSERT_EQ(list.emplace(value), index);
		assert_contents();
	}

	void erase(Index index)
	{
		assert(!reference[index].empty());
		ASSERT_TRUE(list.contains(index));

		reference[index] = "";
		list.erase(index);

		assert_contents();
	}

	std::vector<std::string> reference;
	RecyclingList<std::string> list;
};

TEST_F(RecyclingListTests, Simple)
{
	emplace("hello");
	emplace("world");
	erase(Index(0));
	emplace("new");
	erase(Index(1));
	erase(Index(0));
}

TEST_F(RecyclingListTests, Loop)
{
	for (size_t i = 0; i < 10; ++i) emplace(std::to_string(i));

	for (size_t i : { 1, 7, 3, 0, 5, 2, 8, 6, 9, 4 }) erase(Index(i));

	for (size_t i = 0; i < 16; ++i) emplace(std::to_string(i));

	for (size_t i = 9; i < 13; ++i) erase(Index(i));

	for (size_t i = 0; i < 7; ++i) emplace(std::to_string(i));
}

TEST_F(RecyclingListTests, Random)
{
	std::mt19937 random(42);

	for (size_t i = 0; i < 3000; ++i)
	{
		emplace(std::to_string(i));
		if (i % 3 != 0) continue;

		std::uniform_int_distribution<size_t> distribution(0, list.size() - 1);
		Index index(distribution(random));
		if (list.contains(index)) erase(index);
	}
}
