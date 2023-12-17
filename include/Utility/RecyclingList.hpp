#pragma once

#include "main.hpp"
#include "Types.hpp"
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
		count = other.count;
		capacity = other.capacity;
		other.capacity = 0;
		other.count = 0;

		allocator = std::move(other.allocator);
		ranges = std::move(other.ranges);
		return *this;
	}

	RecyclingList(const RecyclingList&) = delete;
	RecyclingList& operator=(const RecyclingList&) = delete;

	[[nodiscard]] size_t size() const { return count; }

	[[nodiscard]] const T& operator[](Index index) const
	{
		assert(contains(index));
		return items[index];
	}

	T& operator[](Index index)
	{
		assert(contains(index));
		return items[index];
	}

	[[nodiscard]] bool contains(Index index) const;

	template<class Action>
	void for_each(Action action) const
	{
		for_each_index([action, this](Index index) { action(items[index]); });
	}

	template<class Action>
	void for_each(Action action)
	{
		for_each_index([action, this](Index index) { action(items[index]); });
	}

	template<class Action>
	void for_each_index(Action action) const
	{
		for_each_range([action](uint32_t start, uint32_t end) { for (; start < end; ++start) action(Index(start)); });
	}

	Index push(const T& value)
	{
		return emplace(value);
	}

	template<class... Arguments>
	Index emplace(Arguments&& ... arguments);

	void erase(Index index);

	void reserve(size_t threshold);

	friend void swap(RecyclingList& value, RecyclingList& other) noexcept
	{
		using std::swap;

		swap(value.items, other.items);
		swap(value.capacity, other.capacity);
		swap(value.allocator, other.allocator);
		swap(value.ranges, other.ranges);
	}

private:
	/**
	 * Performs an action on each valid range of indices, passed in as start and end parameters.
	 */
	template<class Action>
	void for_each_range(Action action) const;

	void destruct();

	T* items = nullptr;
	size_t count = 0;
	size_t capacity = 0;
	Allocator allocator;
	std::map<uint32_t, uint32_t> ranges; //Maps from the end of a range (exclusive) to the start of a range (inclusive)
};

template<class T, class Allocator>
bool RecyclingList<T, Allocator>::contains(Index index) const
{
	if (index >= capacity) return false;

	auto iterator = ranges.upper_bound(index);
	if (iterator == ranges.end()) return true;

	uint32_t start = iterator->second;
	uint32_t end = iterator->first;
	return !(start <= index && index < end);
}

template<class T, class Allocator>
template<class... Arguments>
Index RecyclingList<T, Allocator>::emplace(Arguments&& ... arguments)
{
	//Ensure capacity
	++count;
	if (ranges.empty()) reserve(count);
	assert(not ranges.empty());

	//Find first empty range
	auto& range = *ranges.begin();
	size_t index = range.second++;
	if (range.first == range.second) ranges.erase(ranges.begin());

	//Construct object in place
	std::construct_at(items + index, std::forward<Arguments>(arguments)...);
	return Index(index);
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::erase(Index index)
{
	assert(contains(index));
	std::destroy_at(items + index);
	--count;

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
			iterator = std::prev(iterator);
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
void RecyclingList<T, Allocator>::reserve(size_t threshold)
{
	if (threshold <= capacity) return;

	//Calculate new capacity
	size_t new_capacity = std::max(capacity * 2, 8LLU);
	while (new_capacity < threshold) new_capacity *= 2;
	T* new_items = allocator.allocate(new_capacity);

	//Move all valid items
	if (capacity > 0)
	{
		auto move_items = [new_items, this](uint32_t start, uint32_t end)
		{
			T* pointer = items + start;
			size_t length = end - start;
			std::uninitialized_move_n(pointer, length, new_items);
		};

		for_each_range(move_items);
		allocator.deallocate(items, capacity);
	}

	size_t old_capacity = capacity;
	capacity = new_capacity;
	items = new_items;

	//Update ranges
	if (not ranges.empty())
	{
		auto iterator = std::prev(ranges.end());

		if (iterator->first == old_capacity)
		{
			auto node = ranges.extract(iterator);
			node.key() = capacity;
			ranges.insert(std::move(node));
			return;
		}
	}

	ranges.emplace(capacity, old_capacity);
}

template<class T, class Allocator>
template<class Action>
void RecyclingList<T, Allocator>::for_each_range(Action action) const
{
	auto iterator = ranges.begin();
	size_t current = 0;

	while (iterator != ranges.end())
	{
		std::pair<uint32_t, uint32_t> range = *iterator;
		action(current, range.second);

		current = range.first;
		++iterator;
	}

	action(current, capacity);
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
