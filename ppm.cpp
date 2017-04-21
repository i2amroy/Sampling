//
// Created by OtherPlayers on 3/3/17.
//

#include "ppm.h"
#include "sampling.h"
#include <fstream>

#define DEBUG

#ifdef DEBUG
int pixelcount = 0;
#endif

void write_p6(std::ofstream &outfile, int width, int height, int maxval, std::string filename) {
    outfile.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    outfile << "P6 " << width << " " << height << " " << maxval << "\n";
}

void nodes_to_p6(std::ofstream &stream, sample_node nodes[], int count) {
    for (int i = 0; i < count; ++i) {
        unsigned char arr[3] = {};
        if (nodes[i].count > 0) {
            arr[0] = nodes[i].red / nodes[i].count;
            arr[1] = nodes[i].green / nodes[i].count;
            arr[2] = nodes[i].blue / nodes[i].count;
        }
        stream.write((char *) arr, 3);

#ifdef DEBUG
        if (i == 0) {
            printf("node.red:%6d, node.green:%6d, node.blue:%6d, node.count:%3d, r:%3d, g:%3d, b:%3d pixelcount %d\n",
                   nodes[i].red,
                   nodes[i].green, nodes[i].blue, nodes[i].count, arr[0], arr[1], arr[2],
                   pixelcount);
        }
        pixelcount++;
#endif
    }
}