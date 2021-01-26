#pragma once
#include <xmath.h>

#ifdef __linux__
#include <GL/glx.h>
#include <GL/glext.h>
#endif

#include <GLFW/glfw3.h>

#ifdef __APPLE__
#undef __gl_h_
#include <OpenGL/gl3.h>
#endif

using namespace xmath;

namespace g {
namespace gfx {

static size_t width();
static size_t height();
static float aspect();  

struct texture {};

/**
 * @brief      { struct_description }
 */
struct shader {
	GLuint program;
	std::unordered_map<std::string, GLint> uni_locs;

	struct uniform_usage;
	/**
	 * @brief      shader::usage type represents the start of some invocation
	 * of an interaction with a shader.
	 */
	struct usage {
		shader& shader_ref;

		usage (shader& ref) : shader_ref(ref) {}

		template<typename V>
		usage attach_attributes()
		{ 
			V::attributes();
			return *this; 
		}

		uniform_usage set_uniform(const std::string& name)
		{
			GLint loc;
			auto it = shader_ref.uni_locs.find(name);
			if (it == shader_ref.uni_locs.end())
			{
				loc = glGetUniformLocation(shader_ref.program, name.c_str());
			
				if (loc > -1)
				{
					// TODO: handle the missing uniform better
					std::cerr << "uniform '" << name << "' doesn't exist\n";
					shader_ref.uni_locs[name] = loc;
				}
			}
			else
			{
				loc = (*it).second;
			}

			return uniform_usage(loc);
		}
	};

	/**
	 * @brief      Offers interaction with the uniforms defined for a given shader
	 */
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
};


struct shader_factory
{
	std::unordered_map<GLenum, GLuint> shaders;

	static GLuint compile_shader (GLenum type, const GLchar* src, GLsizei len)
	{
		// Create the GL shader and attempt to compile it
		auto shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, &len);
		glCompileShader(shader);

		assert(gl_get_error());

		// Print the compilation log if there's anything in there
		GLint log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0)
		{
			GLchar *log_str = (GLchar *)malloc(log_length);
			glGetShaderInfoLog(shader, log_length, &log_length, log_str);
			std::cerr << "Shader compile log: " << log_length << std::endl << log_str << std::endl;
			write(1, log_str, log_length);
			free(log_str);
		}

		assert(gl_get_error());

		// Check the status and exit on failure
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			std::cerr << "Compiling failed: " << status << std::endl;
			glDeleteShader(shader);

			std::cerr << src << std::endl;
			exit(-2);
		}

		assert(gl_get_error());
		std::cerr << "OK" << std::endl;

		return shader;
	}

	template<GLenum ST>
	shader_factory add(const std::string& path)
	{
		auto fd = open(path.c_str(), O_RDONLY);
		auto size = lseek(fd, SEEK_END, 0);

		{ // read and compile the shader
			GLchar* src = new GLchar[size];
			lseek(fd, SEEK_SET, 0);
			read(fd, src, size);
			close(fd);
			delete src;
			
			shaders[ST] = compile_shader(ST, src, (GLsizei)size);
		}

		return *this;
	}
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
};