#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "ogrsf_frmts.h"
#include "sampling.h"
#include "ppm.h"

#define XSCALING 1
#define YSCALING 1
#define FINEFACTOR 3
#define COARSEFACTOR 10

int main() {
    // First open the raster dataset
    GDALAllRegister();
    GDALDataset *raster_data = (GDALDataset *) GDALOpen("Japan.tif", GA_ReadOnly);
    sampler data_manager(raster_data);
    // Get the required pixel data from the raster data, format here is
    // upper left x, width x value, height x value, upper left y, width y value, height y value
    double pixel_data[6] = {};
    CPLErr err = raster_data->GetGeoTransform(pixel_data);

    // Open the vector data
    GDALDataset *vector_data = (GDALDataset *) GDALOpenEx("vector", GDAL_OF_VECTOR, NULL, NULL,
                                                          NULL);
    OGRLayer *vect_layer = vector_data->GetLayer(0);

//    // Build the envelope and feature dataset
//    std::vector<OGREnvelope> envelopes;
//    std::vector<OGRGeometry *> geometries;
//    OGRFeature *feature;
//    OGREnvelope temp_envelope = OGREnvelope();
//    while ((feature = vect_layer->GetNextFeature()) != NULL) {
//        geometries.push_back(feature->GetGeometryRef());
//        feature->GetGeometryRef()->getEnvelope(&temp_envelope);
//        envelopes.push_back(temp_envelope);
//    }
//    envelopes.shrink_to_fit();
//    geometries.shrink_to_fit();

    OGRMultiPolygon preshapes = OGRMultiPolygon();
    OGRFeature *feature;
    while ((feature = vect_layer->GetNextFeature()) != NULL) {
        preshapes.addGeometry(feature->GetGeometryRef());
    }
    OGRGeometry *multishapes = preshapes.Union(&preshapes);
    if (multishapes == NULL) {
        printf("Error processing shapefile union.");
        return 1;
    }
    OGRGeometry *shapes = multishapes->Boundary();
    printf("multishapes type %s\n", multishapes->getGeometryName());
    printf("shapes type %s\n", shapes->getGeometryName());

    int node_i_count = data_manager.get_width();

    sample_node *node_array = (sample_node *) CPLMalloc(sizeof(sample_node) * node_i_count);
    initialize_nodes(node_array, node_i_count);
    sample_node *up_array = (sample_node *) CPLMalloc(sizeof(sample_node) * node_i_count);
    initialize_nodes(up_array, node_i_count);

    GByte *data = NULL;
//    int current_factor = XSCALING * XCOARSEFACTOR;
    int size = 0;
//    int pixel_x = 0;
//    int line_y = 0;
//    int randnum = rand_int(0, current_factor);
//    int data_count = 0;
//    OGRPoint start_point = OGRPoint(0, 0); // Used for point in polygon checking


    OGRPoint start_point = OGRPoint();
    OGRPoint end_point = OGRPoint();
    OGRLineString test_segment = OGRLineString();
    test_segment.addPoint(&start_point);
    test_segment.addPoint(&end_point);
    OGRPoint tmp_point = OGRPoint();
    OGRGeometry *tmp;

    // Initialize our random number generator
    srand(time(NULL));

    std::ofstream outfile;
    write_p6(outfile, data_manager.get_width() / XSCALING, data_manager.get_height() / YSCALING,
             255, "out.ppm");
    // For each row of nodes
    while ((data = data_manager.get_next_data(size)) != NULL) {
        for (int current_line = 0; current_line < data_manager.get_blockheight(); ++current_line) {
            // Create a line segment reaching from one end to the other
            int line_num =
                    data_manager.get_current_j() * data_manager.get_blockheight() + current_line;
            int end_num = data_manager.get_width() - 1;
            double start_x = pixel_data[0] + line_num * pixel_data[2];
            double end_x = pixel_data[0] + end_num * pixel_data[1] + line_num * pixel_data[2];
            double start_y = pixel_data[3] + line_num * pixel_data[5];
            double end_y = pixel_data[3] + end_num * pixel_data[4] + line_num * pixel_data[5];
            test_segment.setPoint(0, start_x, start_y);
            test_segment.setPoint(1, end_x, end_y);

            // Find all intersection points
            tmp = test_segment.Intersection(shapes);

            // We have to do 1 point in polygon check at the start to see if we are already
            // inside of a shape.
            test_segment.getPoint(0, &tmp_point);
            OGRBoolean in_shape = shapes->Contains(&tmp_point);

            std::vector<int> counts = std::vector<int>();
            std::vector<int> remainders = std::vector<int>();
            // If there was at least one intersection
            if (!tmp->IsEmpty()) {
                OGRMultiPoint *intersections = (OGRMultiPoint *) tmp;

                bool x_coords = pixel_data != 0;

                // Calculate what our initial start pixel was
                double last_pixel = 0;
                if (x_coords) {
                    last_pixel =
                            (start_x - pixel_data[0] - line_num * pixel_data[2]) / pixel_data[1];
                } else {
                    last_pixel =
                            (start_y - pixel_data[3] - line_num * pixel_data[5]) / pixel_data[4];
                }

                for (int i = 0; i < intersections->getNumGeometries(); ++i) {
                    // Get the next intersection point
                    OGRPoint *pt = (OGRPoint *) intersections->getGeometryRef(i);
                    // Calculate how many steps we need to take to get there and store it
                    double point_pixel = 0;
                    if (x_coords) {
                        point_pixel = (pt->getX() - pixel_data[0] - line_num * pixel_data[2]) /
                                      pixel_data[1];
                    } else {
                        point_pixel = (pt->getY() - pixel_data[3] - line_num * pixel_data[5]) /
                                      pixel_data[4];
                    }
                    double pixel_dif = point_pixel - last_pixel;
                    if (in_shape) {
                        counts.push_back(int(pixel_dif) / FINEFACTOR);
                        remainders.push_back(int(pixel_dif) % FINEFACTOR);
                    } else {
                        counts.push_back(int(pixel_dif) / COARSEFACTOR);
                        remainders.push_back(int(pixel_dif) % COARSEFACTOR);
                    }
                    // And then set our current point to the last point and flip our in_shape
                    last_pixel = point_pixel;
                    in_shape = !in_shape;
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
            for (int region = 0; region < counts.size(); ++region) {
                int factor = COARSEFACTOR;
                if (in_shape) {
                    factor = FINEFACTOR;
                }
                int countdown = counts[region];
                while (countdown > 0) {
                    int rand = rand_int(0, factor);
                    int rem = factor - rand;
                    pixel_counter += rand;
                    node_array[pixel_counter].red = 255;
                    node_array[pixel_counter].count++;
                    pixel_counter += rem;
                    countdown--;
                }
                for (int remainder = 0; remainder < remainders[region]; ++remainder) {
                    pixel_counter++;
                    if (!rand_int(0, factor)) {
                        node_array[pixel_counter].red = 255;
                        node_array[pixel_counter].count++;
                    }
                }
            }
            nodes_to_p6(outfile, node_array, node_i_count);
            initialize_nodes(node_array, node_i_count);
        }
    }
}

//        int current_line = 0;
//        while (data_count + randnum * 3 < size) {
//            // We've got data to process at this point, increment our counts
//            data_count += randnum * 3;
//            pixel_x += randnum;
//            if (pixel_x >= node_i_count * XSCALING) {
//                pixel_x -= node_i_count * XSCALING;
//                line_y++;
//                current_line++;
//            }
//            if (line_y >= YSCALING) {
//                nodes_to_p6(outfile, node_array, node_i_count);
//                initialize_nodes(node_array, node_i_count);
//                line_y -= YSCALING;
//            }
//            int node_x = pixel_x / XSCALING;
//
//            // Testing code here
//            int pixel_num = data_manager.get_current_i() * data_manager.get_blockwidth() + pixel_x;
//            int line_num = data_manager.get_current_j() * data_manager.get_blockheight() + current_line;
//            pix_point.setX(pixel_data[0] + pixel_num * pixel_data[1] + line_num * pixel_data[2]);
//            pix_point.setY(pixel_data[3] + pixel_num * pixel_data[4] + line_num * pixel_data[5]);
//
//            OGREnvelope point_envelope = OGREnvelope();
//            pix_point.getEnvelope(&point_envelope);
//            bool in_shape = false;
//            for (int i = 0; i < envelopes.size(); i++) {
//                if (envelopes[i].Contains(point_envelope)) {
//                    if (geometries[i]->Contains(&pix_point)) {
//                        in_shape = true;
//                        break;
//                    }
//                }
//            }
//            if (in_shape) {
//                current_factor = XSCALING;
//                node_array[node_x].red += data[data_count];
//                node_array[node_x].green += data[data_count + 1];
//                node_array[node_x].blue += data[data_count + 2];
//                node_array[node_x].count++;
//            } else {
//                current_factor = XSCALING * XCOARSEFACTOR;
//                for (int i = 0; i < XCOARSEFACTOR && node_x + i < node_i_count; ++i) {
//                    node_array[node_x + i].red += data[data_count];
//                    node_array[node_x + i].green += data[data_count + 1];
//                    node_array[node_x + i].blue += data[data_count + 2];
//                    node_array[node_x + i].count++;
//                }
//            }
//
//            // Add the remaining offset to set us up for the next sample. Exceeding the boundaries will
//            // automatically be caught on the next run of the while loop
//            data_count += (XSCALING - randnum) * 3;
//            pixel_x += (XSCALING - randnum);
//            randnum = rand_int(0, current_factor);
//        }
//        // We ran out of data, calculate the count offset and get some more
//        data_count -= size;