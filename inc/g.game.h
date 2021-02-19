#pragma once
#include "xmath.h"
#include <string.h>
#include <stddef.h>
#include <ogt_vox.h>

using namespace xmath;

namespace g {
namespace game {

struct view_point
{
	vec<3> position = {0, 0, 0}; 
	quat orientation = {0, 0, 0, 1};
};


struct camera : public view_point
{
	enum class proj_type
	{
		perspective,
		orthographic,
	};

	float field_of_view = M_PI / 2;
	float near = 0.1f, far = 1000.f;
	proj_type proj = proj_type::perspective;

	quat& d_pitch(float delta)
	{
		auto dq = quat::from_axis_angle({1, 0, 0}, delta);
		return orientation *= dq;
	}

	quat& d_yaw(float delta)
	{
		auto dq = quat::from_axis_angle({0, 1, 0}, delta);
		return orientation *= dq;
	}

	quat& d_roll(float delta)
	{
		auto dq = quat::from_axis_angle({0, 0, 1}, delta);
		return orientation *= dq;
	}

	mat<4, 4> view() const
	{
		return mat<4, 4>::translation(position) * orientation.to_matrix();
	}

	mat<4, 4> projection(float aspect) const
	{
		switch (proj)
		{
			default:
				return mat<4, 4>::perspective(near, far, field_of_view, aspect);
		}
	}
};

template<typename DAT>
struct voxels
{
	struct slice
	{
		size_t depth = 0;
		DAT* v = nullptr;

		slice(DAT* ptr, size_t d)
		{
			v = ptr;
			depth = d;
		}

		DAT* operator[](size_t idx_h)
		{
			return v + (idx_h * depth);
		}
	};

	size_t width, height, depth;
	DAT* v;

	voxels() = default;

	voxels(const DAT* ptr, size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v = new DAT[w * h * d];
		memcpy(v, ptr, sizeof(DAT) * w * h * d);
	}

	slice& operator[](size_t idx_w)
	{
		return { v + (idx_w * height * depth), depth };
	}

};

struct voxels_paletted : public voxels<uint8_t>
{
	xmath::vec<4, uint8_t> palette[256];

	voxels_paletted() = default;

	voxels_paletted(const ogt_vox_palette& pal, const uint8_t* ptr, size_t w, size_t h, size_t d) : voxels(ptr, w, h, d)
	{
		for (int i = 0; i < 256; i++)
		{
			auto& color = pal.color[i];
			palette[i] = { color.r, color.g, color.b, color.a };
		}
	}
};


}
}