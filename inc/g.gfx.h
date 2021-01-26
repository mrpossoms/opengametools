#include <xmath.h>

using namespace xmath;

namespace gfx {

static size_t width();
static size_t height();
static float aspect();  

struct texture {};

struct shader {
	GLuint program; 

	struct usage;
	struct uniform_usage {
		GLuint uni_loc;

		uniform_usage(GLuint loc) { uni_loc = loc; }

		usage parent_usage;
		uniform_usage(usage parent) : parent_usage(parent) { }
	
		inline usage mat4 (const mat<4, 4>& m)
		{
			glUniformMatrix4fv(uni_loc, false, m.m);

			return parent_usage;
		}
	};

	struct usage {
		template<typename V>
		usage attach_attributes()
		{ 
			V::attributes();
			return *this; 
		}

		uniform_usage set_uniform(const std::string& name)
		{
			return uniform_usage{};
		}
	};
};

template<typename V>
struct mesh {
	std::vector<uint32_t> indices;
	std::vector<V> vertices;

	shader::usage using_shader (const shader& shader)
	{
		return shader::usage{};
	}
};

};
