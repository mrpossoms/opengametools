#include "xmath.h"

using namespace xmath;

namespace g {
namespace game {

struct camera
{
	vec<3> position = {0, 0, 0}; 
	quat orientation = {0, 0, 0, 1};

	mat<4, 4> view()
	{
		return mat<4, 4>::translation(position) * orientation.to_matrix();
	}
};

}
}