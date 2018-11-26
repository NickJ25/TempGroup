#include "PNGProcessor.h"

// Randomizer provided from http://www.xbdev.net/directx3dx/specialX/Fur/index.php
const float INV_RAND_MAX = 1.0 / (RAND_MAX + 1);
inline float rnd(float max = 1.0) { return max * INV_RAND_MAX * rand(); }
inline float rnd(float min, float max) { return min + (max - min) * INV_RAND_MAX * rand(); }

// Read PNG Files and store data
void PNGProcessor::readPNG(const char * file_name)
{

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp) abort();

	/* initialize stuff */
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) abort();

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) abort();

	if (setjmp(png_jmpbuf(png_ptr))) abort();

	png_init_io(png_ptr, fp);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	//number_of_passes = png_set_interlace_handling(png_ptr);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++)
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

	png_read_image(png_ptr, row_pointers);

	fclose(fp);
}

// Write to the read PNG file.
void PNGProcessor::writePNG(const char * file_name)
{
	// Create file
	int y;

	FILE *fp = fopen(file_name, "wb");
	if (!fp) {
		cout << "[PNG Writer] File " << file_name << " could not be opened for writing" << endl;
		return;
	}

	// Initalize Stuff
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		cout << "[PNG Writer] png_create_write_struct failed" << endl;
		return;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		cout << "[PNG Writer] png_create_info_struct failed" << endl;
		return;
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		cout << "[PNG Writer] Error during init_io" << endl;
		return;
	}
	png_init_io(png_ptr, fp);

	// Write Header

	png_set_IHDR(png_ptr, info_ptr, width, height,
		bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	// Write Bytes
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, NULL);

	// Cleanup heap allocation
	for (y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);
	fclose(fp);
}

// Standard Fur Strand Randomizer Method
GLuint PNGProcessor::createFurTextures(int seed, int size, int num, int density, int texWidth, int texHeight)
{
	width = texWidth;
	height = texHeight;
	srand(seed);
	int m_Size = size;
	int m_NumLayers = num;

	// Clear Layers and make background transparent
	GLubyte *data = new GLubyte[width * height * 4];
	for (int layer = 0; layer < m_NumLayers; layer++) {
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				data[((row * width + col) * 4)] = (GLubyte)(0.0f * 255.0f);
				
			}
		}
	}
	// Put dots randomly across the transparent images
	for (int layer = 0; layer < m_NumLayers; layer++) {
		srand(28382);
		float length = float(layer) / m_NumLayers;
		int m_density = density * length * 3;
		for (int i = 0; i < density; i++) {
			int xrand = rnd(0, m_Size) * 4;
			int yrand = rnd(0, m_Size) * 4;
			data[((xrand *width + yrand) * 4)] = (GLubyte)(1.0f * 255.0f);
		}
	}

	// Bind texture and return it
	GLuint texID;
	glGenTextures(1, &texID);

	glBindTexture(GL_TEXTURE_2D, texID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return texID;
}

// Fur Strand Randomizer with PNG export
GLuint PNGProcessor::createFurTextures(int seed, int size, int num, int density, const char* file_name)
{
	// Read PNG
	readPNG(file_name);

	srand(seed);
	int m_Size = size;
	int m_NumLayers = num;

	// Clear Layers and make background transparent
	GLubyte *data = new GLubyte[width * height * 4];
	for (int layer = 0; layer < m_NumLayers; layer++) {
		for (int row = 0; row < height; row++) {
			png_byte* png_row = row_pointers[row];
			for (int col = 0; col < width; col++) {
				png_bytep px = &(png_row[col * 4]);
				data[((row * width + col) * 4)] = (GLubyte)(0.0f * 255.0f);
				px[0] = (0.0f* 255.0f);
				px[1] = (0.0f* 255.0f);
				px[2] = (0.0f* 255.0f);
				px[3] = (0.0f* 255.0f);

			}
		}
	}
	// Put dots randomly across the transparent images
	for (int layer = 0; layer < m_NumLayers; layer++) {
		srand(28382);
		float length = float(layer) / m_NumLayers;
		int m_density = density * length * 3;
		for (int i = 0; i < density; i++) {
			int xrand = rnd(0, m_Size) * 4;
			int yrand = rnd(0, m_Size) * 4;
			png_byte* png_row = row_pointers[yrand];
			png_bytep px = &(png_row[xrand * 4]);
			px[0] = (1.0f* 255.0f);
			px[1] = (1.0f* 255.0f);
			px[2] = (1.0f* 255.0f);
			px[3] = (1.0f* 255.0f);
			data[((xrand *width + yrand) * 4)] = (GLubyte)(1.0f * 255.0f);
		}
	}
	// Write to PNG
	writePNG(file_name);

	// Bind texture and return it
	GLuint texID;
	glGenTextures(1, &texID);
	
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return texID;
}

