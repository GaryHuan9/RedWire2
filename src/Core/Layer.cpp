#include "Core/Layer.hpp"
#include "Core/WireData.hpp"

#include <SFML/Graphics.hpp>
#include <random>

namespace rw
{

const static std::array<Int2, 4> four_directions = { Int2(1, 0), Int2(-1, 0), Int2(0, 1), Int2(0, -1) };

Layer::Layer() = default;
Layer::~Layer() = default;

void Layer::insert(Int2 position, TileType type)
{
	if (get(position).type == type) return;

	if (type == TileType::Wire)
	{
		static std::uniform_int_distribution distribution(std::numeric_limits<uint32_t>::min());
		static std::default_random_engine random(42);

		auto iterator = std::find_if(four_directions.begin(), four_directions.end(), [&](Int2 direction)
		{
			return get(position + direction).type == TileType::Wire;
		});

		if (iterator == four_directions.end())
		{
			uint32_t index = wires.size();

			uint32_t color = distribution(random);
			wires.emplace_back(color | 0xFFu);
			set(position, TileTag(TileType::Wire, index));
		}
		else
		{
			auto lambda = [this](Int2 position, uint32_t index)
			{
				std::vector<Int2> stack{ position };

				do
				{
					Int2 current = stack.back();
					stack.pop_back();

					TileTag tile = get(current);
					if (tile.type != TileType::Wire || tile.index == index) continue;

					set(current, TileTag(TileType::Wire, index));

					for (Int2 direction : four_directions) stack.push_back(current + direction);
				}
				while (!stack.empty());
			};

			uint32_t index = get(position + *iterator).index;
			set(position, TileTag(TileType::Wire, index + 1));
			lambda(position, index);
		}
	}
	else if (type == TileType::None && get(position).type == TileType::Wire)
	{
		set(position, {});


	}
	else set(position, TileTag(type, 0));
}

void Layer::erase(Int2 position)
{
	set(position, TileTag());
}

void Layer::draw(std::vector<sf::Vertex>& vertices, Float2 min, Float2 max, Float2 scale, Float2 origin) const
{
	Int2 chunk_min = Chunk::get_chunk_position(Float2::floor(min));
	Int2 chunk_max = Chunk::get_chunk_position(Float2::ceil(max));

	Int2 search_area = chunk_max - chunk_min;

	auto drawer = [&](Int2 chunk_position, const Chunk& chunk)
	{
		Float2 offset(chunk_position * Chunk::Size);
		chunk.draw(vertices, *this, scale, offset * scale + origin);
	};

	if (chunks.size() < search_area.x * search_area.y)
	{
		for (const auto& pair : chunks)
		{
			Int2 chunk_position = pair.first;
			if (chunk_min.x <= chunk_position.x && chunk_position.x <= chunk_max.x &&
			    chunk_min.y <= chunk_position.y && chunk_position.y <= chunk_max.y)
				drawer(chunk_position, pair.second);
		}
	}
	else
	{
		for (int32_t y = chunk_min.y; y <= chunk_max.y; ++y)
		{
			for (int32_t x = chunk_min.x; x <= chunk_max.x; ++x)
			{
				Int2 chunk_position(x, y);
				auto iterator = chunks.find(chunk_position);
				if (iterator == chunks.end()) continue;
				drawer(chunk_position, iterator->second);
			}
		}
	}
}

TileTag Layer::get(Int2 position)
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);
	if (iterator == chunks.end()) return {};
	return iterator->second.get(position);
}

void Layer::set(Int2 position, TileTag tile)
{
	Int2 chunk_position = Chunk::get_chunk_position(position);
	auto iterator = chunks.find(chunk_position);

	if (iterator == chunks.end())
	{
		if (tile.type == TileType::None) return;
		auto pair = chunks.emplace(chunk_position, Chunk());

		assert(pair.second);
		iterator = pair.first;
	}

	iterator->second.set(position, tile);
	if (iterator->second.empty()) chunks.erase(iterator);
}

}
