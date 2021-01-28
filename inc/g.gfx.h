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

	shader& bind() { glUseProgram(program); return *this; }

	struct uniform_usage;
	/**
	 * @brief      shader::usage type represents the start of some invocation
	 * of an interaction with a shader.
	 */
	struct usage {
		shader& shader_ref;
		size_t vertices, indices;

		usage (shader& ref, size_t verts, size_t inds) : shader_ref(ref)
		{
			vertices = verts;
			indices = inds;
		}

		template<typename MV>
		usage attach_attributes(const shader& shader)
		{
			MV::attributes(shader.program);
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

		usage& draw_tri_fan()
		{
			if (indices > 0)
			{
				glDrawElements(GL_TRIANGLE_FAN, indices, GL_UNSIGNED_INT, NULL);
			}
			else
			{
				glDrawArrays(GL_TRIANGLE_FAN, 0, vertices);
			}

			return *this;
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
			delete[] src;

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

		return out;
	}
};


namespace vertex
{
	struct pos_uv_norm
	{
		vec<3> position;
		vec<2> uv;
		vec<3> normal;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");
			auto uv_loc = glGetAttribLocation(prog, "a_uv");
			auto norm_loc = glGetAttribLocation(prog, "a_normal");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (uv_loc > -1) glEnableVertexAttribArray(uv_loc);
			if (norm_loc > -1) glEnableVertexAttribArray(norm_loc);

			auto p_size = sizeof(position);
			auto uv_size = sizeof(uv);

			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)0);
			if (uv_loc > -1) glVertexAttribPointer(uv_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)p_size);
			if (norm_loc > -1) glVertexAttribPointer(norm_loc, 3, GL_FLOAT, false, sizeof(pos_uv_norm), (void*)(p_size + uv_size));
		}
	};
}


template<typename V>
struct mesh {
	GLuint vbo, ibo;
	std::vector<uint32_t> indices;
	std::vector<V> vertices;

	mesh& set_vertices(const std::vector<V>& verts)
	{
		vertices = verts;
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			verts.size() * sizeof(V),
			verts.data(),
			GL_STATIC_DRAW
		);

		return *this;
	}

	shader::usage using_shader (shader& shader)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		if (indices.size() > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
		}

		shader.bind();
		shader::usage usage = {shader, vertices.size(), indices.size()};
		usage.attach_attributes<V>(shader);
		return usage;
	}
};


struct mesh_factory {
	static mesh<vertex::pos_uv_norm> plane()
	{
		mesh<vertex::pos_uv_norm> p;
		glGenBuffers(2, &p.vbo);

		p.set_vertices({
			{{-1, 1, 0}, {1, 1}, {0, 0, 1}},
			{{ 1, 1, 0}, {0, 1}, {0, 0, 1}},
			{{ 1,-1, 0}, {0, 0}, {0, 0, 1}},
			{{-1,-1, 0}, {1, 0}, {0, 0, 1}},
		});

		return p;
	}
};

};
};
