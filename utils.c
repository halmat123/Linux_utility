#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

int read_png_file(const char* filename, struct Png* image) {
    png_byte header[8];
    FILE* fp = fopen(filename, "rb");
    int code = 0;

    if (fp) {
        fread(header, 1, 8, fp);
        if (!png_sig_cmp(header, 0, 8)) {
            image->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            if (image->png_ptr) {
                image->info_ptr = png_create_info_struct(image->png_ptr);
                if (image->info_ptr) {
                    if (!setjmp(png_jmpbuf(image->png_ptr))) {
                        png_init_io(image->png_ptr, fp);
                        png_set_sig_bytes(image->png_ptr, 8);
                        png_read_info(image->png_ptr, image->info_ptr);

                        image->width = png_get_image_width(image->png_ptr, image->info_ptr);
                        image->height = png_get_image_height(image->png_ptr, image->info_ptr);
                        image->color_type = png_get_color_type(image->png_ptr, image->info_ptr);
                        image->bit_depth = png_get_bit_depth(image->png_ptr, image->info_ptr);

                        if (image->bit_depth == 16)
                            png_set_strip_16(image->png_ptr);
                        if (image->color_type == PNG_COLOR_TYPE_PALETTE)
                            png_set_palette_to_rgb(image->png_ptr);
                        if (image->color_type == PNG_COLOR_TYPE_GRAY && image->bit_depth < 8)
                            png_set_expand_gray_1_2_4_to_8(image->png_ptr);
                        if (png_get_valid(image->png_ptr, image->info_ptr, PNG_INFO_tRNS))
                            png_set_tRNS_to_alpha(image->png_ptr);
                        if (!(image->color_type & PNG_COLOR_MASK_ALPHA))
                            png_set_add_alpha(image->png_ptr, 0xFF, PNG_FILLER_AFTER);

                        png_set_gray_to_rgb(image->png_ptr);
                        png_read_update_info(image->png_ptr, image->info_ptr);

                        int channels = png_get_channels(image->png_ptr, image->info_ptr);
                        if (channels == 4) {
                            png_size_t rowbytes = png_get_rowbytes(image->png_ptr, image->info_ptr);
                            image->row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * image->height);
                            if (image->row_pointers) {
                                for (int y = 0; y < image->height && code == 0; y++) {
                                    image->row_pointers[y] = (png_byte*)malloc(rowbytes);
                                    if (!image->row_pointers[y]) {
                                        fprintf(stderr, "Failed to allocate memory for row %d.\n", y);
                                        for (int i = 0; i < y; i++) free(image->row_pointers[i]);
                                        free(image->row_pointers);
                                        image->row_pointers = NULL;
                                        code = ERR_FILE_IO;
                                    }
                                }
                                if (code == 0) {
                                    png_read_image(image->png_ptr, image->row_pointers);
                                }
                            } else {
                                fprintf(stderr, "Failed to allocate memory for row_pointers.\n");
                                code = ERR_FILE_IO;
                            }
                        } else {
                            fprintf(stderr, "Expected 4 channels (RGBA), got %d. Aborting.\n", channels);
                            code = ERR_INVALID_CHANNELS;
                        }
                    } else {
                        fprintf(stderr, "libpng encountered an error during reading.\n");
                        code = ERR_FILE_IO;
                    }
                } else {
                    fprintf(stderr, "Error in png_create_info_struct\n");
                    code = ERR_FILE_IO;
                }
            } else {
                fprintf(stderr, "Error in png_create_read_struct\n");
                code = ERR_FILE_IO;
            }
        } else {
            fprintf(stderr, "Error: %s is not a valid PNG file.\n", filename);
            code = ERR_FILE_IO;
        }

        if (code != 0) {
            if (image->png_ptr && image->info_ptr)
                png_destroy_read_struct(&image->png_ptr, &image->info_ptr, NULL);
            else if (image->png_ptr)
                png_destroy_read_struct(&image->png_ptr, NULL, NULL);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Cannot read file: %s\n", filename);
        code = ERR_FILE_IO;
    }

    return code;
}

int write_png_file(const char *file_name, struct Png *image) {
    FILE *fp = NULL;
    png_structp write_png_ptr = NULL;
    png_infop write_info_ptr = NULL;
    int code = 0;

    fp = fopen(file_name, "wb");
    if (fp) {
        write_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (write_png_ptr) {
            write_info_ptr = png_create_info_struct(write_png_ptr);
            if (write_info_ptr) {
                if (!setjmp(png_jmpbuf(write_png_ptr))) {
                    png_init_io(write_png_ptr, fp);
                    png_set_IHDR(
                        write_png_ptr,
                        write_info_ptr,
                        image->width,
                        image->height,
                        8,
                        PNG_COLOR_TYPE_RGBA,
                        PNG_INTERLACE_NONE,
                        PNG_COMPRESSION_TYPE_BASE,
                        PNG_FILTER_TYPE_BASE
                    );

                    png_write_info(write_png_ptr, write_info_ptr);
                    png_write_image(write_png_ptr, image->row_pointers);
                    png_write_end(write_png_ptr, NULL);
                } else {
                    fprintf(stderr, "libpng error during writing PNG.\n");
                    code = ERR_INVALID_CHANNELS;
                }
            } else {
                fprintf(stderr, "Error creating PNG info structure\n");
                code = ERR_INVALID_CHANNELS;
            }
        } else {
            fprintf(stderr, "Error creating PNG write structure\n");
            code = ERR_FILE_IO;
        }
    } else {
        fprintf(stderr, "Cannot open file: %s\n", file_name);
        code = ERR_FILE_IO;
    }

    if (write_png_ptr && write_info_ptr) {
        png_destroy_write_struct(&write_png_ptr, &write_info_ptr);
    } else if (write_png_ptr) {
        png_destroy_write_struct(&write_png_ptr, NULL);
    }

    if (fp) fclose(fp);
    return code;
}

void process_file(struct Png* image) {
    if (image) {
        if (image->draw_hexagon) {
            draw_hexagon(image,
                image->hex_center_x,
                image->hex_center_y,
                image->hex_radius,
                image->hex_thickness,
                (int[]){ image->hex_border_color.r, image->hex_border_color.g, image->hex_border_color.b },
                image->hex_fill,
                (int[]){ image->hex_fill_color.r, image->hex_fill_color.g, image->hex_fill_color.b }
            );
        } else if (image->draw_rectangle) {
            draw_rectangle(image,
                image->rect_left,
                image->rect_up,
                image->rect_right,
                image->rect_down,
                image->rect_thickness,
                (int[]){ image->rect_border_color.r, image->rect_border_color.g, image->rect_border_color.b },
                image->rect_fill,
                (int[]){ image->rect_fill_color.r, image->rect_fill_color.g, image->rect_fill_color.b }
            );
        } else if (image->copy_mode) {
            copy_region(image,
                image->src_left,
                image->src_top,
                image->src_right,
                image->src_bottom,
                image->dest_left,
                image->dest_top
            );
        }
    }
}

void free_image(struct Png *image) {
    if (image->row_pointers != NULL) {
        for (int y = 0; y < image->height; y++) {
            if (image->row_pointers[y] != NULL) {
                free(image->row_pointers[y]);
            }
        }
        free(image->row_pointers);
        image->row_pointers = NULL;
    }
}

void draw_rectangle(struct Png* image, int x0, int y0, int x1, int y1,
                    int thickness, int* color, bool fill, int* fill_color) {
    if (image && image->row_pointers && image->png_ptr && image->info_ptr) {
        bool valid = true;
        int width = image->width;
        int height = image->height;

        if (color[0] < 0 || color[0] > 255 ||
            color[1] < 0 || color[1] > 255 ||
            color[2] < 0 || color[2] > 255) {
            fprintf(stderr, "Invalid border color values. Must be between 0 and 255.\n");
            image->error_code = ERR_INVALID_COLOR_FORMAT;
            valid = false;
        }

        if (x0 >= x1) { int tmp = x0; x0 = x1; x1 = tmp; }
        if (y0 >= y1) { int tmp = y0; y0 = y1; y1 = tmp; }

        int t = thickness / 2;

        if (fill) {
            if (fill_color[0] < 0 || fill_color[0] > 255 ||
                fill_color[1] < 0 || fill_color[1] > 255 ||
                fill_color[2] < 0 || fill_color[2] > 255) {
                fprintf(stderr, "Invalid fill color values. Must be between 0 and 255.\n");
                image->error_code = ERR_INVALID_COLOR_FORMAT;
                valid = false;
            } else if (valid) {
                for (int y = y0; y <= y1; y++) {
                    if (y >= 0 && y < height) {
                        for (int x = x0; x <= x1; x++) {
                            if (x >= 0 && x < width) {
                                set_pixel(image, x, y, fill_color);
                            }
                        }
                    }
                }
            }
        }

        if (valid) {
            // Верхняя граница
            for (int y = y0 - t; y <= y0 + t; y++) {
                if (y >= 0 && y < height) {
                    for (int x = x0 - t; x <= x1 + t; x++) {
                        if (x >= 0 && x < width) {
                            set_pixel(image, x, y, color);
                        }
                    }
                }
            }

            // Нижняя граница
            for (int y = y1 - t; y <= y1 + t; y++) {
                if (y >= 0 && y < height) {
                    for (int x = x0 - t; x <= x1 + t; x++) {
                        if (x >= 0 && x < width) {
                            set_pixel(image, x, y, color);
                        }
                    }
                }
            }

            // Левая граница
            for (int x = x0 - t; x <= x0 + t; x++) {
                if (x >= 0 && x < width) {
                    for (int y = y0; y <= y1; y++) {
                        if (y >= 0 && y < height) {
                            set_pixel(image, x, y, color);
                        }
                    }
                }
            }

            // Правая граница
            for (int x = x1 - t; x <= x1 + t; x++) {
                if (x >= 0 && x < width) {
                    for (int y = y0; y <= y1; y++) {
                        if (y >= 0 && y < height) {
                            set_pixel(image, x, y, color);
                        }
                    }
                }
            }
        }
    }

    return;
}
void set_pixel(struct Png* image, int x, int y, int* color) {
    if (image && image->row_pointers && image->png_ptr && image->info_ptr) {
        if (x >= 0 && x < image->width && y >= 0 && y < image->height) {
            int channels = png_get_channels(image->png_ptr, image->info_ptr);
            png_bytep px = &(image->row_pointers[y][x * channels]);
            px[0] = color[0];
            px[1] = color[1];
            px[2] = color[2];
            if (channels == 4) {
                px[3] = 255;
            }
        }
    }
}

void fill_part(struct Png* image, int x, int x0, int x1, int y, bool fill, int* fill_color) {
    if (fill) {
        if (x0 > x1) {
            int tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
        for (int i = x0; i <= x1; i++) {
            set_pixel(image, i, y, fill_color);
        }
    }
}

void plot_circle(struct Png* image, int xm, int ym, int thickness, int* color) {
    int x0 = xm - thickness/2, x1 = xm + thickness/2, y0 = ym - thickness/2, y1 = ym + thickness/2;
    for (int x = x0; x <= x1; x++) {
        for (int y = y0; y <= y1; y++) {
            if ((x-xm)*(x-xm) + (y-ym)*(y-ym) <= (thickness/2)*(thickness/2)) {
                set_pixel(image, x, y, color);
            }
        }
    }
}

void draw_line1(struct Png* image, int x0, int y0, int x1, int y1, float r, float thickness, int* color, bool fill, int* fill_color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int d = 2 * dx - dy;
    int incrE = 2 * dx;
    int incrNE = 2 * (dx - dy);
    int x = x0, y = y0;
    while (y <= y1) {
        fill_part(image, x, x0, x1, y, fill, fill_color);
        set_pixel(image, x, y, color);
        if (d <= 0) { d += incrE; y++; }
        else { d += incrNE; x++; y++; }
    }
    for (x = x0 - (thickness*sqrt(3)/4); x <= x0 + thickness*sqrt(3)/4; x++) {
        draw_line1(image, x, y0 + thickness/4, x + r/2, y1 + thickness/4, 0, -1, color, false, fill_color);
    }
    for (x = x0; x <= x0 + (thickness*sqrt(3)/4); x++) {
        draw_line1(image, x, y0 - thickness/4, x + r/2, y1 - thickness/4, 0, -1, color, false, fill_color);
    }
}

void draw_line2(struct Png* image, int x0, int y0, int x1, int y1, float thickness, int* color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int d = 2 * dy - dx;
    int incrE = 2 * dy;
    int incrNE = 2 * (dy - dx);
    int x = x0, y = y0;
    while (x <= x1) {
        if (d <= 0) { d += incrE; x++; }
        else { d += incrNE; x++; y++; }
    }
    for (x = x0; x <= x1; x++) {
        for (int y = y0 - thickness/2; y <= y0 + thickness/2; y++) {
            set_pixel(image, x, y, color);
        }
    }
}

void draw_line3(struct Png* image, int x0, int y0, int x1, int y1, float r, float thickness, int* color, bool fill, int* fill_color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int d = 2 * dx - dy;
    int incrE = 2 * dx;
    int incrNE = 2 * (dx - dy);
    int x = x0, y = y0;
    while (y >= y1) {
        fill_part(image, x, x0, x1, y, fill, fill_color);
        set_pixel(image, x, y, color);
        if (d <= 0) { d += incrE; y--; }
        else { d += incrNE; y--; x++; }
    }
    for (x = x0 - (thickness*sqrt(3)/4); x <= x0 + thickness*sqrt(3)/4; x++) {
        draw_line3(image, x, y0 - thickness/4, x + r/2, y1 - thickness/4, 0, -1, color, false, fill_color);
    }
    for (x = x0; x <= x0 + (thickness*sqrt(3)/4); x++) {
        draw_line3(image, x, y0 + thickness/4, x + r/2, y1 + thickness/4, 0, -1, color, false, fill_color);
    }
}

static bool point_in_hexagon(int x, int y, float cx, float cy, float r) {
    float dx = fabsf(x - cx);
    float dy = fabsf(y - cy);
    return dy <= (sqrtf(3.0f) * r / 2) &&
           dx <= r &&
           (sqrtf(3.0f) * dx + dy <= sqrtf(3.0f) * r);
}

void fill_hex(struct Png* image, int x0, int y0, float r, int* fill_color) {
    float h = r * sqrtf(3) / 2;
    int y_start = (int)floorf(y0 - h);
    int y_end = (int)ceilf(y0 + h);
    int x_start = (int)floorf(x0 - r);
    int x_end   = (int)ceilf(x0 + r);

    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            if (point_in_hexagon(x, y, x0, y0, r)) {
                set_pixel(image, x, y, fill_color);
            }
        }
    }
}

void draw_hexagon(struct Png* image, int x0, int y0, float r, float thickness, int* color, bool fill, int* fill_color) {
    if (image && image->row_pointers && image->png_ptr && image->info_ptr) {
        if (x0 >= 0 && y0 >= 0 && r >= 0 && thickness >= 0) {
            if (x0 < image->width && y0 < image->height) {
                if (color[0] >= 0 && color[0] <= 255 &&
                    color[1] >= 0 && color[1] <= 255 &&
                    color[2] >= 0 && color[2] <= 255) {

                    if (!fill || (fill_color[0] >= 0 && fill_color[0] <= 255 &&
                                  fill_color[1] >= 0 && fill_color[1] <= 255 &&
                                  fill_color[2] >= 0 && fill_color[2] <= 255)) {

                        int x1 = x0 + r,     y1 = y0;
                        int x2 = x0 + r / 2, y2 = y0 - r * sqrtf(3) / 2;
                        int x3 = x0 - r / 2, y3 = y0 - r * sqrtf(3) / 2;
                        int x4 = x0 - r,     y4 = y0;
                        int x5 = x0 - r / 2, y5 = y0 + r * sqrtf(3) / 2;
                        int x6 = x0 + r / 2, y6 = y0 + r * sqrtf(3) / 2;

                        if (fill) {
                            fill_hex(image, x0, y0, r, fill_color);
                        }

                        draw_line2(image, x3, y3, x2, y2, thickness, color); 
                        draw_line3(image, x4, y4, x3, y3, r, thickness, color, false, fill_color); 
                        draw_line1(image, x4, y4, x5, y5, r, thickness, color, false, fill_color); 
                        draw_line2(image, x5, y5, x6, y6, thickness, color); 
                        draw_line3(image, x6, y6, x1, y1, r, thickness, color, false, fill_color); 
                        draw_line1(image, x2, y2, x1, y1, r, thickness, color, false, fill_color);

                        plot_circle(image, x1, y1, thickness, color); 
                        plot_circle(image, x2, y2, thickness, color); 
                        plot_circle(image, x3, y3, thickness, color); 
                        plot_circle(image, x4, y4, thickness, color); 
                        plot_circle(image, x5, y5, thickness, color); 
                        plot_circle(image, x6, y6, thickness, color);
                    } else {
                        printf("Fill color values must be in the range 0–255.\n");
                        image->error_code = ERR_INVALID_COLOR_FORMAT;
                    }

                } else {
                    printf("Border color values must be in the range 0–255.\n");
                    image->error_code = ERR_INVALID_COLOR_FORMAT;
                }
            } else {
                printf("Invalid input: coordinates are outside the image bounds.\n");
                image->error_code = ERR_INVALID_COORD_FORMAT;
            }
        } else {
            printf("Invalid input: coordinates, radius, and line thickness must not be negative.\n");
            image->error_code = ERR_INVALID_COORD_FORMAT;
        }
    }

    return;
}

void copy_region(struct Png* image,
    int src_left, int src_top,
    int src_right, int src_bottom,
    int dest_left, int dest_top) {

    int width, height, channels, code = 0;
    png_bytep* buffer = NULL;
    int copy_width = 0, copy_height = 0;

    if (image && image->row_pointers && image->png_ptr && image->info_ptr) {
        width = image->width;
        height = image->height;
        channels = png_get_channels(image->png_ptr, image->info_ptr);

        if (src_right < src_left) { int tmp = src_left; src_left = src_right; src_right = tmp; }
        if (src_bottom < src_top) { int tmp = src_top; src_top = src_bottom; src_bottom = tmp; }

        copy_width = src_right - src_left;
        copy_height = src_bottom - src_top;

        if (copy_width <= 0 || copy_height <= 0) {
            fprintf(stderr, "Copy region has non-positive size.\n");
            code = ERR_INVALID_COORD_FORMAT;
        } else if (src_left < 0 || src_top < 0 || src_right > width || src_bottom > height) {
            fprintf(stderr, "Copy source coordinates out of bounds.\n");
            code = ERR_INVALID_COORD_FORMAT;
        } else if (dest_left < 0 || dest_top < 0 || dest_left >= width || dest_top >= height) {
            fprintf(stderr, "Copy destination coordinates out of bounds.\n");
            code = ERR_INVALID_COORD_FORMAT;
        } else {
            buffer = (png_bytep*)malloc(copy_height * sizeof(png_bytep));
            if (!buffer) {
                fprintf(stderr, "Memory allocation failed.\n");
                code = ERR_FILE_IO;
            } else {
                for (int y = 0; y < copy_height && code == 0; y++) {
                    buffer[y] = (png_bytep)malloc(copy_width * channels);
                    if (!buffer[y]) {
                        fprintf(stderr, "Memory allocation failed.\n");
                        code = ERR_FILE_IO;
                    } else {
                        memcpy(buffer[y],
                               &(image->row_pointers[src_top + y][src_left * channels]),
                               copy_width * channels);
                    }
                }

                if (code == 0) {
                    for (int y = 0; y < copy_height; y++) {
                        int dy = dest_top + y;
                        if (dy >= 0 && dy < height) {
                            for (int x = 0; x < copy_width; x++) {
                                int dx = dest_left + x;
                                if (dx >= 0 && dx < width) {
                                    memcpy(&(image->row_pointers[dy][dx * channels]),
                                           &(buffer[y][x * channels]),
                                           channels);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (buffer) {
            for (int y = 0; y < copy_height; y++) {
                if (buffer[y]) free(buffer[y]);
            }
            free(buffer);
        }
    }

    if (image && code != 0) {
        image->error_code = code;
    }
}