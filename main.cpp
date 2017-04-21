#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "ogrsf_frmts.h"
#include "sampling.h"
#include "ppm.h"

#define XSCALING 1
#define YSCALING 1
#define XCOARSEFACTOR 4
#define YCOARSEFACTOR 3

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

    // Build the envelope and feature dataset
    std::vector<OGREnvelope> envelopes;
    std::vector<OGRGeometry *> geometries;
    OGRFeature *feature;
    OGREnvelope temp_envelope = OGREnvelope();
    while ((feature = vect_layer->GetNextFeature()) != NULL) {
        geometries.push_back(feature->GetGeometryRef());
        feature->GetGeometryRef()->getEnvelope(&temp_envelope);
        envelopes.push_back(temp_envelope);
    }
    envelopes.shrink_to_fit();
    geometries.shrink_to_fit();

    // Now that we've opened the datasets, get the red band for band info
    int node_i_count = data_manager.get_width() / XSCALING;

    sample_node *node_array = (sample_node *) CPLMalloc(sizeof(sample_node) * node_i_count);
    initialize_nodes(node_array, node_i_count);

    GByte *data = NULL;
    int current_factor = XSCALING * XCOARSEFACTOR;
    int size = 0;
    int pixel_x = 0;
    int line_y = 0;
    int randnum = rand_int(0, current_factor);
    int data_count = 0;
    OGRPoint pix_point = OGRPoint(0, 0); // Used for point in polygon checking
    // Initialize our random number generator
    srand(time(NULL));
    std::ofstream outfile;
    write_p6(outfile, data_manager.get_width() / XSCALING, data_manager.get_height() / YSCALING,
             255, "out.ppm");
    // For each row of nodes
    while ((data = data_manager.get_next_data(size)) != NULL) {
        int current_line = 0;
        while (data_count + randnum * 3 < size) {
            // We've got data to process at this point, increment our counts
            data_count += randnum * 3;
            pixel_x += randnum;
            if (pixel_x >= node_i_count * XSCALING) {
                pixel_x -= node_i_count * XSCALING;
                line_y++;
                current_line++;
            }
            if (line_y >= YSCALING) {
                nodes_to_p6(outfile, node_array, node_i_count);
                initialize_nodes(node_array, node_i_count);
                line_y -= YSCALING;
            }
            int node_x = pixel_x / XSCALING;

            // Testing code here
            int pixel_num = data_manager.get_current_i() * data_manager.get_blockwidth() + pixel_x;
            int line_num = data_manager.get_current_j() * data_manager.get_blockheight() + current_line;
            pix_point.setX(pixel_data[0] + pixel_num * pixel_data[1] + line_num * pixel_data[2]);
            pix_point.setY(pixel_data[3] + pixel_num * pixel_data[4] + line_num * pixel_data[5]);

            OGREnvelope point_envelope = OGREnvelope();
            pix_point.getEnvelope(&point_envelope);
            bool in_shape = false;
            for (int i = 0; i < envelopes.size(); i++) {
                if (envelopes[i].Contains(point_envelope)) {
                    if (geometries[i]->Contains(&pix_point)) {
                        in_shape = true;
                        break;
                    }
                }
            }
            if (in_shape) {
                current_factor = XSCALING;
                node_array[node_x].red += data[data_count];
                node_array[node_x].green += data[data_count + 1];
                node_array[node_x].blue += data[data_count + 2];
                node_array[node_x].count++;
            } else {
                current_factor = XSCALING * XCOARSEFACTOR;
                for (int i = 0; i < XCOARSEFACTOR && node_x + i < node_i_count; ++i) {
                    node_array[node_x + i].red += data[data_count];
                    node_array[node_x + i].green += data[data_count + 1];
                    node_array[node_x + i].blue += data[data_count + 2];
                    node_array[node_x + i].count++;
                }
            }

            // Add the remaining offset to set us up for the next sample. Exceeding the boundaries will
            // automatically be caught on the next run of the while loop
            data_count += (XSCALING - randnum) * 3;
            pixel_x += (XSCALING - randnum);
            randnum = rand_int(0, current_factor);
        }
        // We ran out of data, calculate the count offset and get some more
        data_count -= size;
    }
    nodes_to_p6(outfile, node_array, node_i_count);



    /*
    //Raster stuff
    GDALDataset *poDataset;
    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen("raster/HYP_LR_SR_OB_DR.tif", GA_ReadOnly);
    double        adfGeoTransform[6];
    printf( "Driver: %s/%s\n",
            poDataset->GetDriver()->GetDescription(),
            poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    printf( "Size is %dx%dx%d\n",
            poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
            poDataset->GetRasterCount() );
    if( poDataset->GetProjectionRef()  != NULL )
        printf( "Projection is `%s'\n", poDataset->GetProjectionRef() );
    if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        printf( "Origin = (%.6f,%.6f)\n",
                adfGeoTransform[0], adfGeoTransform[3] );
        printf( "Pixel Size = (%.6f,%.6f)\n",
                adfGeoTransform[1], adfGeoTransform[5] );
    }

    GDALRasterBand  *poBand;
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    double          adfMinMax[2];
    poBand = poDataset->GetRasterBand( 3 );
    poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
    printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
            nBlockXSize, nBlockYSize,
            GDALGetDataTypeName(poBand->GetRasterDataType()),
            GDALGetColorInterpretationName(
                    poBand->GetColorInterpretation()) );
    adfMinMax[0] = poBand->GetMinimum( &bGotMin );
    adfMinMax[1] = poBand->GetMaximum( &bGotMax );
    if( ! (bGotMin && bGotMax) )
        GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
    printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
    if( poBand->GetOverviewCount() > 0 )
        printf( "Band has %d overviews.\n", poBand->GetOverviewCount() );
    if( poBand->GetColorTable() != NULL )
        printf( "Band has a color table with %d entries.\n",
                poBand->GetColorTable()->GetColorEntryCount() );
    */

    // Vector Stuff
    /*
    GDALAllRegister();
    GDALDataset *poDS;
    poDS = (GDALDataset *) GDALOpenEx("vector", GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (poDS == NULL) {
        printf("Open failed.\n");
        exit(1);
    }
    OGRLayer *poLayer;
    poLayer = poDS->GetLayer(0);
    OGRFeature *poFeature;
    poLayer->ResetReading();
    while ((poFeature = poLayer->GetNextFeature()) != NULL) {
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
        int iField;

        for (iField = 0; iField < poFDefn->GetFieldCount(); iField++) {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);
            if (poFieldDefn->GetType() == OFTInteger) {
                printf("%d,", poFeature->GetFieldAsInteger(iField));
            } else if (poFieldDefn->GetType() == OFTInteger64) {
                printf(CPL_FRMT_GIB ",", poFeature->GetFieldAsInteger64(iField));
            } else if (poFieldDefn->GetType() == OFTReal) {
                printf("%.3f,", poFeature->GetFieldAsDouble(iField));
            } else if (poFieldDefn->GetType() == OFTString) {
                printf("%s,", poFeature->GetFieldAsString(iField));
            } else {
                printf("%s,", poFeature->GetFieldAsString(iField));
            }
        }
        OGRGeometry *poGeometry;
        poGeometry = poFeature->GetGeometryRef();
        if( poGeometry != NULL
            && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon )
        {
            OGRPolygon *poPolygon = (OGRPolygon *) poGeometry;
            if (poGeometry->getLinearGeometry()) {
                printf("testing\n");
            }
            //printf( "%.3f,%3.f\n", poPolygon->ge );
        }
        else
        {
            printf( "no polygon geometry\n" );
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
     */
}