#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "sampling.h"
#include "ppm.h"

#define FINEFACTOR 2
#define COARSEFACTOR 10
#define DOWNSAMPLING 2
#define VECTOR "vector"
#define RASTER "Japan.tif"

// NOTE: This implementation currently only processes GEOTIFF's where the blocksize contains an
// entire row of the image. Blocks may be multiple rows tall, but there may not be multiple blocks
// per row.

int main() {
    // First open the raster dataset
    GDALAllRegister();
    GDALDataset *raster_data = (GDALDataset *) GDALOpen(RASTER, GA_ReadOnly);
    sampler data_manager(raster_data);
    // Get the required pixel data from the raster data, format here is
    // upper left x, width x value, height x value, upper left y, width y value, height y value
    double pixel_data[6] = {};
    CPLErr err = raster_data->GetGeoTransform(pixel_data);

    // Open the vector data
    GDALDataset *vector_data = (GDALDataset *) GDALOpenEx(VECTOR, GDAL_OF_VECTOR, NULL, NULL,
                                                          NULL);
    OGRLayer *vect_layer = vector_data->GetLayer(0);

    // Build our multishape and shape references
    OGRMultiPolygon preshapes = OGRMultiPolygon();
    OGRFeature *feature;
    std::vector<OGRGeometry *> geometries;
    while ((feature = vect_layer->GetNextFeature()) != NULL) {
        preshapes.addGeometry(feature->GetGeometryRef());
        geometries.push_back(feature->GetGeometryRef());
    }
    geometries.shrink_to_fit();
    OGRGeometry *multishapes = preshapes.Union(&preshapes);
    if (multishapes == NULL) {
        printf("Error processing shapefile union.");
        return 1;
    }
    OGRGeometry *shapes = multishapes->Boundary();

    int node_i_count = data_manager.get_width() / DOWNSAMPLING;

    sample_node *node_array = (sample_node *) CPLMalloc(sizeof(sample_node) * node_i_count);
    initialize_nodes(node_array, node_i_count);
    sample_node *up_array = (sample_node *) CPLMalloc(sizeof(sample_node) * node_i_count);
    initialize_nodes(up_array, node_i_count);

    GByte *data = NULL;
    int size = 0;
    OGRPoint start_point = OGRPoint();
    OGRPoint end_point = OGRPoint();
    OGRLineString test_segment = OGRLineString();
    test_segment.addPoint(&start_point);
    test_segment.addPoint(&end_point);
    OGRPoint tmp_point = OGRPoint();
    OGRGeometry *tmp;


    // Initialize our random number generator
    srand((unsigned int) time(NULL));

    std::ofstream outfile;
    write_p6(outfile, data_manager.get_width() / DOWNSAMPLING,
             data_manager.get_height() / DOWNSAMPLING,
             255, "out.ppm");

    std::ofstream logfile;
    write_p6(logfile, data_manager.get_width() / DOWNSAMPLING,
             data_manager.get_height() / DOWNSAMPLING,
             255, "log.ppm");
    // For each row of nodes
    double end_num = data_manager.get_width() - 1;
    int sample_line = 0;
    while ((data = data_manager.get_next_data(size)) != NULL) {
        for (int current_line = 0; current_line < data_manager.get_blockheight(); ++current_line) {
            // Create a line segment reaching from one end to the other
            int line_num =
                    data_manager.get_current_j() * data_manager.get_blockheight() + current_line;
            // .5's here to get us to the middle of the pixels
            double start_x = pixel_data[0] + .5 * pixel_data[1] + line_num * pixel_data[2];
            double end_x =
                    pixel_data[0] + (end_num + .5) * pixel_data[1] + line_num * pixel_data[2];
            double start_y = pixel_data[3] + .5 * pixel_data[4] + line_num * pixel_data[5];
            double end_y =
                    pixel_data[3] + (end_num + .5) * pixel_data[4] + line_num * pixel_data[5];
            test_segment.setPoint(0, start_x, start_y);
            test_segment.setPoint(1, end_x, end_y);

            // Find all intersection points
            tmp = test_segment.Intersection(shapes);

            // We have to do 1 point in polygon check at the start to see if we are already
            // inside of a shape.
            tmp_point.setX(start_x);
            tmp_point.setY(start_y);
            bool in_shape = false;
            for (auto geom:geometries) {
                if (geom->Contains(&tmp_point)) {
                    in_shape = true;
                    break;
                }
            }
            bool initial_in_shape = in_shape;

            std::vector<int> counts = std::vector<int>();
            std::vector<int> remainders = std::vector<int>();
            // If there was at least one intersection
            if (tmp != NULL && !tmp->IsEmpty()) {
                OGRMultiPoint *intersections = dynamic_cast<OGRMultiPoint *>(tmp);

                bool x_coords = pixel_data[1] != 0;

                // Calculate what our initial start pixel was
                int last_pixel = 0;
                if (x_coords) {
                    last_pixel = (int) (
                            (start_x - pixel_data[0] - line_num * pixel_data[2]) / pixel_data[1] -
                            .5);
                } else {
                    last_pixel = (int) (
                            (start_y - pixel_data[3] - line_num * pixel_data[5]) / pixel_data[4] -
                            .5);
                }

                int point_pixel = 0;
                // Run all calculations for the middle regions
                for (int i = 0; i < intersections->getNumGeometries(); ++i) {
                    // Get the next intersection point
                    OGRPoint *pt = dynamic_cast<OGRPoint *>(intersections->getGeometryRef(i));
                    // Calculate how many steps we need to take to get there and store it
                    if (x_coords) {
                        point_pixel = (int) (
                                (pt->getX() - pixel_data[0] - line_num * pixel_data[2]) /
                                pixel_data[1] - .5);
                    } else {
                        point_pixel = (int) (
                                (pt->getY() - pixel_data[3] - line_num * pixel_data[5]) /
                                pixel_data[4] - .5);
                    }
                    int pixel_dif = point_pixel - last_pixel;
                    if (in_shape) {
                        counts.push_back(pixel_dif / FINEFACTOR);
                        remainders.push_back(pixel_dif % FINEFACTOR);
                    } else {
                        counts.push_back(pixel_dif / COARSEFACTOR);
                        remainders.push_back(pixel_dif % COARSEFACTOR);
                    }
                    // And then set our current point to the last point and flip our in_shape
                    last_pixel = point_pixel;
                    in_shape = !in_shape;
                }

                // One final calculation for the last region
                int pixel_dif = data_manager.get_width() - last_pixel;
                if (in_shape) {
                    counts.push_back(pixel_dif / FINEFACTOR);
                    remainders.push_back(pixel_dif % FINEFACTOR);
                } else {
                    counts.push_back(pixel_dif / COARSEFACTOR);
                    remainders.push_back(pixel_dif % COARSEFACTOR);
                }
            } else {
                if (in_shape) {
                    counts.push_back(data_manager.get_width() / FINEFACTOR);
                    remainders.push_back(data_manager.get_width() % FINEFACTOR);
                } else {
                    counts.push_back(data_manager.get_width() / COARSEFACTOR);
                    remainders.push_back(data_manager.get_width() % COARSEFACTOR);
                }
            }

            // At this point our counts and remainders are both initialized so we can do the actual
            // sampling of our image
            int pixel_counter = 0;
            in_shape = initial_in_shape;

            // Always sample the first pixel in every row
            int replacement = 0;
            while (true) {
                if (counts[replacement] > 0) {
                    counts[replacement]--;
                    if (in_shape) {
                        remainders[replacement] += FINEFACTOR - 1;
                    } else {
                        remainders[replacement] += COARSEFACTOR - 1;
                    }
                    break;
                } else if (remainders[replacement] > 0) {
                    remainders[replacement]--;
                    break;
                }
                replacement++;
                in_shape = !in_shape;
            }

            // Reset after usage in our first pixel sampling
            in_shape = initial_in_shape;
            if (line_num == 314) {
                printf("inshape: %d\n", in_shape);
                for (int i = 0; i < counts.size(); ++i) {
                    printf("%2d ", counts[i]);
                }
                printf("\n");
                for (int i = 0; i < remainders.size(); ++i) {
                    printf("%2d ", remainders[i]);
                }
                printf("\n");
            }

            // *3 here because data has 3 red/green/blue bands interlaced
            int data_line_offset = current_line * data_manager.get_width() * 3;
            int array_slot = pixel_counter / DOWNSAMPLING;
            node_array[array_slot].red += data[data_line_offset + pixel_counter * 3];
            node_array[array_slot].green += data[data_line_offset + pixel_counter * 3 + 1];
            node_array[array_slot].blue += data[data_line_offset + pixel_counter * 3 + 2];
            node_array[array_slot].count++;
            pixel_counter++;

            // Next we go through the list of counts and remainders
            for (int region = 0; region < counts.size(); ++region) {
                int factor = COARSEFACTOR;
                if (in_shape) {
                    factor = FINEFACTOR;
                }

                // For each count sample one spot in your sample factor as a speedup to individual
                // sampling rolls
                int countdown = counts[region];
                while (countdown > 0) {
                    int rand = rand_int(0, factor);
                    int rem = factor - rand;
                    pixel_counter += rand;
                    array_slot = pixel_counter / DOWNSAMPLING;
                    node_array[array_slot].red += data[data_line_offset + pixel_counter * 3];
                    node_array[array_slot].green += data[data_line_offset + pixel_counter * 3 + 1];
                    node_array[array_slot].blue += data[data_line_offset + pixel_counter * 3 + 2];
                    node_array[array_slot].count++;
                    pixel_counter += rem;
                    countdown--;
                }

                // While each remainder spot has to be individually rolled as a 1 in FACTOR rng to
                // determine if we sample that spot or not.
                int remainder = remainders[region];
                while (remainder > 0) {
                    if (!rand_int(0, factor)) {
                        array_slot = pixel_counter / DOWNSAMPLING;
                        node_array[array_slot].red += data[data_line_offset + pixel_counter * 3];
                        node_array[array_slot].green += data[data_line_offset + pixel_counter * 3 +
                                                             1];
                        node_array[array_slot].blue += data[data_line_offset + pixel_counter * 3 +
                                                            2];
                        node_array[array_slot].count++;
                    }
                    pixel_counter += 1;
                    remainder--;
                }
                in_shape = !in_shape;
            }

            // Only print our line out to the output if we've reached the amount of lines per
            // our DOWNSAMPLING value
            if (sample_line == DOWNSAMPLING - 1) {
                nodes_to_p6(outfile, logfile, node_array, up_array, node_i_count);
                // Store our old node_array into up_array
                swap_node_arrays(node_array, up_array);
                // And reset our old node_array for the next pass
                initialize_nodes(node_array, node_i_count);
                sample_line = 0;
            } else {
                // Else just add one to our line counter for our sample
                sample_line++;
            }
        }
    }
}