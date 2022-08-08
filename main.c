#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <arm_neon.h>

#define SIZEOFBLOCK 16
#define NUMFRAMES 2
#define NUMBLOCKS 16
#define SIZEOFIMAGE 256

/*
* Struct definition
*/

typedef struct Vector
{
    int8_t x;
    int8_t y;
} Vector;

/*
* Function definitions
*/

int8_t max(int8_t val_1, int8_t val_2)
{
    return (val_1 < val_2) ? val_2 : val_1;
}

int8_t min(int8_t val_1, int8_t val_2)
{
    return (val_1 > val_2) ? val_2 : val_1;
}

void process_frame(uint8_t Frame1[NUMBLOCKS][NUMBLOCKS][SIZEOFBLOCK][SIZEOFBLOCK], 
                   uint8_t Frame2[NUMBLOCKS][NUMBLOCKS][SIZEOFBLOCK][SIZEOFBLOCK])
{
    FILE *fptr1;
    FILE *fptr2;
    uint8_t rm_header [54];

    fptr1 = fopen("test_images/Image1.bmp", "rb");
    fptr2 = fopen("test_images/Image1.bmp", "rb");

    // BMP header size is 54 bytes (8 bytes * 7 - 2 = 54 bytes)
    fread(&rm_header, sizeof(uint8_t) * 7 - 2, 1, fptr1);
    fread(&rm_header, sizeof(uint8_t)* 7 - 2, 1, fptr2);

    if(fptr1 == NULL || fptr2 == NULL)
    {
        printf("Error!");   
        exit(1);             
    }

    for (uint8_t block_row = 0; block_row < NUMBLOCKS; block_row++) {
        for (uint8_t pixel_row = 0; pixel_row < SIZEOFBLOCK; pixel_row++){
            for (uint8_t block_col = 0; block_col < NUMBLOCKS; block_col++) {
                fread(&Frame1[block_row][block_col][pixel_row], sizeof(uint8_t)*16, 1, fptr1);
                fread(&Frame2[block_row][block_col][pixel_row], sizeof(uint8_t)*16, 1, fptr2);
            }
        }
    }

    fclose(fptr1);
    fclose(fptr2);
    return;
}


void print_uint8 (uint8x16_t data) {
    int i;
    static uint8_t p[16];

    vst1q_u8 (p, data);

    for (i = 0; i < 16; i++) {
	printf ("%02d ", p[i]);
    }
    printf ("\n");
}

/*
* Main function
*/
int main(int argc, char *argv[]) 
{

    uint8_t Frame1[NUMBLOCKS][NUMBLOCKS][SIZEOFBLOCK][SIZEOFBLOCK];
    uint8_t Frame2[NUMBLOCKS][NUMBLOCKS][SIZEOFBLOCK][SIZEOFBLOCK];

    process_frame(Frame1, Frame2);

    uint32_t Differences[NUMBLOCKS][NUMBLOCKS];

    Vector vectors [NUMBLOCKS][NUMBLOCKS];

    // Initialize the Differences 2-D array
    for (int i = 0; i < NUMBLOCKS; ++i)
    {
        for (int j = 0; j < NUMBLOCKS; ++j)
        {
            Differences[i][j] = UINT32_MAX;
        }
    }
    
    // Initialize the frame arrays
    process_frame(Frame1, Frame2);


    // for (int i = 0; i < NUMBLOCKS; ++i)
    // {
    //     for (int j = 0; j < NUMBLOCKS; ++j)
    //     {
    //         for (int k = 0; k < SIZEOFBLOCK; ++k)
    //         {
    //             printf("Frame 1: ");
    //             for (int s = 0; s < SIZEOFBLOCK; ++s)
    //             {
    //                 printf("%d ", Frame1[i][j][k][s]);
    //             }
    //             printf("\n");

    //             printf("Frame 2: ");
    //             for (int s = 0; s < SIZEOFBLOCK; ++s)
    //             {
    //                 printf("%d ", Frame2[i][j][k][s]);
    //             }
    //             printf("\n");
    //         }
    //     }
    // }

    // return 0;

    // Start calculating the SAD values
    for (uint8_t block_row_ref = 0; block_row_ref < NUMBLOCKS; ++block_row_ref) // every row in frame (block)
    {
        for (uint8_t block_col_ref = 0; block_col_ref < NUMBLOCKS; ++block_col_ref) // every column in frame (block)
        {
            for (uint8_t block_row_comp = max(0, block_row_ref - 3); block_row_comp < min(NUMBLOCKS, block_row_ref + 3); ++block_row_comp) // every block row in other frame
            {
                // printf("block_row_ref: %d, block_row_comp: %d\n", block_row_ref, block_row_comp);
                for (uint8_t block_col_comp = max(0, block_col_ref - 3); block_col_comp < min(NUMBLOCKS, block_col_ref + 3); ++block_col_comp) // every block column in other frame
                {
                    uint32_t temp_sad = 0;
                    for (uint8_t pixel_row = 0; pixel_row < SIZEOFBLOCK; ++pixel_row) // every row in cur_block (cur_pixel)
                    {
                        uint8x16_t vector_ref; // declare a vector of 16 8-bit lanes
                        uint8x16_t vector_comp; // declare a vector of 16 8-bit lanes
                        vector_ref = vld1q_u8(Frame1[block_row_ref][block_col_ref][pixel_row]); // load the array from memory into a vector
                        vector_comp = vld1q_u8(Frame2[block_row_comp][block_row_comp][pixel_row]); // load the array from memory into a vector
                        
                        if (block_row_ref == block_row_comp && block_col_ref == block_col_comp)
                        {
                            printf("reference vector:\n");
                            print_uint8(vector_ref);
                            printf("\n*****\n\n");
                            printf("comparison vector:\n");
                            print_uint8(vector_comp);
                        }

                        // Perform the Absolute Differences operation:
                        uint8x16_t init_result;
                        init_result = vabdq_u8(vector_ref, vector_comp);

                        // Store first and second halves of the result vector in two different vectors of half size
                        uint8x8_t result_high = vget_high_u8( init_result );
                        uint8x8_t result_low = vget_low_u8( init_result );

                        // Perform vector addition with the high and low vectors
                        uint16x8_t final_result = vaddl_u8( result_high, result_low);

                        // Sum all elements in the result vector by reading the lanes individually
                        temp_sad += vgetq_lane_u8(init_result, 0);
                        temp_sad += vgetq_lane_u8(init_result, 1);
                        temp_sad += vgetq_lane_u8(init_result, 2);
                        temp_sad += vgetq_lane_u8(init_result, 3);
                        temp_sad += vgetq_lane_u8(init_result, 4);
                        temp_sad += vgetq_lane_u8(init_result, 5);
                        temp_sad += vgetq_lane_u8(init_result, 6);
                        temp_sad += vgetq_lane_u8(init_result, 7);
                        temp_sad += vgetq_lane_u8(init_result, 8);
                        temp_sad += vgetq_lane_u8(init_result, 9);
                        temp_sad += vgetq_lane_u8(init_result, 10);
                        temp_sad += vgetq_lane_u8(init_result, 11);
                        temp_sad += vgetq_lane_u8(init_result, 12);
                        temp_sad += vgetq_lane_u8(init_result, 13);
                        temp_sad += vgetq_lane_u8(init_result, 14);
                        temp_sad += vgetq_lane_u8(init_result, 15);

                    }
                    if (block_row_ref == block_row_comp && block_col_ref == block_col_comp)
                    {
                        printf("temp_sad value: %d\n", temp_sad);
                    }
                    if (Differences[block_row_ref][block_col_ref] > temp_sad )
                    {
                        Differences[block_row_ref][block_col_ref] = temp_sad;
                        vectors[block_row_ref][block_col_ref].x = block_col_comp - block_col_ref;
                        vectors[block_row_ref][block_col_ref].y = block_row_ref - block_row_comp;
                    }
                }
            }
        }
    }

    
    for (int i = 0; i < NUMBLOCKS; ++i)
    {
        for (int j = 0; j < NUMBLOCKS; ++j)
        {
            int temp_diff = Differences[i][j];
            int temp_x = vectors[i][j].x;
            int temp_y = vectors[i][j].y;
            printf("Block[%d][%d]: Vector: (%d, %d); Difference: %d\n", i, j, temp_x, temp_y, temp_diff);
        }
    }

    return 0;
}