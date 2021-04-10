#include "g.gfx.h"

using namespace g::gfx;

void texture::release_bitmap()
{
	if (data)
	{
		free(data);
		data = nullptr;
	}
}

void texture::create(GLenum texture_type)
{
	type = texture_type;
	glGenTextures(1, &this->texture);
	assert(gl_get_error());
}

void texture::destroy()
{
	glDeleteTextures(1, &texture);
	release_bitmap();
}

void texture::set_pixels(size_t w, size_t h, char* data, GLenum format, GLenum storage, GLenum t)
{
	width = w;
	height = h;
	type = t;
	this->data = data;
	glTexImage2D(type, 0, format, width, height, 0, format, storage, data);
}

void texture::bind() const { glBindTexture(type, texture); }



texture_factory::texture_factory(int w, int h, GLenum type)
{
	data = nullptr;
	texture_type = type;
	width = w;
	height = h;
}

void texture_factory::abort(std::string message)
{
	std::cerr << message << std::endl;
	exit(-1);
}

texture_factory& texture_factory::from_png(const std::string& path)
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

texture_factory& texture_factory::color()
{
	color_type = GL_RGBA;
	storage_type = GL_UNSIGNED_BYTE;
	return *this;
}

texture_factory& texture_factory::depth()
{
	color_type = GL_DEPTH_COMPONENT;
	storage_type = GL_UNSIGNED_SHORT;
	return *this;
}

texture_factory& texture_factory::pixelated()
{
	min_filter = mag_filter = GL_NEAREST;
	return *this;
}

texture_factory& texture_factory::smooth()
{
	min_filter = mag_filter = GL_LINEAR;
	return *this;		
}

texture_factory& texture_factory::clamped()
{
	wrap_s = wrap_t = GL_CLAMP_TO_EDGE;
	return *this;		
}

texture_factory& texture_factory::repeating()
{
	wrap_s = wrap_t = GL_REPEAT;
	return *this;		
}

texture texture_factory::create()
{
	texture out;

	out.create(texture_type);
	out.bind();

	assert(gl_get_error());	
	out.set_pixels(width, height, data, color_type, storage_type);	
	assert(gl_get_error());


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glGenerateMipmap(GL_TEXTURE_2D);
	assert(gl_get_error());

	return out;
}