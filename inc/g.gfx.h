#pragma once
#define XMTYPE float
#include <xmath.h>
#include <g.game.h>
#include <string.h>

#ifdef __linux__
#include <GL/glx.h>
#include <GL/glext.h>
#endif

#include <GLFW/glfw3.h>
#include <png.h>

#ifdef __APPLE__
#undef __gl_h_
#include <OpenGL/gl3.h>
#endif

#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

#define G_TERM_GREEN "\033[0;32m"
#define G_TERM_RED "\033[1;31m"
#define G_TERM_YELLOW "\033[1;33m"
#define G_TERM_COLOR_OFF "\033[0m"

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

extern GLFWwindow* GLFW_WIN;

static size_t width()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width;
}

static size_t height()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return height;
}

static float aspect()
{
	int width, height;
	glfwGetFramebufferSize(GLFW_WIN, &width, &height);
	return width / (float)height;
}

struct texture 
{
	GLenum type;
	size_t width, height;
	GLuint texture = (GLuint)-1;
	char* data = nullptr;

	void release_bitmap()
	{
		if (data)
		{
			free(data);
			data = nullptr;
		}
	}

	void create(GLenum texture_type)
	{
		type = texture_type;
		glGenTextures(1, &this->texture);
		assert(gl_get_error());
	}

	void destroy()
	{
		glDeleteTextures(1, &texture);
		release_bitmap();
	}

	void set_pixels(size_t w, size_t h, char* data, GLenum format=GL_RGBA, GLenum storage=GL_UNSIGNED_BYTE, GLenum t=GL_TEXTURE_2D)
	{
		width = w;
		height = h;
		type = t;
		this->data = data;
		glTexImage2D(type, 0, format, width, height, 0, format, storage, data);
	}

	void bind() const { glBindTexture(type, texture); }
};


struct texture_factory
{
	int width, height;
	char* data = nullptr;
	GLenum texture_type;
	GLenum min_filter = GL_LINEAR, mag_filter = GL_LINEAR;
	GLenum wrap_s = GL_CLAMP_TO_EDGE, wrap_t = GL_CLAMP_TO_EDGE;
	GLenum color_type = GL_RGBA;
	GLenum storage_type = GL_UNSIGNED_BYTE;


	texture_factory(int w=0, int h=0, GLenum type=GL_TEXTURE_2D)
	{
		data = nullptr;
		texture_type = type;
		width = w;
		height = h;
	}

	void abort(std::string message)
	{
		std::cerr << message << std::endl;
		exit(-1);
	}

	texture_factory& from_png(const std::string& path)
	{
		char header[8];    // 8 is the maximum size that can be checked
		png_structp png_ptr = {};
		png_infop info_ptr;
		png_bytep* row_pointers;
		png_byte png_color_type;

		std::cerr << "loading texture '" <<  path << "'... ";

		/* open file and test for it being a png */
		FILE *fp = fopen(path.c_str(), "rb");
		if (!fp)
		{
			fprintf(stderr, G_TERM_RED "[read_png_file] File %s could not be opened for reading" G_TERM_COLOR_OFF, path.c_str());
			return *this;
		}

		fread(header, 1, 8, fp);
		if (png_sig_cmp((png_bytep)header, 0, 8))
		{
			fprintf(stderr, G_TERM_RED "[read_png_file] File %s is not recognized as a PNG file" G_TERM_COLOR_OFF, path.c_str());
		}


		/* initialize stuff */
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		if (!png_ptr)
			abort(G_TERM_RED "[read_png_file] png_create_read_struct failed" G_TERM_COLOR_OFF);

		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
			abort(G_TERM_RED "[read_png_file] png_create_info_struct failed" G_TERM_COLOR_OFF);

		if (setjmp(png_jmpbuf(png_ptr)))
			abort(G_TERM_RED "[read_png_file] Error during init_io" G_TERM_COLOR_OFF);

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		width = png_get_image_width(png_ptr, info_ptr);
		height = png_get_image_height(png_ptr, info_ptr);
		png_color_type = png_get_color_type(png_ptr, info_ptr);
		auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
		auto channels = png_get_channels(png_ptr, info_ptr);
		auto color_depth = channels * (bit_depth >> 3);

		switch (bit_depth)
		{
			case 8:
				storage_type = GL_UNSIGNED_BYTE;
				break;
		}

		//number_of_passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);

		/* read file */
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			abort(G_TERM_RED "[read_png_file] Error during read_image" G_TERM_COLOR_OFF);
		}

		switch (png_color_type) {
			case PNG_COLOR_TYPE_RGBA:
				color_type = GL_RGBA;
				break;
			case PNG_COLOR_TYPE_PALETTE:
			case PNG_COLOR_TYPE_RGB:
				color_type = GL_RGB;
				break;
		}

		row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
		char* pixel_buf = (char*)calloc(color_depth * width * height, sizeof(char));
		int bytes_per_row = png_get_rowbytes(png_ptr,info_ptr);

		for (int y = 0; y < height; y++)
		{
			row_pointers[y] = (png_byte*) malloc(bytes_per_row); // 
			assert(row_pointers[y]);
		}

		png_read_image(png_ptr, row_pointers);
		for (int y = 0; y < height; y++)
		{
			memcpy(pixel_buf + (y * bytes_per_row), row_pointers[y], bytes_per_row);
			free(row_pointers[y]);
		}
		free(row_pointers);
		fclose(fp);

		data = pixel_buf;

		std::cerr << G_TERM_GREEN "OK" G_TERM_COLOR_OFF << std::endl;

		return *this;
	}

	texture_factory& color()
	{
		color_type = GL_RGBA;
		storage_type = GL_UNSIGNED_BYTE;
		return *this;
	}

	texture_factory& depth()
	{
		color_type = GL_DEPTH_COMPONENT;
		storage_type = GL_UNSIGNED_SHORT;
		return *this;
	}

	texture_factory& pixelated()
	{
		min_filter = mag_filter = GL_NEAREST;
		return *this;
	}

	texture_factory& smooth()
	{
		min_filter = mag_filter = GL_LINEAR;
		return *this;		
	}

	texture_factory& clamped()
	{
		wrap_s = wrap_t = GL_CLAMP_TO_EDGE;
		return *this;		
	}

	texture_factory& repeating()
	{
		wrap_s = wrap_t = GL_REPEAT;
		return *this;		
	}

	texture create()
	{
		texture out;

		out.create(texture_type);
		out.bind();

		assert(gl_get_error());
		if (data)
		{
			out.set_pixels(width, height, data, color_type, storage_type);	
		}	
		assert(gl_get_error());


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glGenerateMipmap(GL_TEXTURE_2D);
		assert(gl_get_error());

		return out;
	}
};


struct framebuffer
{
	GLuint fbo;
	size_t width, height;
	texture color;
	texture depth;

	void bind_as_target()
	{
		glViewport(0, 0, width, height);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	void unbind_as_target()
	{
		if (color.texture != (GLuint)-1)
		{
			color.bind();
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glViewport(0, 0, g::gfx::width(), g::gfx::height());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};


struct framebuffer_factory
{
	int width, height;
	texture color_tex, depth_tex;

	framebuffer_factory(int w, int h)
	{
		width = w;
		height = h;
	}

	framebuffer_factory& color()
	{
		color_tex = texture_factory{ width, height }.color().clamped().smooth().create();
		return *this;
	}

	framebuffer_factory& depth()
	{
		depth_tex = texture_factory{ width, height }.depth().clamped().smooth().create();
		return *this;
	}

	framebuffer create()
	{
		framebuffer fb;

		fb.width = width;
		fb.height = height;
		fb.color = color_tex;
		fb.depth = depth_tex; 
		glGenFramebuffers(1, &fb.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
		assert(gl_get_error());

		if (color_tex.texture != (GLuint)-1)
		{
			color_tex.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.texture, 0);
		}

		if (depth_tex.texture != (GLuint)-1)
		{
			depth_tex.bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.texture, 0);
		}

		auto fb_stat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(fb_stat == GL_FRAMEBUFFER_COMPLETE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return fb;
	}
};

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
		int texture_unit;

		usage (shader& ref, size_t verts, size_t inds) : shader_ref(ref)
		{
			vertices = verts;
			indices = inds;
			texture_unit = 0;
		}

		template<typename MV>
		usage attach_attributes(const shader& shader)
		{
			MV::attributes(shader.program);
			return *this;
		}

		usage set_camera(const g::game::camera& cam)
		{
			this->set_uniform("u_view").mat4(cam.view());
			this->set_uniform("u_proj").mat4(cam.projection(aspect()));
			return *this;
		}

		uniform_usage set_uniform(const std::string& name)
		{
			GLint loc;
			auto it = shader_ref.uni_locs.find(name);
			if (it == shader_ref.uni_locs.end())
			{
				loc = glGetUniformLocation(shader_ref.program, name.c_str());

				if (loc < 0)
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

		uniform_usage operator[](const std::string& name)
		{
			return set_uniform(name);
		}

		template<GLenum PRIM>
		usage& draw()
		{
			if (indices > 0)
			{
				glDrawElements(PRIM, indices, GL_UNSIGNED_INT, NULL);
			}
			else
			{
				glDrawArrays(PRIM, 0, vertices);
			}

			return *this;
		}

		usage& draw_tri_fan()
		{
			return draw<GL_TRIANGLE_FAN>();
		}
	};

	/**
	 * @brief      Offers interaction with the uniforms defined for a given shader
	 */
	struct uniform_usage {
		GLuint uni_loc;
		usage& parent_usage;

		uniform_usage(usage& parent, GLuint loc) : parent_usage(parent) { uni_loc = loc; }

		inline usage mat4 (const mat<4, 4>& m)
		{
			glUniformMatrix4fv(uni_loc, 1, false, m.ptr());

			return parent_usage;
		}

		inline usage texture(const texture& tex)
		{
			glActiveTexture(GL_TEXTURE0 + parent_usage.texture_unit);
			tex.bind();
			glUniform1i(uni_loc, parent_usage.texture_unit);
			parent_usage.texture_unit++;
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

	struct pos
	{
		vec<3> position;

		static void attributes(GLuint prog)
		{
			auto pos_loc = glGetAttribLocation(prog, "a_position");

			if (pos_loc > -1) glEnableVertexAttribArray(pos_loc);
			if (pos_loc > -1) glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(pos), (void*)0);
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

	mesh& set_indices(const std::vector<uint32_t>& inds)
	{
		indices = inds;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			inds.size() * sizeof(uint32_t),
			inds.data(),
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

	template<typename V>
	static mesh<V> from_voxels(g::game::voxels_paletted& vox, std::function<V(ogt_mesh_vertex* vert_in)> converter)
	{
		mesh<V> m;

		return m;
	}


	template<typename V>
	static mesh<V> empty_mesh()
	{
		mesh<V> m;
		glGenBuffers(2, &m.vbo);
		return m;		
	}
};

};
};
