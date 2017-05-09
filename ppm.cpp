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
        u_int8_t arr[3] = {};
        u_int8_t  arr2[3] = {};
        // If we have samples then calculate our output
        if (nodes[i].count > 0) {
            arr[0] = nodes[i].red / nodes[i].count;
            arr[1] = nodes[i].green / nodes[i].count;
            arr[2] = nodes[i].blue / nodes[i].count;
            arr2[0] = 255;
        } else {
            if (upnodes[i].count != 0) {
                int left_weight = std::max(100 / (int)nodes[i-1].streak, 1);
                int up_weight = std::max(100 / (int)upnodes[i].streak, 1);
                int total_weight = left_weight + up_weight;
                nodes[i].red = (nodes[i-1].red * left_weight + upnodes[i].red * up_weight) / total_weight;
                nodes[i].green = (nodes[i-1].green * left_weight + upnodes[i].green * up_weight) / total_weight;
                nodes[i].blue = (nodes[i-1].blue * left_weight + upnodes[i].blue * up_weight) / total_weight;
                nodes[i].count = 1;
                nodes[i].streak = std::min(nodes[i-1].streak, upnodes[i].streak) + 1;
                arr[0] = nodes[i].red;
                arr[1] = nodes[i].green;
                arr[2] = nodes[i].blue;
            } else {
                nodes[i].red = nodes[i-1].red;
                nodes[i].green = nodes[i-1].green;
                nodes[i].blue = nodes[i-1].blue;
                nodes[i].count = 1;
                nodes[i].streak = nodes[i-1].streak + 1;
                arr[0] = nodes[i].red;
                arr[1] = nodes[i].green;
                arr[2] = nodes[i].blue;
            }
        }
        stream.write((char *) arr, 3);
        logstream.write((char *) arr2, 3);

#ifdef DEBUG
        if (i == 0 || nodes[i].count > 1) {
            printf("node.red:%6d, node.green:%6d, node.blue:%6d, node.count:%3d, r:%3d, g:%3d, b:%3d pixelcount %d\n",
                   nodes[i].red,
                   nodes[i].green, nodes[i].blue, nodes[i].count, arr[0], arr[1], arr[2],
                   pixelcount);
        }
        pixelcount++;
#endif
    }
}