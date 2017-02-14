/*
 *       uarray2b.c
 *       Conor Jenkinson & Sydney Strzempko, 9 Oct 2016
 *       HW3
 *
 *       Implementation file for the UArray2b blocked 2-D array struct. Has two
 *       initialization options, with blocksize determined by the client, or if
 *       omitted, generated to largest-possible blocksize while maintaining a
 *       square space of less than 64KB, otherwise generated to a standardized
 *       blocksize of 1. Other functions include mapping, which follows a row-
 *       major larger map of each block (internally also following a row-major
 *       traversal across each block), and at, which returns a void pointer.
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <except.h>
#include <mem.h>
#include <assert.h>
#include "uarray2b.h"
#include "uarray2.h"
#include <uarray.h>
#define T UArray2b_T

const Except_T OUT_OF_RANGE = {"Coordinates in U2B_at were out of range"};
const int MAX_BLOCK_B = 64000;/* MAX blocksize is 64kb */
const int STD_BLOCKSIZE = 1;

struct T /*UArray2b_T struct implementation*/
{
        int width;/*UNDERLYING WIDTH - IE, LAST VALUE*/
        int height;
        int size;/*SIZE of 1 cell*/
        int blocksize;
        int blocks;
        UArray2_T uarray2;
};
/*
 * NEW FUNCTIONS - functionality with blocksize included (standard _new) or
 * omitted (_new_64k_block). Uses MALLOC to generate space on the heap
 */
T    UArray2b_new (int width, int height, int size, int blocksize)
{
        assert(blocksize > 0);/*Checks that client has passed in a valid bsize*/
        int j;
        int i = 0;
       
        T uarray2b = malloc(sizeof(struct UArray2b_T));/*MALLOCATION HERE*/
        if (width>=height) {
                while (i*blocksize < width ) {
                        i++;
                }
        } else {
                while (i*blocksize < height ) {
                        i++;
                }
        }
        uarray2b->blocks = i;
        uarray2b->width = width;/*WIDTH here indicates underlying width*/
        uarray2b->height = height;/*Same for height*/
        uarray2b->size = size;/*SIZE is the byte space of a single unit*/
        uarray2b->blocksize = blocksize;/*dimensions of a block in array*/
        /*To get 1 block size*/
        uarray2b->uarray2 = UArray2_new(i, i, sizeof(UArray2_T));
       
        for (i = 0; i < uarray2b->blocks; i++) {
                for (j = 0; j < uarray2b->blocks; j++) {
                        UArray_T *block = UArray2_at(uarray2b->uarray2, i, j);
                        *(block) = UArray_new(blocksize*blocksize, size);
                }
        }

        return uarray2b;/*RETURNS initialized version of the struct*/
}

T    UArray2b_new_64K_block(int width, int height, int size)
{
        /*DETERMINES BLOCKSIZE NEEDED*/
        int blocksize = width * height;
        if(blocksize > MAX_BLOCK_B){
        blocksize = STD_BLOCKSIZE;
        }
        T uarray2b = UArray2b_new(width,height,size,blocksize);
       
        return uarray2b;
}

int   UArray2b_width(T array2b)
{
        return array2b->width;
}
int   UArray2b_height(T array2b)
{
        return array2b->height;
}
int   UArray2b_size(T array2b)
{
        return array2b->size;
}
int   UArray2b_blocksize(T array2b)
{
        return array2b->blocksize;
}
/*****************************************************************************/
void *UArray2b_at(T array2b, int column, int row)
{      
        /*THIS STATEMENT ensures that the (col,row)coords arent outside range*/
        if (!((column < array2b->width) || (row < array2b->height))) {
                RAISE(OUT_OF_RANGE);
                exit(0);
        }      
        void *block = NULL;/*Void pointer used for Block search*/
        void *cell = NULL;/*Void pointer used for cell search*/
        int bcol, brow;/*Block coordinates - used to fetch cell within block*/
        int c_c;/*Cell coordinates within u2 array (a block) */
       
        bcol = column/(array2b->blocksize);/* MATH provided courtesy of spec*/
        brow = row/(array2b->blocksize);
        c_c = (array2b->blocksize) * (column % array2b->blocksize) +
              (row % array2b->blocksize);
       
        block = UArray2_at(array2b->uarray2, bcol, brow);/*Pt to block*/
        cell = UArray_at(*(UArray_T *)block, c_c);/*Pt to cell in blck*/
       
        return cell;
}

void  UArray2b_map(T array2b,
                void apply(int col, int row, T array2b, void *elem, void *cl),
                void *cl)
{
        int bcol,brow, i, block_len,col,row;
        void *block = NULL;
        void *cell = NULL;

        for (bcol = 0; bcol < array2b->blocks; bcol++) {
                for(brow = 0; brow < array2b->blocks; brow++) {
                        /*This establishes a row-mapped traversal thru blocks*/
                        block = UArray2_at(array2b->uarray2, bcol, brow);
                        block_len = (array2b->blocksize)*(array2b->blocksize);

                        for (i = 0; i < block_len; i++) {
                                /*THESE MATH VALUES DERIVED BY US PERSONALLY*/
                                col = ((bcol*array2b->blocksize) +
                                       (i % array2b->blocksize));
                                row = ((brow*array2b->blocksize) +
                                       (i / array2b->blocksize));
                                /*Now we can decide if we want to call apply*/
                                if ((col < array2b->width) &&
                                    (row < array2b->height)) {
                                        cell = UArray_at(*((UArray_T *)block),
                                                                        i);
                                        apply(col, row, array2b, cell, cl);
                                }
                        }
                }
        }
}

void   UArray2b_free(T *array2b)
{
        int bcol, brow;
        for (bcol = 0; bcol < (*array2b)->blocks; bcol++) {
                for(brow = 0; brow < (*array2b)->blocks; brow++) {
                        UArray_T *block = UArray2_at((*array2b)->uarray2,
                                                    bcol, brow);
                        UArray_free(block);/*Frees every blocked Uarray*/
                }
        }
        UArray2_free(&((*array2b)->uarray2));
        FREE(*array2b);
}


#undef T
