#pragma once

#include "main.hpp"
#include "SimpleTypes.hpp"

#include <map>

namespace rw
{

template<class T, class Allocator = std::allocator<T>>
class RecyclingList
{
public:
	RecyclingList() = default;
	~RecyclingList();

	RecyclingList(const RecyclingList& begin) noexcept;

	RecyclingList(RecyclingList&& other) noexcept : RecyclingList() { swap(*this, other); }

	RecyclingList& operator=(RecyclingList other) noexcept
	{
		swap(*this, other);
		return *this;
	}

	RecyclingList& operator=(RecyclingList&& other) noexcept
	{
		swap(*this, other);
		return *this;
	}

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
		for_each_range([action](uint32_t begin, uint32_t end) { for (; begin < end; ++begin) action(Index(begin)); });
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
		swap(value.count, other.count);
		swap(value.capacity, other.capacity);
		swap(value.allocator, other.allocator);
		swap(value.ranges, other.ranges);
	}

	friend BinaryWriter& operator<<(BinaryWriter& writer, const RecyclingList& list)
	{
		list.write(writer);
		return writer;
	}

	friend BinaryReader& operator>>(BinaryReader& reader, RecyclingList& list)
	{
		list.read(reader);
		return reader;
	}

private:
	/**
	 * Performs an action on each valid range of indices, passed in as begin and end parameters.
	 */
	template<class Action>
	void for_each_range(Action action) const;

	void write(BinaryWriter& writer) const;
	void read(BinaryReader& reader);

	T* items = nullptr;
	size_t count = 0;
	size_t capacity = 0;
	Allocator allocator;

	/**
	 * Contains all ranges from which the items are empty as pairs of uint32_t.
	 * The first uint32_t indicates the end of the range (exclusive).
	 * The second uint32_t indicates the beginning of the range (inclusive).
	 */
	std::map<uint32_t, uint32_t> ranges;
};

template<class T, class Allocator>
RecyclingList<T, Allocator>::RecyclingList(const RecyclingList& other) noexcept :
	count(other.count), capacity(other.capacity), allocator(other.allocator), ranges(other.ranges)
{
	items = allocator.allocate(capacity);
	T* source = other.items;
	T* destination = items;

	auto copy_range = [source, destination](uint32_t begin, uint32_t end)
	{
		std::uninitialized_copy_n(source + begin, end - begin, destination + begin);
	};

	other.for_each_range(copy_range);
}

template<class T, class Allocator>
RecyclingList<T, Allocator>::~RecyclingList()
{
	if (capacity == 0) return;

	if constexpr (not std::is_trivially_destructible_v<T>)
	{
		for_each([](T& item) { std::destroy_at(&item); });
	}

	allocator.deallocate(items, capacity);
}

template<class T, class Allocator>
bool RecyclingList<T, Allocator>::contains(Index index) const
{
	if (index >= capacity) return false;

	auto iterator = ranges.upper_bound(index);
	if (iterator == ranges.end()) return true;

	uint32_t begin = iterator->second;
	uint32_t end = iterator->first;
	return !(begin <= index && index < end);
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
	size_t new_capacity = std::max(capacity * 2, 8LLu);
	while (new_capacity < threshold) new_capacity *= 2;
	T* new_items = allocator.allocate(new_capacity);

	//Move all valid items
	if (capacity > 0)
	{
		auto move_items = [new_items, this](uint32_t begin, uint32_t end)
		{
			T* pointer = items + begin;
			size_t length = end - begin;
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
	size_t current = 0;

	if (not ranges.empty())
	{
		auto iterator = ranges.begin();
		assert(iterator != ranges.end());

		if (iterator->second > 0) action(current, iterator->second);

		current = iterator->first;
		++iterator;

		while (iterator != ranges.end())
		{
			std::pair<uint32_t, uint32_t> range = *iterator;
			assert(current != range.second);
			action(current, range.second);

			current = range.first;
			++iterator;
		}
	}

	if (current != capacity) action(current, capacity);
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::write(BinaryWriter& writer) const
{
	size_t current = 0;
	writer << capacity;

	auto write_range = [&writer, &current, this](size_t begin, size_t end)
	{
		assert(begin >= current);
		writer << static_cast<uint32_t>(begin - current);
		writer << static_cast<uint32_t>(end - begin);
		current = end;

		for (; begin < end; ++begin) writer << items[begin];
	};

	for_each_range(write_range);
	writer << static_cast<uint32_t>(capacity - current);
}

template<class T, class Allocator>
void RecyclingList<T, Allocator>::read(BinaryReader& reader)
{
	assert(items == nullptr);
	assert(count == 0);
	assert(capacity == 0);

	size_t current = 0;
	reader >> capacity;
	if (capacity > 0) items = allocator.allocate(capacity);

	while (true)
	{
		uint32_t range_size;
		reader >> range_size;

		if (range_size == 0) assert(current == 0 || current == capacity);
		else ranges.emplace(current + range_size, current);

		current += range_size;
		if (current == capacity) break;

		uint32_t new_count;
		reader >> new_count;
		count += new_count;

		uint32_t end = current + new_count;

		for (; current < end; ++current)
		{
			T* pointer = items + current;
			std::construct_at(pointer);
			reader >> *pointer;
		}
	}
}

}
