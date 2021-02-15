#pragma once
#include "xmath.h"

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

}
}