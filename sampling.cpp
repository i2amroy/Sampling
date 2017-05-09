//
// Created by OtherPlayers on 3/2/17.
//

#include "sampling.h"

void initialize_nodes(sample_node nodes[], int count) {
    for (int i = 0; i < count; i++) {
        nodes[i].red = 0;
        nodes[i].green = 0;
        nodes[i].blue = 0;
        nodes[i].count = 0;
        nodes[i].streak = 1;
    }
}

void swap_node_arrays(sample_node *&first_array, sample_node *&second_array) {
    sample_node *tmp = first_array;
    first_array = second_array;
    second_array = tmp;
}

sampler::sampler(GDALDataset *raster_data) : raster_data(raster_data), current_i(-1), current_j(0) {
    GDALRasterBand *red = raster_data->GetRasterBand(1);
    // At this point we're assuming all three bands are the same size
    width = red->GetXSize();
    height = red->GetYSize();
    red->GetBlockSize(&blockwidth, &blockheight);
    block_i_count = (width + blockwidth - 1) / blockwidth;
    block_j_count = (height + blockheight - 1) / blockheight;
}

GByte *sampler::get_next_data(int &size) {
    current_i++;
    if (current_i == block_i_count) {
        // Else reset our column offset and move to the next row
        current_i = 0;
        current_j += 1;
    }
    // If we've hit the very end then return 0 for our sizes and null for our data
    if (current_j == block_j_count) {
        size = 0;
        return NULL;
    }
    size = blockwidth * blockheight * 3;
    // Return the RGB data interleaved
    GByte *band_data = (GByte *) CPLMalloc((size_t) blockwidth * blockheight * 3);
    CPLErr ecode = raster_data->RasterIO(GF_Read, current_i * blockwidth, current_j * blockheight,
                                         blockwidth, blockheight, band_data, blockwidth,
                                         blockheight, GDT_Byte, 3, NULL, 3, 3 * blockwidth, 1);
    if (ecode == CE_Failure) {
        printf("Error reading RasterIO");
        exit(2);
    }
    return band_data;
}

void sampler::reset_sampling() {
    current_i = 0;
    current_j = 0;
}

int sampler::get_width() {
    return width;
}

int sampler::get_height() {
    return height;
}

int sampler::get_blockwidth() {
    return blockwidth;
}

int sampler::get_blockheight() {
    return blockheight;
}

int sampler::get_current_i() {
    return current_i;
}

int sampler::get_current_j() {
    return current_j;
}

int rand_int(int min, int max) {
    if (min == max) {
        return min;
    }
    if (min > max) {
        std::swap(min, max);
    }
    return rand() % (max - min) + min;
}