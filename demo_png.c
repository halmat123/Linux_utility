#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "utils.h"
#include <png.h>

void print_help() {
    printf("Course work for option 4.21, created by Yuliya Khalmetova.\n\n");
    printf("Options:\n");
    printf("  -h, --help                Show this help message\n");
    printf("  -i, --info                Print PNG file info\n");
    printf("  -o, --output FILE         Specify output file name (default: out.png)\n");
    printf("      --rect                Draw rectangle\n");
    printf("      --left_up X.Y         Top-left corner of rectangle or source\n");
    printf("      --right_down X.Y      Bottom-right corner of rectangle or source\n");
    printf("      --color R.G.B         Border color\n");
    printf("      --fill                Fill the shape\n");
    printf("      --fill_color R.G.B    Fill color\n");
    printf("      --hexagon             Draw hexagon\n");
    printf("      --center X.Y          Center of hexagon\n");
    printf("      --radius N            Radius of hexagon\n");
    printf("      --copy                Copy region\n");
    printf("      --dest_left_up X.Y    Destination point\n");
    printf("      --thickness N         Line thickness\n");
    printf("      --input FILE          Input PNG file\n");
}

int parse_coord_pair(const char *arg, int *x, int *y) {
    return sscanf(arg, "%d.%d", x, y) == 2 && *x >= 0 && *y >= 0;
}

int main(int argc, char *argv[]) {
    int code = 0;

    struct Png image = {0};
    char *input_file = NULL;
    char *output_file = "out.png";
    int do_info = 0, do_rect = 0, do_hex = 0, do_copy = 0;
    int fill = 0, thickness = 1;

    int rect_x1 = -1, rect_y1 = -1, rect_x2 = -1, rect_y2 = -1;
    int center_x = -1, center_y = -1, radius = -1;
    int src_x1 = -1, src_y1 = -1, src_x2 = -1, src_y2 = -1;
    int dest_x = -1, dest_y = -1;

    struct Color border_color = {0, 0, 0};
    struct Color fill_color = {255, 255, 255};

    if (argc == 1) {
        print_help();
    } else {
        static struct option long_options[] = {
            {"help",         no_argument,       NULL, 'h'},
            {"info",         no_argument,       NULL, 'i'},
            {"output",       required_argument, NULL, 'o'},
            {"rect",         no_argument,       NULL, OPT_RECT},
            {"left_up",      required_argument, NULL, OPT_LEFT_UP},
            {"right_down",   required_argument, NULL, OPT_RIGHT_DOWN},
            {"color",        required_argument, NULL, OPT_COLOR},
            {"fill",         no_argument,       NULL, OPT_FILL},
            {"fill_color",   required_argument, NULL, OPT_FILL_COLOR},
            {"hexagon",      no_argument,       NULL, OPT_HEXAGON},
            {"center",       required_argument, NULL, OPT_CENTER},
            {"radius",       required_argument, NULL, OPT_RADIUS},
            {"copy",         no_argument,       NULL, OPT_COPY},
            {"dest_left_up", required_argument, NULL, OPT_DEST_LEFT_UP},
            {"thickness",    required_argument, NULL, OPT_THICKNESS},
            {"input",        required_argument, NULL, OPT_INPUT},
            {0, 0, 0, 0}
        };

        int opt, long_index = 0;
        opterr = 0;

        while ((opt = getopt_long(argc, argv, "hio:", long_options, &long_index)) != -1) {
            switch (opt) {
                case 'h':
                    print_help();
                    return 0;
                case 'i': do_info = 1; break;
                case 'o': output_file = optarg; break;
                case OPT_RECT: do_rect = 1; break;
                case OPT_LEFT_UP: {
                    int x, y;
                    if (!parse_coord_pair(optarg, &x, &y)) {
                        fprintf(stderr, "Error: Invalid --left_up format. Expected X.Y\n");
                        code = ERR_INVALID_COORD_FORMAT;
                    } else {
                        rect_x1 = x;
                        rect_y1 = y;
                        src_x1 = x;
                        src_y1 = y;
                    }
                    break;
                }
                case OPT_RIGHT_DOWN: {
                    int x, y;
                    if (!parse_coord_pair(optarg, &x, &y)) {
                        fprintf(stderr, "Error: Invalid --right_down format. Expected X.Y\n");
                        code = ERR_INVALID_COORD_FORMAT;
                    } else {
                        rect_x2 = x;
                        rect_y2 = y;
                        src_x2 = x;
                        src_y2 = y;
                    }
                    break;
                }               
                case OPT_COLOR: {
                    int r, g, b;
                    if (sscanf(optarg, "%d.%d.%d", &r, &g, &b) != 3 || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                        fprintf(stderr, "Invalid --color format.\n");
                        code = ERR_INVALID_COLOR_FORMAT;
                    } else {
                        border_color.r = r;
                        border_color.g = g;
                        border_color.b = b;
                    }
                    break;
                }
                case OPT_FILL: fill = 1; break;
                case OPT_FILL_COLOR: {
                    int r, g, b;
                    if (sscanf(optarg, "%d.%d.%d", &r, &g, &b) != 3 || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                        fprintf(stderr, "Error: Invalid format for --fill_color, expected R.G.B\n");
                        code = ERR_INVALID_COLOR_FORMAT;
                    } else {
                        fill_color.r = r;
                        fill_color.g = g;
                        fill_color.b = b;
                    }
                    break;
                }
                case OPT_HEXAGON: do_hex = 1; break;
                case OPT_CENTER:
                    if (!parse_coord_pair(optarg, &center_x, &center_y)) {
                        fprintf(stderr, "Invalid value for --center\n");
                        code = ERR_INVALID_HEX_ARGS;
                    }
                    break;
                case OPT_RADIUS:
                    radius = atoi(optarg);
                    if (radius <= 0) {
                        fprintf(stderr, "Invalid value for --radius\n");
                        code = ERR_INVALID_HEX_ARGS;
                    }
                    break;
                case OPT_COPY: do_copy = 1; break;
                case OPT_DEST_LEFT_UP:
                    if (!parse_coord_pair(optarg, &dest_x, &dest_y)) {
                        fprintf(stderr, "Invalid value for --dest_left_up\n");
                        code = ERR_INVALID_COORD_FORMAT;
                    }
                    break;
                case OPT_THICKNESS:
                    thickness = atoi(optarg);
                    if (thickness <= 0) {
                        fprintf(stderr, "Invalid value for --thickness\n");
                        code = ERR_INVALID_THICKNESS;
                    }
                    break;
                case OPT_INPUT: input_file = optarg; break;
                default: 
                    fprintf(stderr, "Error: unknown option: %s\n", argv[optind - 1]);
                    code = ERR_UNKNOWN_OPTION;
                    break;
            }
        }

        if (optind < argc) {
            fprintf(stderr, "Error: unknown argument: %s\n", argv[optind]);
            code = ERR_UNKNOWN_OPTION;
        }

        if (!input_file && code == 0) {
            fprintf(stderr, "Error: input PNG file must be provided.\n");
            code = ERR_MISSING_INPUT_FILE;
        }

        if (output_file && input_file && strcmp(output_file, input_file) == 0 && code == 0) {
            fprintf(stderr, "Error: input and output file must differ.\n");
            code = ERR_SAME_INPUT_OUTPUT;
        }

        int actions = do_rect + do_hex + do_copy + do_info;
        if (actions != 1 && code == 0) {
            fprintf(stderr, "Error: only one action can be performed.\n");
            code = ERR_MULTIPLE_ACTIONS;
        }

        if (code == 0) {
            code = read_png_file(input_file, &image);
            if (code == 0) {
                if (do_info) {
                    printf("=== PNG Image Information ===\n");
                    printf("Image size: %dx%d pixels\n", image.width, image.height);
                    printf("Color type: ");
                    switch (image.color_type) {
                        case PNG_COLOR_TYPE_GRAY:       printf("Grayscale\n"); break;
                        case PNG_COLOR_TYPE_PALETTE:    printf("Palette-based (indexed)\n"); break;
                        case PNG_COLOR_TYPE_RGB:        printf("RGB\n"); break;
                        case PNG_COLOR_TYPE_RGB_ALPHA:  printf("RGB with alpha (RGBA)\n"); break;
                        case PNG_COLOR_TYPE_GRAY_ALPHA: printf("Grayscale with alpha\n"); break;
                        default:                        printf("Unknown (%d)\n", image.color_type); break;
                    }
                    printf("Bit depth: %d bits per channel\n", image.bit_depth);
                } else {
                    if (do_rect) {
                        if (rect_x1 == -1 || rect_y1 == -1 || rect_x2 == -1 || rect_y2 == -1) {
                            fprintf(stderr, "Error: Missing or invalid rectangle coordinates (--left_up and --right_down must be provided).\n");
                            code = ERR_INVALID_COORD_FORMAT;
                        } else {
                            image.draw_rectangle = 1;
                            image.rect_border_color = border_color;
                            image.rect_fill_color = fill_color;
                            image.rect_fill = fill;
                            image.rect_thickness = thickness;
                            image.rect_left = rect_x1;
                            image.rect_up = rect_y1;
                            image.rect_right = rect_x2;
                            image.rect_down = rect_y2;
                        }
                    }

                    if (do_hex && code == 0) {
                        if (center_x == -1 || center_y == -1 || radius == -1) {
                            fprintf(stderr, "Error: --center and --radius must be provided for hexagon.\n");
                            code = ERR_INVALID_COORD_FORMAT;
                        } else {
                            image.draw_hexagon = 1;
                            image.hex_center_x = center_x;
                            image.hex_center_y = center_y;
                            image.hex_radius = radius;
                            image.hex_thickness = thickness;
                            image.hex_fill_color = fill_color;
                            image.hex_border_color = border_color;
                            image.hex_fill = fill;
                        }
                    }

                    if (do_copy && code == 0) {
                        if (src_x1 == -1 || src_y1 == -1 || src_x2 == -1 || src_y2 == -1 || dest_x == -1 || dest_y == -1) {
                            fprintf(stderr, "Error: --left_up, --right_down and --dest_left_up must be provided for copy.\n");
                            code = ERR_INVALID_COORD_FORMAT;
                        } else {
                            image.copy_mode = 1;
                            image.src_left = src_x1;
                            image.src_top = src_y1;
                            image.src_right = src_x2;
                            image.src_bottom = src_y2;
                            image.dest_left = dest_x;
                            image.dest_top = dest_y;
                        }
                    }

                    if (code == 0) {
                        process_file(&image);
                        if (image.error_code) {
                            code = image.error_code;
                        } else {
                            code = write_png_file(output_file, &image);
                        }
                    }
                }
            }
        }
    }

    free_image(&image);
    return code;
}