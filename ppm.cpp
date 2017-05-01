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

void nodes_to_p6(std::ofstream &stream, std::ofstream &logstream, sample_node nodes[],
                 sample_node upnodes[], int count) {
    for (int i = 0; i < count; ++i) {
        unsigned char arr[3] = {};
        unsigned char arr2[3] = {};
        // If we have samples then calculate our output
        if (nodes[i].count > 0) {
            arr[0] = nodes[i].red / nodes[i].count;
            arr[1] = nodes[i].green / nodes[i].count;
            arr[2] = nodes[i].blue / nodes[i].count;
            arr2[0] = 255;
        } else {
            if (upnodes[i].count == 0 || nodes[i-1].streak < upnodes[i].streak || rand_int(0, 2)) {
                nodes[i].red = nodes[i-1].red;
                nodes[i].green = nodes[i-1].green;
                nodes[i].blue = nodes[i-1].blue;
                nodes[i].count++;
                nodes[i].streak++;
                arr[0] = nodes[i].red / nodes[i].count;
                arr[1] = nodes[i].green / nodes[i].count;
                arr[2] = nodes[i].blue / nodes[i].count;
            } else {
                nodes[i].red = upnodes[i].red;
                nodes[i].green = upnodes[i].green;
                nodes[i].blue = upnodes[i].blue;
                nodes[i].count++;
                nodes[i].streak++;
                arr[0] = nodes[i].red / nodes[i].count;
                arr[1] = nodes[i].green / nodes[i].count;
                arr[2] = nodes[i].blue / nodes[i].count;
            }
        }
        stream.write((char *) arr, 3);
        logstream.write((char *) arr2, 3);

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