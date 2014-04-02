 /*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |               2012-2014 Håkon Strandenes
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "h5Write.H"
#include "cellModeller.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::h5Write::meshWrite()
{
    Info<< "h5Write::meshWrite:" << endl;
    
    // Find over all (global) number of cells per process
    nCells_[Pstream::myProcNo()] = mesh_.cells().size();
    Pstream::gatherList(nCells_);
    Pstream::scatterList(nCells_);
    
    // Write mesh
    meshWritePoints();
    meshWriteCells();
    
    Info<< endl;
}


void Foam::h5Write::meshWritePoints()
{   
    Info<< "  meshWritePoints" << endl;
    
    const pointField& points = mesh_.points();
    
    
    // Find out how many points each process has
    List<label> nPoints(Pstream::nProcs());
    nPoints[Pstream::myProcNo()] = points.size();
    Pstream::gatherList(nPoints);
    Pstream::scatterList(nPoints);
    
    
    // Create the different datasets (needs to be done collectively)
    char datasetName[80];
    hsize_t dimsf[2];
    hid_t fileSpace;
    hid_t dsetID;
    hid_t plistID;
    
    forAll(nPoints, proc)
    {
        
        // Create the dataspace for the dataset
        dimsf[0] = nPoints[proc];
        dimsf[1] = 3;
        fileSpace = H5Screate_simple(2, dimsf, NULL);
        
        // Set property to create parent groups as neccesary
        plistID = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(plistID, 1);
        
        // Create the dataset for points
        sprintf
            (
                datasetName,
                "MESH/%s/processor%i/POINTS",
                mesh_.time().timeName().c_str(),
                proc
            );
        
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistID,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        H5Dclose(dsetID);
        H5Pclose(plistID);
        H5Sclose(fileSpace);
    }
    
    
    // Create a simple array of points (to pass on to H5Dwrite)
    ioScalar pointList[points.size()][3];
    forAll(points, ptI)
    {
        pointList[ptI][0] = points[ptI].x();
        pointList[ptI][1] = points[ptI].y();
        pointList[ptI][2] = points[ptI].z();
    }
    
    // Open correct dataset for this process
    sprintf
        (
            datasetName,
            "MESH/%s/processor%i/POINTS",
            mesh_.time().timeName().c_str(),
            Pstream::myProcNo()
        );
    dsetID = H5Dopen2(fileID_, datasetName, H5P_DEFAULT);
    
    
    // Create property list for collective dataset write.
    plistID = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plistID, H5_XFER_MODE);
    
    
    // Do the actual write
    H5Dwrite
        (
            dsetID, 
            H5T_SCALAR,
            H5S_ALL,
            H5S_ALL,
		        plistID,
		        pointList
		    );
    
    // Close/release resources.
    H5Pclose(plistID);
    H5Dclose(dsetID);
}


void Foam::h5Write::meshWriteCells()
{
    Info<< "  meshWriteCells" << endl;

    // Map shapes OpenFOAM->XDMF
    Map<label> shapeLookupIndex;
    shapeLookupIndex.insert(hexModel->index(), 9);
    shapeLookupIndex.insert(prismModel->index(), 8);
    shapeLookupIndex.insert(pyrModel->index(), 7);
    shapeLookupIndex.insert(tetModel->index(), 6);
    
    
    const cellList& cells  = mesh_.cells();
    const cellShapeList& shapes = mesh_.cellShapes();
    
    
    // Find dataset length for this process and fill dataset in one operation
    // this will possible give a little overhead w.r.t. storage, but on a 
    // hex-dominated mesh, this is OK.
    int j = 0;
    int myDataset[9*cells.size()];
    forAll(cells, cellId)
    {
        const cellShape& shape = shapes[cellId];
        label mapIndex = shape.model().index();
        
        // A registered primitive type
        if (shapeLookupIndex.found(mapIndex))
        {
            label shapeId = shapeLookupIndex[mapIndex];
            const labelList& vrtList = shapes[cellId];
            
            myDataset[j] = shapeId; j++;
            forAll(vrtList, i)
            {
                myDataset[j] = vrtList[i]; j++;
            }
        }
        
        // If the cell is not a basic type, exit with an error
        else
        {
            FatalErrorIn
            (
                "h5Write::meshWriteCells()"
            )   << "Unsupported or unknown cell type for cell number "
                << cellId << endl
                << exit(FatalError);
        }
    }
    
    
    // Find out how many points each process has
    List<label> datasetSizes(Pstream::nProcs());
    datasetSizes[Pstream::myProcNo()] = j;
    Pstream::gatherList(datasetSizes);
    Pstream::scatterList(datasetSizes);
    
    
    // Create the different datasets (needs to be done collectively)
    char datasetName[80];
    hsize_t dimsf[1];
    hid_t fileSpace;
    hid_t dsetID;
    hid_t attrID;
    hid_t plistID;
    
    forAll(datasetSizes, proc)
    {
        // Set property to create parent groups as neccesary
        plistID = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(plistID, 1);
        
        
        // Create dataspace for cell list
        dimsf[0] = datasetSizes[proc];
        fileSpace = H5Screate_simple(1, dimsf, NULL);
        
        sprintf
            (
                datasetName,
                "MESH/%s/processor%i/CELLS",
                mesh_.time().timeName().c_str(),
                proc
            );
        
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_NATIVE_INT,
                fileSpace,
                plistID,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        H5Sclose(fileSpace);
        
        
        // Create and write attributte to store number of cells
        dimsf[0] = 1;
        fileSpace = H5Screate_simple(1, dimsf, NULL);
        
        attrID = H5Acreate2
            (
                dsetID,
                "nCells",
                H5T_NATIVE_INT,
                fileSpace,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        
        int nCells = nCells_[proc];
        H5Awrite
            (
                attrID, 
                H5T_NATIVE_INT,
	              &nCells
	          );
        
        H5Aclose(attrID);
        H5Sclose(fileSpace);
        
        // Close last access pointers
        H5Dclose(dsetID);
        H5Pclose(plistID);
    }
    
    
    // Create property list for collective dataset write.
    plistID = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plistID, H5_XFER_MODE);
    
    
    // Open and write correct dataset for this process
    sprintf
        (
            datasetName,
            "MESH/%s/processor%i/CELLS",
            mesh_.time().timeName().c_str(),
            Pstream::myProcNo()
        );
    dsetID = H5Dopen2(fileID_, datasetName, H5P_DEFAULT);
    
    H5Dwrite
        (
            dsetID, 
            H5T_NATIVE_INT,
            H5S_ALL,
            H5S_ALL,
		        plistID,
		        myDataset
		    );
    
    H5Pclose(plistID); 
    H5Dclose(dsetID);
}


const Foam::cellModel* Foam::h5Write::unknownModel = Foam::cellModeller::
lookup
(
    "unknown"
);


const Foam::cellModel* Foam::h5Write::tetModel = Foam::cellModeller::
lookup
(
    "tet"
);


const Foam::cellModel* Foam::h5Write::pyrModel = Foam::cellModeller::
lookup
(
    "pyr"
);


const Foam::cellModel* Foam::h5Write::prismModel = Foam::cellModeller::
lookup
(
    "prism"
);


const Foam::cellModel* Foam::h5Write::hexModel = Foam::cellModeller::
lookup
(
    "hex"
);

// ************************************************************************* //
