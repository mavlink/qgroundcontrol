/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @brief Unit test fpr QGCGeo coordinate project math.
///
///     @author David Goodman <dagoodma@gmail.com>

#include "NetCdfTest.h"
#include <libs/netcdf-cxx4/cxx4/netcdf>

using namespace std;
using namespace netCDF;
using namespace netCDF::exceptions;

// This is the name of the data file we will create.
#define FILE_NAME "pres_temp_4D.nc"

// We are writing 4D data, a 2 x 6 x 12 lvl-lat-lon grid, with 2
// timesteps of data.
#define NDIMS 4
#define NLVL 2
#define NLAT 6
#define NLON 12
#define NREC 2

// Names of things.
#define LVL_NAME "level"
#define LAT_NAME "latitude"
#define LON_NAME "longitude"
#define REC_NAME "time"
#define PRES_NAME "pressure"
#define TEMP_NAME "temperature"
#define MAX_ATT_LEN 80
// These are used to construct some example data.
#define SAMPLE_PRESSURE 900
#define SAMPLE_TEMP 9.0
#define START_LAT 25.0
#define START_LON -125.0

// Return this code to the OS in case of failure.
#define NC_ERR 2

NetCdfTest::NetCdfTest(void)
{
}

void NetCdfTest::_pres_temp_4D_wr_test(void)
{
    string UNITS = "units";
    string DEGREES_EAST = "degrees_east";
    string DEGREES_NORTH = "degrees_north";

    // For the units attributes.
    string PRES_UNITS = "hPa";
    string TEMP_UNITS = "celsius";
    string LAT_UNITS = "degrees_north";
    string LON_UNITS = "degrees_east";

    // We will write latitude and longitude fields.
    float lats[NLAT], lons[NLON];

    // Program variables to hold the data we will write out. We will
    // only need enough space to hold one timestep of data; one record.
    float pres_out[NLVL][NLAT][NLON];
    float temp_out[NLVL][NLAT][NLON];

    int i = 0; // used in the data generation loop

    int result = 0;

    // create some pretend data. If this wasn't an example program, we
    // would have some real data to write for example, model output.
    for (int lat = 0; lat < NLAT; lat++)
        lats[lat] = START_LAT + 5. * lat;
    for (int lon = 0; lon < NLON; lon++)
        lons[lon] = START_LON + 5. * lon;

    for (int lvl = 0; lvl < NLVL; lvl++)
        for (int lat = 0; lat < NLAT; lat++)
            for (int lon = 0; lon < NLON; lon++)
            {
                pres_out[lvl][lat][lon] = (float)(SAMPLE_PRESSURE + i);
                temp_out[lvl][lat][lon] = (float)(SAMPLE_TEMP + i++);
            }

    try
    {

        // Create the file.
        NcFile test(FILE_NAME, NcFile::replace);

        // Define the dimensions. NetCDF will hand back an ncDim object for
        // each.
        NcDim lvlDim = test.addDim(LVL_NAME, NLVL);
        NcDim latDim = test.addDim(LAT_NAME, NLAT);
        NcDim lonDim = test.addDim(LON_NAME, NLON);
        NcDim recDim = test.addDim(REC_NAME); // adds an unlimited dimension

        // Define the coordinate variables.
        NcVar latVar = test.addVar(LAT_NAME, ncFloat, latDim);
        NcVar lonVar = test.addVar(LON_NAME, ncFloat, lonDim);

        // Define units attributes for coordinate vars. This attaches a
        // text attribute to each of the coordinate variables, containing
        // the units.
        latVar.putAtt(UNITS, DEGREES_NORTH);
        lonVar.putAtt(UNITS, DEGREES_EAST);

        // Define the netCDF variables for the pressure and temperature
        // data.
        vector<NcDim> dimVector;
        dimVector.push_back(recDim);
        dimVector.push_back(lvlDim);
        dimVector.push_back(latDim);
        dimVector.push_back(lonDim);
        NcVar pressVar = test.addVar(PRES_NAME, ncFloat, dimVector);
        NcVar tempVar = test.addVar(TEMP_NAME, ncFloat, dimVector);

        // Define units attributes for coordinate vars. This attaches a
        // text attribute to each of the coordinate variables, containing
        // the units.
        pressVar.putAtt(UNITS, PRES_UNITS);
        tempVar.putAtt(UNITS, TEMP_UNITS);

        // Write the coordinate variable data to the file.
        latVar.putVar(lats);
        lonVar.putVar(lons);

        // Write the pretend data. This will write our surface pressure and
        // surface temperature data. The arrays only hold one timestep
        // worth of data. We will just rewrite the same data for each
        // timestep. In a real application, the data would change between
        // timesteps.
        vector<size_t> startp, countp;
        startp.push_back(0);
        startp.push_back(0);
        startp.push_back(0);
        startp.push_back(0);
        countp.push_back(1);
        countp.push_back(NLVL);
        countp.push_back(NLAT);
        countp.push_back(NLON);
        for (size_t rec = 0; rec < NREC; rec++)
        {
            startp[0] = rec;
            pressVar.putVar(startp, countp, pres_out);
            tempVar.putVar(startp, countp, temp_out);
        }

        // The file is automatically closed by the destructor. This frees
        // up any internal netCDF resources associated with the file, and
        // flushes any buffers.

        cout << "*** SUCCESS writing example file " << FILE_NAME << "!" << endl;
        result = 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        result = NC_ERR;
    }

    // if it got through the whole implementation without triggering the catch block, the test passes
    QCOMPARE(result, 0);
}

void NetCdfTest::_pres_temp_4D_rd_test(void)
{
    // These arrays will store the latitude and longitude values.
    float lats[NLAT], lons[NLON];

    // These arrays will hold the data we will read in. We will only
    // need enough space to hold one timestep of data; one record.
    float pres_in[NLVL][NLAT][NLON];
    float temp_in[NLVL][NLAT][NLON];

    int result = 0;
    try
    {
        // Open the file.
        NcFile dataFile("pres_temp_4D.nc", NcFile::read);

        // Get the latitude and longitude variables and read data.
        NcVar latVar, lonVar;
        latVar = dataFile.getVar("latitude");
        if (latVar.isNull())
            result = NC_ERR;
        lonVar = dataFile.getVar("longitude");
        if (lonVar.isNull())
            result = NC_ERR;
        lonVar.getVar(lons);
        latVar.getVar(lats);

        // Check the coordinate variable data.
        for (int lat = 0; lat < NLAT; lat++)
            if (lats[lat] != START_LAT + 5. * lat)
                result = NC_ERR;

        for (int lon = 0; lon < NLON; lon++)
            if (lons[lon] != START_LON + 5. * lon)
                result = NC_ERR;

        // Get the pressure and temperature variables and read data one time step at a time
        NcVar presVar, tempVar;
        presVar = dataFile.getVar("pressure");
        if (presVar.isNull())
            result = NC_ERR;
        tempVar = dataFile.getVar("temperature");
        if (tempVar.isNull())
            result = NC_ERR;

        vector<size_t> startp, countp;
        startp.push_back(0);
        startp.push_back(0);
        startp.push_back(0);
        startp.push_back(0);
        countp.push_back(1);
        countp.push_back(NLVL);
        countp.push_back(NLAT);
        countp.push_back(NLON);
        for (size_t rec = 0; rec < NREC; rec++)
        {
            // Read the data one record at a time.
            startp[0] = rec;
            presVar.getVar(startp, countp, pres_in);
            tempVar.getVar(startp, countp, temp_in);

            int i = 0; // used in the data generation loop
            for (int lvl = 0; lvl < NLVL; lvl++)
                for (int lat = 0; lat < NLAT; lat++)
                    for (int lon = 0; lon < NLON; lon++)
                    {
                        if (pres_in[lvl][lat][lon] != (float)(SAMPLE_PRESSURE + i))
                            result = NC_ERR;
                        if (temp_in[lvl][lat][lon] != (float)(SAMPLE_TEMP + i++))
                            result = NC_ERR;
                    }

        } // next record

        // The file is automatically closed by the destructor. This frees
        // up any internal netCDF resources associated with the file, and
        // flushes any buffers.

        cout << "*** SUCCESS reading example file pres_temp_4D.nc!" << endl;
    }
    catch (NcException &e)
    {
        result = 1;
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
    }

    // if it got through the whole implementation without triggering the catch block, the test passes
    QCOMPARE(result, 0);
}

int sfc_pres_temp_wr(void)
{
    string UNITS = "units";
    string DEGREES_EAST = "degrees_east";
    string DEGREES_NORTH = "degrees_north";

    // We will write surface temperature and pressure fields.
    float presOut[NLAT][NLON];
    float tempOut[NLAT][NLON];
    float lats[NLAT];
    float lons[NLON];

    // In addition to the latitude and longitude dimensions, we will
    // also create latitude and longitude netCDF variables which will
    // hold the actual latitudes and longitudes. Since they hold data
    // about the coordinate system, the netCDF term for these is:
    // "coordinate variables."
    for (int lat = 0; lat < NLAT; lat++)
        lats[lat] = START_LAT + 5. * lat;

    for (int lon = 0; lon < NLON; lon++)
        lons[lon] = START_LON + 5. * lon;

    // Create some pretend data. If this wasn't an example program, we
    // would have some real data to write, for example, model
    // output.
    for (int lat = 0; lat < NLAT; lat++)
        for (int lon = 0; lon < NLON; lon++)
        {
            presOut[lat][lon] = SAMPLE_PRESSURE + (lon * NLAT + lat);
            tempOut[lat][lon] = SAMPLE_TEMP + .25 * (lon * NLAT + lat);
        }

    try
    {

        // Create the file. The Replace parameter tells netCDF to overwrite
        // this file, if it already exists.
        NcFile sfc(FILE_NAME, NcFile::replace);

        // Define the dimensions. NetCDF will hand back an ncDim object for
        // each.
        NcDim latDim = sfc.addDim(LAT_NAME, NLAT);
        NcDim lonDim = sfc.addDim(LON_NAME, NLON);

        // Define coordinate netCDF variables. They will hold the
        // coordinate information, that is, the latitudes and
        // longitudes. An pointer to a NcVar object is returned for
        // each.
        NcVar latVar = sfc.addVar(LAT_NAME, ncFloat, latDim); // creates variable
        NcVar lonVar = sfc.addVar(LON_NAME, ncFloat, lonDim);

        // Write the coordinate variable data. This will put the latitudes
        // and longitudes of our data grid into the netCDF file.
        latVar.putVar(lats);
        lonVar.putVar(lons);

        // Define units attributes for coordinate vars. This attaches a
        // text attribute to each of the coordinate variables, containing
        // the units. Note that we are not writing a trailing NULL, just
        // "units", because the reading program may be fortran which does
        // not use null-terminated strings. In general it is up to the
        // reading C program to ensure that it puts null-terminators on
        // strings where necessary.
        lonVar.putAtt(UNITS, DEGREES_EAST);
        latVar.putAtt(UNITS, DEGREES_NORTH);

        // Define the netCDF data variables.
        vector<NcDim> dims;
        dims.push_back(latDim);
        dims.push_back(lonDim);
        NcVar presVar = sfc.addVar(PRES_NAME, ncFloat, dims);
        NcVar tempVar = sfc.addVar(TEMP_NAME, ncFloat, dims);

        // Define units attributes for vars.
        presVar.putAtt(UNITS, "hPa");
        tempVar.putAtt(UNITS, "celsius");

        // Write the pretend data. This will write our surface pressure and
        // surface temperature data. The arrays of data are the same size
        // as the netCDF variables we have defined.
        presVar.putVar(presOut);
        tempVar.putVar(tempOut);

        // The file is automatically closed by the destructor. This frees
        // up any internal netCDF resources associated with the file, and
        // flushes any buffers.

        // cout << "*** SUCCESS writing example file " << FILE_NAME << "!" << endl;
        return 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        return NC_ERR;
    }
}

void NetCdfTest::_sfc_pres_temp_wr_test(void)
{
    const int result = sfc_pres_temp_wr();
    QCOMPARE(result, 0);
}

int sfc_pres_temp_rd(void)
{
    // These will hold our pressure and temperature data.
    float presIn[NLAT][NLON];
    float tempIn[NLAT][NLON];

    // These will hold our latitudes and longitudes.
    float latsIn[NLAT];
    float lonsIn[NLON];

    try
    {
        // Open the file and check to make sure it's valid.
        NcFile dataFile("sfc_pres_temp.nc", NcFile::read);

        // There are a number of inquiry functions in netCDF which can be
        // used to learn about an unknown netCDF file. In this case we know
        // that there are 2 netCDF dimensions, 4 netCDF variables, no
        // global attributes, and no unlimited dimension.

        // cout<<"there are "<<dataFile.getVarCount()<<" variables"<<endl;
        // cout<<"there are "<<dataFile.getAttCount()<<" attributes"<<endl;
        // cout<<"there are "<<dataFile.getDimCount()<<" dimensions"<<endl;
        // cout<<"there are "<<dataFile.getGroupCount()<<" groups"<<endl;
        // cout<<"there are "<<dataFile.getTypeCount()<<" types"<<endl;

        // Get the  latitude and longitude coordinate variables and read data
        NcVar latVar, lonVar;
        latVar = dataFile.getVar("latitude");
        if (latVar.isNull())
            return NC_ERR;
        lonVar = dataFile.getVar("longitude");
        if (lonVar.isNull())
            return NC_ERR;
        latVar.getVar(latsIn);
        lonVar.getVar(lonsIn);

        // Check the coordinate variable data.
        for (int lat = 0; lat < NLAT; lat++)
            if (latsIn[lat] != START_LAT + 5. * lat)
                return NC_ERR;

        // Check longitude values.
        for (int lon = 0; lon < NLON; lon++)
            if (lonsIn[lon] != START_LON + 5. * lon)
                return NC_ERR;

        // Read in presure and temperature variables and read data
        NcVar presVar, tempVar;
        presVar = dataFile.getVar("pressure");
        if (presVar.isNull())
            return NC_ERR;
        tempVar = dataFile.getVar("temperature");
        if (tempVar.isNull())
            return NC_ERR;
        presVar.getVar(presIn);
        tempVar.getVar(tempIn);

        // Check the data.
        for (int lat = 0; lat < NLAT; lat++)
            for (int lon = 0; lon < NLON; lon++)
                if (presIn[lat][lon] != SAMPLE_PRESSURE + (lon * NLAT + lat) || tempIn[lat][lon] != SAMPLE_TEMP + .25 * (lon * NLAT + lat))
                    return NC_ERR;

        // Each of the netCDF variables has a "units" attribute. Let's read
        // them and check them.
        NcVarAtt att;
        string units;

        att = latVar.getAtt("units");
        if (att.isNull())
            return NC_ERR;

        att.getValues(units);
        if (units != "degrees_north")
        {
            cout << "getValue returned " << units << endl;
            return NC_ERR;
        }

        att = lonVar.getAtt("units");
        if (att.isNull())
            return NC_ERR;

        att.getValues(units);
        if (units != "degrees_east")
        {
            cout << "getValue returned " << units << endl;
            return NC_ERR;
        }

        att = presVar.getAtt("units");
        if (att.isNull())
            return NC_ERR;

        att.getValues(units);
        if (units != "hPa")
        {
            cout << "getValue returned " << units << endl;
            return NC_ERR;
        }

        att = tempVar.getAtt("units");
        if (att.isNull())
            return NC_ERR;

        att.getValues(units);
        if (units != "celsius")
        {
            cout << "getValue returned " << units << endl;
            return NC_ERR;
        }

        // The file will be automatically closed by the destructor. This
        // frees up any internal netCDF resources associated with the file,
        // and flushes any buffers.
        // cout << "*** SUCCESS reading example file sfc_pres_temp.nc!" << endl;
        return 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        return NC_ERR;
    }
}

void NetCdfTest::_sfc_pres_temp_rd_test(void)
{
    const int result = sfc_pres_temp_rd();
    QCOMPARE(result, 0);
}

int simple_xy_wr(void)
{
    static const int NX = 6;
    static const int NY = 12;
    // This is the data array we will write. It will just be filled
    // with a progression of numbers for this example.
    int dataOut[NX][NY];

    // Create some pretend data. If this wasn't an example program, we
    // would have some real data to write, for example, model output.
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++)
            dataOut[i][j] = i * NY + j;

    // The default behavior of the C++ API is to throw an exception i
    // an error occurs. A try catch block is necessary.

    try
    {
        // Create the file. The Replace parameter tells netCDF to overwrite
        // this file, if it already exists.
        NcFile dataFile("simple_xy.nc", NcFile::replace);

        // Create netCDF dimensions
        NcDim xDim = dataFile.addDim("x", NX);
        NcDim yDim = dataFile.addDim("y", NY);

        // Define the variable. The type of the variable in this case is
        // ncInt (32-bit integer).
        vector<NcDim> dims;
        dims.push_back(xDim);
        dims.push_back(yDim);
        NcVar data = dataFile.addVar("data", ncInt, dims);

        // Write the data to the file. Although netCDF supports
        // reading and writing subsets of data, in this case we write all
        // the data in one operation.
        data.putVar(dataOut);

        // The file will be automatically close when the NcFile object goes
        // out of scope. This frees up any internal netCDF resources
        // associated with the file, and flushes any buffers.

        // cout << "*** SUCCESS writing example file simple_xy.nc!" << endl;
        return 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        return NC_ERR;
    }
}

void NetCdfTest::_simple_xy_wr_test(void)
{
    const int result = simple_xy_wr();
    QCOMPARE(result, 0);
}

int create_file(string filename, NcFile::FileFormat format)
{
    static const int NX = 6;
    static const int NY = 12;
    // This is the data array we will write. It will just be filled
    // with a progression of numbers for this example.
    int dataOut[NX][NY];

    // Create some pretend data. If this wasn't an example program, we
    // would have some real data to write, for example, model output.
    for (int i = 0; i < NX; i++)
        for (int j = 0; j < NY; j++)
            dataOut[i][j] = i * NY + j;
    // The default behavior of the C++ API is to throw an exception i
    // an error occurs. A try catch block is necessary.
    try
    {
        // Create the file. The Replace parameter tells netCDF to overwrite
        // this file, if it already exists.  The classic parameter specifies
        // that the file ceated should be in classic format, rather than the
        // default netCDF-4 format used by the cxx4 interface.
        NcFile dataFile(filename, NcFile::replace, format);

        // Create netCDF dimensions
        NcDim xDim = dataFile.addDim("x", NX);
        NcDim yDim = dataFile.addDim("y", NY);

        // Define the variable. The type of the variable in this case is
        // ncInt (32-bit integer).
        vector<NcDim> dims;
        dims.push_back(xDim);
        dims.push_back(yDim);
        NcVar data = dataFile.addVar("data", ncInt, dims);

        // In the classic model, must explicitly leave define mode
        // before writing data.  Need a method that calls nc_enddef().
        if (format != NcFile::nc4)
            dataFile.enddef();

        // Write the data to the file. Although netCDF supports
        // reading and writing subsets of data, in this case we write all
        // the data in one operation.
        data.putVar(dataOut);

        // The file will be automatically closed when the NcFile object goes
        // out of scope. This frees up any internal netCDF resources
        // associated with the file, and flushes any buffers.

        // cout << "*** SUCCESS writing example file simple_xy.nc!" << endl;
        return 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        return NC_ERR;
    }
}

int simple_xy_wr_formats()
{
    int ret;
    if (ret = create_file("simple_xy_nc4.nc", NcFile::nc4))
        return ret;
    cout << "*** SUCCESS creating nc4 file" << endl;

    if (ret = create_file("simple_xy_nc4classic.nc", NcFile::nc4classic))
        return ret;
    cout << "*** SUCCESS creating nc4classic file" << endl;

    if (ret = create_file("simple_xy_classic.nc", NcFile::classic))
        return ret;
    cout << "*** SUCCESS creating classic file" << endl;

    if (ret = create_file("simple_xy_classic64.nc", NcFile::classic64))
        return ret;
    cout << "*** SUCCESS creating classic64 file" << endl;

    return 0;
}

void NetCdfTest::_simple_xy_wr_formats_test(void)
{
    const int result = simple_xy_wr_formats();
    QCOMPARE(result, 0);
}

int simple_xy_rd()
{
    static const int NX = 6;
    static const int NY = 12;
    try
    {
        // This is the array we will read.
        int dataIn[NX][NY];

        // Open the file for read access
        NcFile dataFile("simple_xy.nc", NcFile::read);

        // Retrieve the variable named "data"
        NcVar data = dataFile.getVar("data");
        if (data.isNull())
            return NC_ERR;
        data.getVar(dataIn);

        // Check the values.
        for (int i = 0; i < NX; i++)
            for (int j = 0; j < NY; j++)
                if (dataIn[i][j] != i * NY + j)
                    return NC_ERR;

        // The netCDF file is automatically closed by the NcFile destructor
        // cout << "*** SUCCESS reading example file simple_xy.nc!" << endl;

        return 0;
    }
    catch (NcException &e)
    {
        cout << "FAILURE**************************\n";
        cout << e.what() << endl;
        return NC_ERR;
    }
}

void NetCdfTest::_simple_xy_rd_test(void)
{
    const int result = simple_xy_rd();
    QCOMPARE(result, 0);
}