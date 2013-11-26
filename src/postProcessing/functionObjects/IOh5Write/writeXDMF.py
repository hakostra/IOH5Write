#!/usr/bin/env python3

"""writeXDMF.py: Script to parse an HDF5 file written by OpenFOAM and write corresponding XDMF file"""

__author__ = "Håkon Strandenes"
__email__ = "hakon.strandenes@ntnu.no"
__copyright__ = "Copyright 2012-2013"
__license__ = "GPL v3 or later"


import h5py
import numpy
import math
import re
import argparse
import os


# Define a dictionary with the XDMF attribute types
xdmfAttrTypes = {1: 'Scalar', 3: 'Vector', 6: 'Tensor6', 9: 'Tensor'}


# Checks is there are any clouds present and returns a list with their names
def detectClouds(f) :
    # Check if the file contain any clouds
    if ('CLOUDS' in f['/']) is False :
        print('Note: No clouds present in file \'{}\''.format(args.input))
        return False
    
    clouds = list(f['CLOUDS'].keys())
    return clouds


# Function that checks is there are any fields present
def detectFields(f) :
    # Check if the file contain any fields
    if ('MESH' in f) is False or ('FIELDS' in f) is False :
        print('Error: No mesh and field data present in file \'{}\''.format(fileName))
        return False
    
    return True


# Parse and write clouds
def writeCloud(cloud, fo, args) :
    
    # Create a list of scalar time values
    timeNames = list(cloud.keys())
    timeValues = [float(i) for i in timeNames]
    timeIndex = numpy.argsort(timeValues)


    # Determine the precision of the data in the cloud
    prec = cloud[timeNames[0]]['position'].dtype.itemsize
    
    
    # Find name and path to HDF5-file relative to XDMF-file
    h5Path = os.path.relpath(cloud.parent.parent.file.filename, os.path.dirname(fo.name))
    
    
    # Print header
    fo.write('<Xdmf>\n')
    fo.write('  <Domain>\n')
    fo.write('    <Grid Name="cloudData" GridType="Collection" CollectionType="Temporal">\n\n')
    
    
    # Loop over all times
    for index in timeIndex :
        
        # Skip time = 0 if required
        if args.noZero and index == timeIndex[0] and math.fabs(timeValues[index] - 0) < 1e-8 :
            continue
        
        # Skip if it only is to process latest timestep
        if args.latestTime and index != timeIndex[timeIndex.size-1] :
            continue
        
    
        timeName = timeNames[index]
        pathInFile = '{}:{}/{}'.format(h5Path, cloud.name, timeName)
        nPoints = cloud[timeName]['position'].shape[0]
        
        fo.write('      <Grid Name="time{}" Type="Uniform">\n'.format(timeValues[index]))
        fo.write('        <Time Type="Single" Value="{}" />\n'.format(timeValues[index]))
        
        # Geometry definition
        fo.write('        <Geometry GeometryType="XYZ">\n')
        fo.write('          <DataStructure Dimensions="{} 3" NumberType="Float" Presicion="{}" Format="HDF" >\n'.format(nPoints, prec))
        fo.write('            {}/position\n'.format(pathInFile))
        fo.write('          </DataStructure>\n')
        fo.write('        </Geometry>\n')
        
        # Topology information
        fo.write('        <Topology Type="Polyvertex" NodesPerElement="1" NumberOfElements="{}">\n'.format(nPoints))
        fo.write('        </Topology>\n')
        
        
        # Loop over all attributes
        for attr in cloud[timeName] :
            h = cloud[timeName][attr]
            nCmp =  int(h.size/h.shape[0])
            
            # Determine number type (Int, Float)
            if h.dtype in (numpy.dtype('f4'), numpy.dtype('f8')):
                numberType = 'Float'
            elif h.dtype in (numpy.dtype('i4'), numpy.dtype('i8')):
                numberType = 'Int'
            else :
                print('Error: Cloud \'{}\' was created using an unknown number type.'.format(cloud.name))
                exit(1)
            
            # Write data
            fo.write('        <Attribute Name="{}" Center="Node" AttributeType="{}">\n'.format(attr, xdmfAttrTypes[nCmp]))
            fo.write('          <DataStructure Dimensions="{} {}" NumberType="{}" Presicion="{}" Format="HDF" >\n'.format(nPoints, nCmp, numberType, prec))
            fo.write('            {}/{}\n'.format(pathInFile, attr))
            fo.write('          </DataStructure>\n')
            fo.write('        </Attribute>\n')
            
        
        # End of current time
        fo.write('      </Grid>\n\n\n')


    # Print file footer
    fo.write('    </Grid>\n')
    fo.write('  </Domain>\n')
    fo.write('</Xdmf>\n');
    
    # Return control
    return


# Parse and write mesh/fields
def writeFields(f, fo, args) :
    
    # Sone useful handles
    mesh = f['MESH/0']
    fields = f['FIELDS']


    # Find number of processes
    nProcs = len(mesh)


    # Find number of points, cells and dataset length for each process
    nPoints = [None]*nProcs
    nCells = [None]*nProcs
    cellLength = [None]*nProcs
    procNo = [None]*nProcs

    i = 0
    for proc in mesh :
        attrs = h5py.AttributeManager(mesh[proc]['CELLS'])
        
        procNo[i] = int(re.findall(r'\d+', proc)[0])
        nCells[i] = attrs['nCells'][0]
        cellLength[i] = len(mesh[proc]['CELLS'])
        nPoints[i] = len(mesh[proc]['POINTS'])
        
        i += 1


    # Create a list of scalar time values
    timeNames = list(fields.keys())
    timeValues = [float(i) for i in timeNames]
    timeIndex = numpy.argsort(timeValues)


    # Determine the precision of the HDF5 file
    someFieldName = list(fields[timeNames[0]]['processor0'].keys())[0]
    prec = fields[timeNames[0]]['processor0'][someFieldName].dtype.itemsize
    
    
    # Find name and path to HDF5-file relative to XDMF-file
    h5Path = os.path.relpath(f.filename, os.path.dirname(fo.name))
    
    
    # Print header
    fo.write('<Xdmf>\n')
    fo.write('  <Domain>\n')
    fo.write('    <Grid Name="FieldData" GridType="Collection" CollectionType="Temporal">\n\n')
    
    
    # Loop over all times
    for index in timeIndex :
    
        # Skip time = 0 if required
        if args.noZero and index == timeIndex[0] and math.fabs(timeValues[index] - 0) < 1e-8 :
            continue
        
        # Skip if it only is to process latest timestep
        if args.latestTime and index != timeIndex[timeIndex.size-1] :
            continue
        
        
        timeName = timeNames[index]
        
        fo.write('      <Grid GridType="Collection" CollectionType="Spatial">\n'.format(timeValues[index]))
        fo.write('        <Time Type="Single" Value="{}" />\n'.format(timeValues[index]))
        
        # Loop over all processes
        i = 0
        for proc in mesh :
            fo.write('        <Grid Name="time{}-{}" Type="Uniform">\n'.format(timeValues[index], proc))
            
            # Geometry definition
            fo.write('          <Topology Type="Mixed" NumberOfElements="{}">\n'.format(nCells[i]))
            fo.write('            <DataStructure Dimensions="{}" NumberType="Int" Format="HDF" >\n'.format(cellLength[i]))
            fo.write('              {}:/MESH/0/{}/CELLS\n'.format(h5Path, proc))
            fo.write('            </DataStructure>\n')
            fo.write('          </Topology>\n')
            fo.write('          <Geometry GeometryType="XYZ">\n')
            fo.write('            <DataStructure Dimensions="{} 3" NumberType="Float" Presicion="{}" Format="HDF" >\n'.format(nPoints[i], prec))
            fo.write('              {}:/MESH/0/{}/POINTS\n'.format(h5Path, proc))
            fo.write('            </DataStructure>\n')
            fo.write('          </Geometry>\n')
            
            # Loop over all fields
            for field in fields[timeName][proc] :
                h = fields[timeName][proc][field]
                nCmp =  int(h.size/h.shape[0])
                
                fo.write('          <Attribute Name="{}" Center="Cell" AttributeType="{}">\n'.format(field, xdmfAttrTypes[nCmp]))
                fo.write('            <DataStructure Format="HDF" DataType="Float" Precision="{}" Dimensions="{} {}">\n'.format(prec, nCells[i], nCmp))
                fo.write('              {}:/FIELDS/{}/{}/{}\n'.format(h5Path, timeName, proc, field))
                fo.write('            </DataStructure>\n')
                fo.write('          </Attribute>\n')
            
            # Write processor footer
            fo.write('        </Grid>\n')
            
            # Increment processor counter
            i += 1
        
        # Write footer for time
        fo.write('      </Grid>\n\n\n')


    # Print footer
    fo.write('    </Grid>\n')
    fo.write('  </Domain>\n')
    fo.write('</Xdmf>\n');
    
    # Return control
    return



"""
Main part of program/script start below here
"""


# Read command line arguments
parser = argparse.ArgumentParser(description='Script to parse an HDF5 file written by OpenFOAM  and write corresponding XDMF file')
parser.add_argument('-i','--input', default='h5Data/h5Data0.h5', help='Input file name (default: \'h5Data/h5Data0.h5\')', required=False)
parser.add_argument('-d','--dir', default='xdmf', help='Output directory (default: \'xdmf\')', required=False)
parser.add_argument('-l','--noLagrangian', action='store_true', help='Skip Lagrangian clouds', required=False)
parser.add_argument('-f','--noFields', action='store_true', help='Skip fields (mesh) data', required=False)
parser.add_argument('-z','--noZero', action='store_true', help='Do not process time zero (t=0)', required=False)
parser.add_argument('-t','--latestTime', action='store_true', help='Only process latest timestep present', required=False)

args = parser.parse_args()

print('Reading from \'{}\''.format(args.input))
print('Printing XDMF files to folder \'{}\''.format(args.dir))


# Check if file exist
if os.path.isfile(args.input) is False :
    print('Error: File \'{}\' does not exist!'.format(args.input))
    exit(1)


# Open HDF5 file
f = h5py.File(args.input, 'r')


# Check if HDF5 directory exist, create if not
try:
    os.stat(args.dir)
except:
    os.mkdir(args.dir)  


# Write fields if present
fieldsPresent = detectFields(f)
if args.noFields is False and fieldsPresent :
    fo = open('{}/fieldData.xdmf'.format(args.dir), 'w')
    print('Writing field data to file {}'.format(fo.name))
    writeFields(f, fo, args)
    fo.close()
else :
    print('Skipping fields')


# Detect, loop over clouds and write them
clouds = detectClouds(f)
if args.noLagrangian is False and clouds:
    for cloud in clouds :
        fo = open('{}/{}.xdmf'.format(args.dir, cloud), 'w')
        H = f['CLOUDS'][cloud]
        print('Writing cloud data for \'{}\' to file {}'.format(cloud, fo.name))
        writeCloud(H, fo, args)
        fo.close()
else :
    print('Skipping Lagrangian clouds')


# Close HDF5 file
f.close()
