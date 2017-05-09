//
// Created by Christian Buskirk on 3/3/17.
//

#ifndef GDALWORK_PPM_H
#define GDALWORK_PPM_H

#include <fstream>
#include "sampling.h"

void write_p6(std::ofstream &outfile, int width, int height, int maxval, std::string filename);

void nodes_to_p6(std::ofstream &stream, std::ofstream &logstream, sample_node nodes[],
                 sample_node upnodes[], int count);

#endif //GDALWORK_PPM_H
