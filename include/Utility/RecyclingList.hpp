#pragma once

#include "main.hpp"
#include <map>

namespace rw
{

template<class T, class Allocator = std::allocator<T>>
class RecyclingList
{
public:
	RecyclingList() = default;

	~RecyclingList() { destruct(); }

	RecyclingList(RecyclingList&& other) noexcept : RecyclingList()
	{
		swap(*this, other);
	}

	RecyclingList& operator=(RecyclingList&& other) noexcept
	{
		destruct();

		items = other.items;
		capacity = other.capacity;
		other.capacity = 0;

		allocator = std::move(other.allocator);
		ranges = std::move(other.ranges);
		return *this;
	}

	RecyclingList(const RecyclingList&) = delete;
	RecyclingList& operator=(const RecyclingList&) = delete;

	[[nodiscard]] const T& operator[](size_t index) const
	{
		assert(contains(index));
		return items[index];
	}

	T& operator[](size_t index)
	{
		assert(contains(index));
		return items[index];
	}

	bool contains(size_t index);

	template<class Action>
	void for_each(Action action) const
	{
		for_each_index([action, this](size_t index) { action(items[index]); });
	}

	template<class Action>
	void for_each(Action action)
	{
		for_each_index([action, this](size_t index) { action(items[index]); });
	}

	template<class Action>
	void for_each_index(Action action) const;

	size_t push(const T& value)
	{
		return emplace(value);
	}

	template<class... Arguments>
	size_t emplace(Arguments&& ... arguments);

	void erase(size_t index);

	friend void swap(RecyclingList& value, RecyclingList& other) noexcept
	{
		using std::swap;

		swap(value.items, other.items);
		swap(value.capacity, other.capacity);
		swap(value.allocator, other.allocator);
		swap(value.ranges, other.ranges);
	}

private:
	void grow();
	void destruct();

	T* items = nullptr;
	size_t capacity = 0;
	Allocator allocator;
	std::map<uint32_t, uint32_t> ranges; //Maps from the end of a range (exclusive) to the start of a range (inclusive)
};

template<class T, class Alloc>
bool RecyclingList<T, Alloc>::contains(size_t index)
{
	if (index >= capacity) return false;
	if (ranges.empty()) return true;

	auto iterator = ranges.upper_bound(index);
	assert(iterator != ranges.end());

	uint32_t start = iterator->second;
	uint32_t end = iterator->first;
	return index < start || end <= index;
}

template<class T, class Alloc>
template<class Action>
void RecyclingList<T, Alloc>::for_each_index(Action action) const
{
	auto iterator = ranges.begin();
	size_t current = 0;

	while (iterator != ranges.end())
	{
		std::pair<uint32_t, uint32_t> range = *iterator;
		for (; current < range.second; ++current) action(current);

		current = range.first;
		++iterator;
	}

	for (; current < capacity; ++current) action(current);
}

template<class T, class Alloc>
template<class... Arguments>
size_t RecyclingList<T, Alloc>::emplace(Arguments&& ... arguments)
{
	//Increase capacity
	if (ranges.empty()) grow();
	assert(not ranges.empty());

	//Find first empty range
	auto& range = *ranges.begin();
	size_t index = range.second++;
	if (range.first == range.second) ranges.erase(ranges.begin());

	//Construct object in place
	std::construct_at(items + index, std::forward<Arguments>(arguments)...);
	return index;
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::erase(size_t index)
{
	assert(contains(index));
	std::destroy_at(items + index);

	//Updates ranges
	using Iterator = decltype(ranges)::iterator;
	Iterator previous = ranges.end();
	Iterator next = ranges.end();

	//Finds whether to connect to next or previous ranges
	if (!ranges.empty())
	{
		auto iterator = ranges.upper_bound(index);
		bool not_end = iterator != ranges.end();
		bool not_begin = iterator != ranges.begin();

		if (not_end && iterator->second == index + 1) next = iterator;

		if (not_begin)
		{
			--iterator;
			if (iterator->first == index) previous = iterator;
		}
	}

	//Modify the ranges
	if (previous != ranges.end())
	{
		assert(previous->first == index);

		if (next != ranges.end())
		{
			assert(next->second == index + 1);
			next->second = previous->second;
			ranges.erase(previous);
		}
		else
		{
			auto node = ranges.extract(previous);
			node.key() = index + 1;
			ranges.insert(std::move(node));
		}
	}
	else if (next != ranges.end())
	{
		assert(next->second == index + 1);
		next->second = index;
	}
	else ranges.emplace(index + 1, index);
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::grow()
{
	assert(ranges.empty());

	size_t new_capacity = std::max(capacity * 2, 8LLU);
	T* new_items = allocator.allocate(new_capacity);

	if (capacity > 0)
	{
		std::uninitialized_move_n(items, capacity, new_items);
		allocator.deallocate(items, capacity);
	}

	ranges.emplace(new_capacity, capacity);

	items = new_items;
	capacity = new_capacity;
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::destruct()
{
	if (capacity == 0) return;

	if constexpr (not std::is_trivially_destructible_v<T>)
	{
		for_each([](T& item) { std::destroy_at(&item); });
	}

	allocator.deallocate(items, capacity);
}

}
