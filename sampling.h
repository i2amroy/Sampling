//
// Created by OtherPlayers on 3/2/17.
//

#ifndef GDALWORK_SAMPLING_H
#define GDALWORK_SAMPLING_H

#include "gdal_priv.h"

class sampler {
public:
    sampler(GDALDataset *raster_data);

    GByte *get_next_data(int &size);

    int get_width();

    int get_height();

    int get_blockheight();

    int get_current_j();

private:
    GDALDataset *raster_data;
    int width;
    int height;
    int blockwidth;
    int blockheight;
    int block_i_count;
    int block_j_count;
    int current_i;
    int current_j;
};

struct sample_node {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int count;
    unsigned int streak;
};

void initialize_nodes(sample_node nodes[], int count);

void swap_node_arrays(sample_node *&first_array, sample_node *&second_array);

int rand_int(int min, int max);


#endif //GDALWORK_SAMPLING_H
