#include "gtest/gtest.h"
#include "Utility/RecyclingList.hpp"

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
			ASSERT_EQ(list.contains(i), !reference[i].empty());
			if (reference[i].empty()) continue;

			++count;
			ASSERT_EQ(list[i], reference[i]);
		}

		ASSERT_EQ(list.size(), count);

		size_t current = 0;

		list.for_each_index([&](size_t index)
		                    {
			                    while (current < reference.size() && reference[current].empty()) ++current;
			                    ASSERT_EQ(reference[current], list[current]);
			                    ASSERT_EQ(current++, index);
		                    });

		while (current < reference.size() && reference[current].empty()) ++current;
		ASSERT_EQ(current, reference.size());
	}

	void emplace(const std::string& value)
	{
		assert(!value.empty());

		auto iterator = std::find(reference.begin(), reference.end(), "");
		size_t index = std::distance(reference.begin(), iterator);

		if (iterator == reference.end()) reference.push_back(value);
		else reference[index] = value;

		ASSERT_EQ(list.emplace(value), index);
		assert_contents();
	}

	void erase(size_t index)
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
	erase(0);
	emplace("new");
	erase(1);
	erase(0);
}

TEST_F(RecyclingListTests, Loop)
{
	for (size_t i = 0; i < 10; ++i) emplace(std::to_string(i));

	for (size_t i : { 1, 7, 3, 0, 5, 2, 8, 6, 9, 4 }) erase(i);

	for (size_t i = 0; i < 16; ++i) emplace(std::to_string(i));

	for (size_t i = 9; i < 13; ++i) erase(i);

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
		size_t index = distribution(random);
		if (list.contains(index)) erase(index);
	}
}
