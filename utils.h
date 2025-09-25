#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <png.h>
#include <stdbool.h>

#define MAX_FILENAME_LENGTH 256

enum OptionFlags {
    OPT_RECT = 1000,
    OPT_LEFT_UP,
    OPT_RIGHT_DOWN,
    OPT_COLOR,
    OPT_FILL,
    OPT_FILL_COLOR,
    OPT_HEXAGON,
    OPT_CENTER,
    OPT_RADIUS,
    OPT_COPY,
    OPT_DEST_LEFT_UP = 1011,
    OPT_THICKNESS,
    OPT_INPUT
};

enum ErrorCodes {
    ERR_INVALID_COORD_FORMAT = 40, 
    ERR_INVALID_COLOR_FORMAT,         
    ERR_SAME_INPUT_OUTPUT,            
    ERR_INVALID_HEX_ARGS,           
    ERR_MISSING_INPUT_FILE,           
    ERR_MULTIPLE_ACTIONS,           
    ERR_UNKNOWN_OPTION,             
    ERR_INVALID_THICKNESS,          
    ERR_FILE_IO,                    
    ERR_INVALID_CHANNELS            
};
struct Color {
    uint8_t r, g, b;
};

struct Png {
    uint32_t width;
    uint32_t height;

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep* row_pointers;
    int color_type;
    int bit_depth;

    char input_file[MAX_FILENAME_LENGTH];
    char output_file[MAX_FILENAME_LENGTH];

    // Прямоугольник
    int draw_rectangle;
    int rect_left;
    int rect_up;
    int rect_right;
    int rect_down;
    int rect_thickness;
    int rect_fill;
    struct Color rect_fill_color;
    struct Color rect_border_color;

    // Шестиугольник
    int draw_hexagon;
    int hex_center_x;
    int hex_center_y;
    int hex_radius;
    int hex_thickness;
    int hex_fill;
    struct Color hex_border_color;
    struct Color hex_fill_color;

    // Копирование
    int src_left, src_top;
    int src_right, src_bottom;
    int dest_left, dest_top;
    int copy_mode;

    int error_code;
};

// Работа с PNG
int read_png_file(const char *filename, struct Png *image);
int write_png_file(const char *filename, struct Png *image);
void free_image(struct Png *image);
void process_file(struct Png *image);

// Рисование
void draw_rectangle(struct Png* image, int x0, int y0, int x1, int y1, int thickness, int* color, bool fill, int* fill_color);
void draw_hexagon(struct Png* image, int x0, int y0, float r, float thickness, int* color, bool fill, int* fill_color);
void draw_line1(struct Png* image, int x0, int y0, int x1, int y1, float r, float thickness, int* color, bool fill, int* fill_color);
void draw_line2(struct Png* image, int x0, int y0, int x1, int y1, float thickness, int* color);
void draw_line3(struct Png* image, int x0, int y0, int x1, int y1, float r, float thickness, int* color, bool fill, int* fill_color);
void plot_circle(struct Png* image, int xm, int ym, int thickness, int* color);
void set_pixel(struct Png* image, int x, int y, int* color);
void fill_part(struct Png* image, int x, int x0, int x1, int y, bool fill, int* fill_color);

// Копирование
void copy_region(struct Png* image, int src_left, int src_top, int src_right, int src_bottom,
    int dest_left, int dest_top);

#endif