#pragma once

#include <time.h>
#include <unordered_map>
#include <string>

#include "g.gfx.h"

namespace g
{

namespace asset
{

template<typename T>
struct type
{
	time_t last_accessed;
	T asset;

	type(time_t last, T a) 
	{
		last_accessed = last;
		asset = a;
	} 

	T& get()
	{
		last_accessed = time(nullptr);
		return asset;
	}
};

struct store
{
	private: std::string root;
	private: std::unordered_map<std::string, g::gfx::texture> textures;

	public: store(const std::string& root_path="data") : root(root_path) { }

	g::gfx::texture& tex(const std::string& partial_path)
	{
		auto itr = textures.find(partial_path);
		if (0 == textures.count(partial_path))
		{
			auto chain = g::gfx::texture_factory{}.from_png(root + "/tex/" + partial_path).pixelated();
			// do spicy chain thing with processors here
			auto tex = chain.create();
			textures[partial_path] = tex;
		}
		
		return textures[partial_path];
	}
};

}

}