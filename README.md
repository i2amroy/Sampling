To run the example data simply compile and execute the code as is. If you
wish to run the program on other raster and vector data, simply repoint the
definitions at the top of main.cpp to your desired vector and raster data
before recompiling and executing.

Notes:
1) Requires the GDAL/OGR library to be installed
2) Currently only functions on GeoTiff's with blocks that span entire lines.
 While the program can handle multi-line blocks; it will currently break if 
 given a file with multiple blocks per line.
 
 Image data taken from naturalearthdata.com/downloads/