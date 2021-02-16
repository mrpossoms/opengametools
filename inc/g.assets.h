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
struct kind
{
	time_t last_accessed;
	T asset;

	kind() = default;

	kind(time_t last, T a) 
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
	private: std::unordered_map<std::string, kind<g::gfx::texture>> textures;
	private: std::unordered_map<std::string, kind<const ogt_vox_scene*>> voxels;

	public: store(const std::string& root_path="data") : root(root_path) { }

	g::gfx::texture& tex(const std::string& partial_path)
	{
		auto itr = textures.find(partial_path);
		if (itr == textures.end())
		{
			auto chain = g::gfx::texture_factory{}.from_png(root + "/tex/" + partial_path).pixelated();
			// do spicy chain thing with processors here
			auto tex = chain.create();
			textures[partial_path] = { time(nullptr), tex };
		}
		
		return textures[partial_path].get();
	}

	const ogt_vox_scene* vox(const std::string& partial_path)
	{
		auto itr = voxels.find(partial_path);
		if (itr == voxels.end())
		{
			std::string filename = root + "/vox/" + partial_path;
		    // open the file
#if defined(_MSC_VER) && _MSC_VER >= 1400
	    	FILE * fp;
	    	if (0 != fopen_s(&fp, filename.c_str(), "rb")) { fp = nullptr; }
#else
	    	FILE * fp = fopen(filename.c_str(), "rb");
#endif
	    	if (!fp) { return nullptr; }

		    // get the buffer size which matches the size of the file
		    fseek(fp, 0, SEEK_END);
		    uint32_t buffer_size = ftell(fp);
		    fseek(fp, 0, SEEK_SET);

		    // load the file into a memory buffer
		    uint8_t * buffer = new uint8_t[buffer_size];
		    fread(buffer, buffer_size, 1, fp);
		    fclose(fp);

		    // construct the scene from the buffer
		    const ogt_vox_scene* scene = ogt_vox_read_scene_with_flags(buffer, buffer_size, 0);
		    voxels[partial_path] = { time(nullptr), scene };

		    // the buffer can be safely deleted once the scene is instantiated.
		    delete[] buffer;
		}

	    return voxels[partial_path].get();	
	}
};

}

}