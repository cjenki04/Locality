/*
 *  ppmtrans.c
 *  Conor Jenkinson & Sydney Strzempko, 9 Oct 2016
 *  HW3
 * 
 *  Program described in Part C of the spec doc; a program with image
 *  manipulation and performance assessment bundled into command-line
 *  arguments. The program offers the following options as arguments;
 *
 *      -rotate 0   This series of arguments perform corresponding
 *      -rotate 90  degree rotations of the original image, before
 *      -rotate 180 printing the manipulated image to stdout
 *
 *      -time <tofile>  This argument generates timing data for the
 *              options it is used in conjunction with, and
 *              stores program performance output to the
 *              indicated file for analysis

 *      -row-major  This next series of arguments determine HOW
 *      -col-major  pixels are copied from the source image using
 *      -block-major    their respective A2Methods built-in mapping
 *              functions - this distinction determines whether
 *              an underlying UArray2 or UArray2b is used to
 *              represent image by the outerlying method suite
 *
 *  An example command-line argument could be inputted as followed;
 *
 *      "./ppmtrans -block-major -rotate 90 -time timedata.txt image.ppm"
 *
 *  The program also supports bypassing of image-filename (last argument
 *  above) in order to input the data (in PPM format) directly to stdin.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include <except.h>
#include "pnm.h"
#include "cputiming.h"
#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)
#define A2 A2Methods_UArray2
/**************************PROGRAM****FUNCTIONS********************************/
/* Main rotation function which designates which manipulation-function to call*/
void rotate_image(Pnm_ppm pixmap,
          Pnm_ppm rpixmap,
          A2Methods_mapfun *map,
          int rotation,
          char *time_file_name);
void apply_90(int col,/* 90-degree rotation helper function */
          int row,
          A2 a2,
          A2Methods_Object *elem,
          void *cl);
void apply_180(int col,/* 180-degree rotation helper function */
           int row,
           A2 a2,
           A2Methods_Object *elem,
           void *cl);
void apply_270(int col,/* 270-degree rotation helper function */
           int row,
           A2 a2,
           A2Methods_Object *elem,
           void *cl);
void apply_horizontal(int col,/* horizontal flip helper function */
              int row,
              A2 a2,
              A2Methods_Object *elem,
              void *cl);
void flip_image(Pnm_ppm pixmap,/* flip-image helper function */
        Pnm_ppm rpixmap, 
        A2Methods_mapfun *map,
        char *flip,
        char *time_file_name);
void transpose_image(Pnm_ppm pixmap,/* transposition helper function*/
             Pnm_ppm rpixmap,
             A2Methods_mapfun *map,
             char *time_file_name);
void apply_vertical(int col,/* inner mapped-apply vertical helper function */
            int row,
            A2 a2,
            A2Methods_Object *elem,
            void *cl);
void apply_transpose(int col,/* inner mapped-apply transpose helper function */
             int row,
             A2 a2,
             A2Methods_Object *elem,
             void *cl);
void write_to_file(Pnm_ppm pixmap,/* PART E helper function - prints results*/
           A2Methods_mapfun *map,
           double time_used,
           int rotation,
           char *time_file_name);
/*****************************************************************************/
static void usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

int main(int argc, char *argv[])
{      

        bool stdin_file = true;
        bool transpose = false;
        char *time_file_name = NULL;
        int   rotation       = 0;
        int   i;
        char *flip = NULL;
        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods);

        /* default to best map */
        A2Methods_mapfun *map = methods->map_default;
        assert(map);


        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain,
                    map_row_major,
                    "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain,
                    map_col_major,
                    "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked,
                    map_block_major,
                                    "block-major");
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {      /* no rotate value */
                                usage(argv[0]);
                        }
                        char *endptr;
                        rotation = strtol(argv[++i], &endptr, 10);
                        if (!(rotation == 0 || rotation == 90 ||
                            rotation == 180 || rotation == 270)) {
                                fprintf(stderr,
                    "Rotation must be 0, 90 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    /* Not a number */
                                usage(argv[0]);
                        }
                } else if (strcmp(argv[i], "-time") == 0) {
                        time_file_name = argv[++i];
                } else if (strcmp(argv[i], "-flip") == 0){
                    flip = argv[++i];
                } else if (strcmp(argv[i], "-transpose") == 0){
                    transpose = true;
                } else if (*argv[i] == '-') {
                        fprintf(stderr,
                "%s: unknown option '%s'\n",
                argv[0],
                argv[i]);
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        stdin_file = false;
                        break;
                }
        }
        FILE *inputfp;
        if (stdin_file) {
            inputfp = stdin;
        } else {
            inputfp = fopen(argv[i], "r");
        }
        Pnm_ppm pixmap ;
        pixmap = Pnm_ppmread(inputfp, methods);
        assert(pixmap);
        Pnm_ppm rpixmap = malloc(sizeof(struct Pnm_ppm));
        assert(rpixmap);
        if ((rotation == 90) || (rotation == 270) || (transpose)){
          rpixmap->height = pixmap->width; 
          rpixmap->width = pixmap->height;
        } else{
          rpixmap->height = pixmap->height;
          rpixmap->width = pixmap->width;  
        }
        rpixmap->pixels = methods->new(rpixmap->width,
                       rpixmap->height,
                       methods->size(pixmap->pixels));
        rpixmap->methods = methods;
        rpixmap->denominator = pixmap->denominator;
        if (transpose){   /*finds argument to apply*/
            rotation = 1;          
            transpose_image(pixmap, rpixmap, map, time_file_name);
        }
        else if (flip != NULL){
            rotation = 1;
            flip_image(pixmap, rpixmap, map, flip, time_file_name);
        } else{
            rotate_image(pixmap, rpixmap, map, rotation, time_file_name);
        }   
        if(rotation == 0){
            Pnm_ppmwrite(stdout, pixmap);/*writes initial 2c array*/
            if (time_file_name != NULL){
              write_to_file(pixmap,map,0.0,rotation,time_file_name);
            }
        }else{
            Pnm_ppmwrite(stdout, rpixmap);
        }
        Pnm_ppmfree(&pixmap);
        Pnm_ppmfree(&rpixmap);
        fclose(inputfp);
}
void rotate_image(Pnm_ppm pixmap,
          Pnm_ppm rpixmap, 
          A2Methods_mapfun *map,
          int rotation,
          char *time_file_name )
{
    if (rotation == 90) {
       if (time_file_name != NULL) { /*-time has been called*/
            CPUTime_T timer; 
            double time_used;
            timer = CPUTime_New();
            CPUTime_Start(timer);
            map(pixmap->pixels, apply_90,(void *) rpixmap);
            time_used = CPUTime_Stop(timer);
            CPUTime_Free(&timer);
            write_to_file(pixmap,map,time_used,rotation,time_file_name);
        } else {
            map(pixmap->pixels, apply_90,(void *) rpixmap);
        }
    } else if (rotation == 180) {
        if (time_file_name != NULL) {
        CPUTime_T timer;
        double time_used;
        timer = CPUTime_New();
        CPUTime_Start(timer);
        map(pixmap->pixels, apply_180,(void *) rpixmap);
        time_used = CPUTime_Stop(timer);
        CPUTime_Free(&timer);
        write_to_file(pixmap,map, time_used, rotation, time_file_name);
        } else {
            map(pixmap->pixels, apply_180,(void *) rpixmap);
        }  
    } else if (rotation == 270) {
        if (time_file_name != NULL) {
            CPUTime_T timer;
            double time_used;
            timer = CPUTime_New();
            CPUTime_Start(timer);
            pixmap->methods->map_default(pixmap->pixels,
                             apply_270,
                             (void *) rpixmap);
            time_used = CPUTime_Stop(timer);
            CPUTime_Free(&timer);
            write_to_file(pixmap,map,time_used,rotation,time_file_name);
        } else {
            pixmap->methods->map_default(pixmap->pixels,
                             apply_270,
                             (void *) rpixmap);
        }
    }
}

void apply_90(int col, int row, A2 a2, A2Methods_Object *elem, void *cl)
{
    int height;
    struct Pnm_rgb *relem;
    
    Pnm_ppm pixmap = (Pnm_ppm )cl;
    height = pixmap->methods->height(a2);
    relem = (struct Pnm_rgb *)(pixmap->methods->at(pixmap->pixels,
                               height-row-1,
                               col));
    *relem = *(Pnm_rgb)elem;
}

void apply_180(int col, int row, A2 a2, A2Methods_Object *elem, void *cl)
{
    struct Pnm_rgb *relem;
    
    Pnm_ppm pixmap = (Pnm_ppm )cl;
    (void)a2;
    relem = (struct Pnm_rgb *)(pixmap->methods->at(pixmap->pixels,
                             pixmap->width - col - 1,
                             pixmap->height - row - 1));
    *relem = *(Pnm_rgb)elem;
}

void apply_270(int col, int row, A2 a2, A2Methods_Object *elem, void *cl)
{
    int width;
    struct Pnm_rgb *relem ;
    
    Pnm_ppm pixmap = (Pnm_ppm)cl;
    width = pixmap->methods->width(a2);
    relem = (struct Pnm_rgb *)(pixmap->methods->at(pixmap->pixels,
                               row,
                               width-col-1));
    *relem = *(Pnm_rgb)elem;
}

void flip_image(Pnm_ppm pixmap,
        Pnm_ppm rpixmap,
        A2Methods_mapfun *map,
        char *flip,
        char *time_file_name)
{
    
    if (strcmp(flip, "horizontal") == 0) {
        if (time_file_name != NULL) {
            CPUTime_T timer;
            double time_used;
            timer = CPUTime_New();
            CPUTime_Start(timer);
            map(pixmap->pixels,apply_horizontal, (void *)rpixmap);
            time_used = CPUTime_Stop(timer);
            CPUTime_Free(&timer);
            write_to_file(pixmap, map, time_used, 1, time_file_name);
        } else {
            map(pixmap->pixels,apply_horizontal, (void *)rpixmap);
        }
    }
    else if (strcmp(flip, "vertical") == 0){
        if (time_file_name != NULL) {
            CPUTime_T timer;
            double time_used;
            timer = CPUTime_New();
            CPUTime_Start(timer);
            map(pixmap->pixels,apply_vertical, (void *)rpixmap);
            time_used = CPUTime_Stop(timer);
            CPUTime_Free(&timer);
            write_to_file(pixmap, map, time_used, 2, time_file_name);
        } else {
            map(pixmap->pixels,apply_vertical, (void *)rpixmap);
        }
    } else {
        fprintf(stderr, "Flip must be horizontal or vertical\n");
    }
}

void apply_horizontal(int col, int row, A2 a2, 
              A2Methods_Object *elem, void *cl)
{
    struct Pnm_rgb *relem;
    
    Pnm_ppm pixmap = (Pnm_ppm )cl;
    (void)a2;
    relem = (struct Pnm_rgb*)(pixmap->methods->at(pixmap->pixels,
                              pixmap->width - col - 1,
                              row));
    *relem = *(Pnm_rgb)elem;
}

void apply_vertical(int col, int row, A2 a2, A2Methods_Object *elem, void *cl)
{
    struct Pnm_rgb *relem;

    Pnm_ppm pixmap = (Pnm_ppm )cl;
    (void)a2;
    relem = (struct Pnm_rgb*)(pixmap->methods->at(pixmap->pixels,
                             col,
                             pixmap->height - row - 1));
    *relem = *(Pnm_rgb)elem;
}

void transpose_image(Pnm_ppm pixmap, Pnm_ppm rpixmap,
                    A2Methods_mapfun *map, char *time_file_name)
{
    if (time_file_name != NULL) {
        CPUTime_T timer;
        double time_used;
        timer = CPUTime_New();
        CPUTime_Start(timer);
        map(pixmap->pixels, apply_transpose, (void *)rpixmap);
        time_used = CPUTime_Stop(timer);
        CPUTime_Free(&timer);
        write_to_file(pixmap, map, time_used, 3, time_file_name);
    } else {
        pixmap->methods->map_default(pixmap->pixels,
                         apply_transpose,
                         (void *)rpixmap);
    }
}

void apply_transpose(int col, int row, A2 a2, A2Methods_Object *elem, void *cl)
{
    struct Pnm_rgb *relem;
    Pnm_ppm pixmap = (Pnm_ppm )cl;
    (void)a2;
    relem = (struct Pnm_rgb*)(pixmap->methods->at(pixmap->pixels, row, col));
    *relem = *(Pnm_rgb)elem;
}

void write_to_file(Pnm_ppm pixmap,
           A2Methods_mapfun *map,
           double time_used,
           int rotation,
           char *time_file_name)
{
    FILE *timings_file = fopen(time_file_name, "a");/*Opens timing filestream*/
    fprintf(timings_file, "METHOD USED:\n");
    if (pixmap->methods == uarray2_methods_blocked) {
        fprintf(timings_file, "UArray2b\n");
    } else {
        fprintf(timings_file, "UArray2\n");
    }
    if(map == pixmap->methods->map_row_major){
      fprintf(timings_file, "MAP ROW MAJOR\n" );
    } 
    if(map == pixmap->methods->map_col_major){
      fprintf(timings_file, "MAP COL MAJOR\n" );
    } 
    if(map == pixmap->methods->map_block_major){
      fprintf(timings_file, "MAP BLOCK MAJOR\n" );
    } 
    if (rotation == 1) {
        fprintf(timings_file, "Horizontal Flip\n");
    } else if (rotation == 2) {
        fprintf(timings_file, "Vertical Flip\n");
    } else if (rotation == 3) {
        fprintf(timings_file, "Transpose\n");
    } else {
        fprintf(timings_file, "Rotation: %i\n", rotation );
    }
    fprintf(timings_file, "TOTAL TIME: %f\n", time_used );
    double time_per_pixel = time_used / (pixmap->height * pixmap->width);
    fprintf(timings_file, "TIME / PIXEL: %f\n", time_per_pixel );
    fprintf(timings_file, "SIZE : %i Pixels\n", pixmap->height * 
                                                pixmap -> width);
    fclose(timings_file);
}

#undef A2
