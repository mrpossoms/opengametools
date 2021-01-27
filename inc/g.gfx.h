#pragma once
#define XMTYPE float
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

static bool gl_get_error()
{
	GLenum err = GL_NO_ERROR;
	bool good = true;

	while((err = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "GL_ERROR: 0x" << std::hex << err << std::endl;
		good = false;
	}

	return good;
}

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

			return uniform_usage(*this, loc);
		}
	};

	/**
	 * @brief      Offers interaction with the uniforms defined for a given shader
	 */
	struct uniform_usage {
		GLuint uni_loc;
		usage parent_usage;

		uniform_usage(usage parent, GLuint loc) : parent_usage(parent) { uni_loc = loc; }
	
		inline usage mat4 (const mat<4, 4>& m)
		{
			glUniformMatrix4fv(uni_loc, 1, false, m.ptr());

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
		GLint status;
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

	template<GLenum ST>
	shader_factory add_src(const std::string& src)
	{
		{ // read and compile the shader
			shaders[ST] = compile_shader(ST, src.c_str(), (GLsizei)src.length());
		}

		return *this;
	}

	shader create()
	{
		GLint status;
		shader out;
		out.program = glCreateProgram();

		glUseProgram(out.program);

		for (auto shader : shaders)
		{
			glAttachShader(out.program, shader.second);
		}

		assert(gl_get_error());
		glLinkProgram(out.program);

		glGetProgramiv(out.program, GL_LINK_STATUS, &status);
		if (status == 0)
		{
			GLint log_length;
			glGetProgramiv(out.program, GL_INFO_LOG_LENGTH, &log_length);
			if (log_length > 0)
			{
				GLchar *log_str = (GLchar *)malloc(log_length);
				glGetProgramInfoLog(out.program, log_length, &log_length, log_str);
				std::cerr << "Shader link log: " << log_length << std::endl << log_str << std::endl;
				write(1, log_str, log_length);
				free(log_str);
			}
			exit(-1);
		}
		else
		{
			std::cerr << "Linked program " << out.program << std::endl;
		}

		assert(gl_get_error());

		// Detach all
		for (auto shader : shaders)
		{
			glDetachShader(out.program, shader.second);
		}
	}
};


namespace vertex
{
	struct pos_uv_norm
	{
		vec<3> position;
		vec<2> uv;
		vec<3> normal;

		void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto uv_loc = glGetAttribLocation(prog, "a_uv");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");

			glEnableVertexAttribArray(pos_loc);
			glEnableVertexAttribArray(uv_loc);
			glEnableVertexAttribArray(norm_loc);

			auto p_size = sizeof(position);
			auto uv_size = sizeof(uv);
			
			glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)0);
			glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)p_size);
			glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)(p_size + uv_size));
		}
	};
}


template<typename V>
struct mesh {
	std::vector<uint32_t> indices;
	std::vector<V> vertices;

	shader::usage using_shader (const shader& shader)
	{
		return shader::usage{};
	}
};


struct mesh_factory {

};


};
};