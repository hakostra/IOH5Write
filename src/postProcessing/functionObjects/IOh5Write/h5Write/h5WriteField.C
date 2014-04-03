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

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::h5Write::fieldWrite()
{
    Info<< "  h5Write::fieldWrite:"  << endl;
    fieldWriteScalar();
    fieldWriteVector();
}


void Foam::h5Write::fieldWriteScalar()
{
    forAll(scalarFields_, fieldI)
    {
        Info<< "    fieldWriteScalar: " << scalarFields_[fieldI] << endl;
        
        // Lookup field
        const volScalarField& field = obr_.lookupObject<volScalarField>
            (
                scalarFields_[fieldI]
            );
        
        
        // Initialize a plain continous array for the data
        ioScalar* scalarData;
        scalarData = new ioScalar[field.size()];
        
        
        // Loop through the field and construct the array
        forAll(field, iter)
        {
            scalarData[iter] = field[iter];
        }
        
        
        // Create the different datasets (needs to be done collectively)
        char datasetName[80];
        hsize_t dimsf[1];
        hid_t fileSpace;
        hid_t dsetID;
        hid_t plistID;
        hid_t plistDCreate;
        
        forAll(nCells_, proc)
        {
            // Create the dataspace for the dataset
            dimsf[0] = nCells_[proc];
            
            fileSpace = H5Screate_simple(1, dimsf, NULL);
            
            // Set property to create parent groups as neccesary
            plistID = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(plistID, 1);
            
            // Set chunking, compression and other HDF5 dataset properties
            plistDCreate = H5Pcreate(H5P_DATASET_CREATE);
            dsetSetProps(1, sizeof(ioScalar), nCells_[proc], plistDCreate);
            
            // Create the dataset for points
            sprintf
                (
                    datasetName,
                    "FIELDS/%s/processor%i/%s",
                    mesh_.time().timeName().c_str(),
                    proc,
                    scalarFields_[fieldI].c_str()
                );
            
            dsetID = H5Dcreate2
                (
                    fileID_,
                    datasetName,
                    H5T_SCALAR,
                    fileSpace,
                    plistID,
                    plistDCreate,
                    H5P_DEFAULT
                );
            H5Dclose(dsetID);
            H5Pclose(plistID);
            H5Pclose(plistDCreate);
            H5Sclose(fileSpace);
        }
        
        
        // Open correct dataset for this process
        sprintf
            (
                datasetName,
                "FIELDS/%s/processor%i/%s",
                mesh_.time().timeName().c_str(),
                Pstream::myProcNo(),
                scalarFields_[fieldI].c_str()
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
		            scalarData
		        );
        
        // Close/release resources.
        H5Pclose(plistID);
        H5Dclose(dsetID);
        
        // Release memory
        delete [] scalarData;
    }
}


void Foam::h5Write::fieldWriteVector()
{
    forAll(vectorFields_, fieldI)
    {
        Info<< "    fieldWriteVector: " << vectorFields_[fieldI] << endl;
        
        const volVectorField& field = obr_.lookupObject<volVectorField>
            (
                vectorFields_[fieldI]
            );
        
        //Initialize a plain continous array for the data
        ioScalar* vectorData;
        vectorData = new ioScalar[field.size()*3];
        
        
        // Loop through the field and construct the array
        forAll(field, iter)
        {
            vectorData[3*iter+0] = field[iter].x();
            vectorData[3*iter+1] = field[iter].y();
            vectorData[3*iter+2] = field[iter].z();
        }
        
        
        // Create the different datasets (needs to be done collectively)
        char datasetName[80];
        hsize_t dimsf[2];
        hid_t fileSpace;
        hid_t dsetID;
        hid_t plistID;
        hid_t plistDCreate;
        
        forAll(nCells_, proc)
        {
            
            // Create the dataspace for the dataset
            dimsf[0] = nCells_[proc];
            dimsf[1] = 3;
            fileSpace = H5Screate_simple(2, dimsf, NULL);
            
            // Set property to create parent groups as neccesary
            plistID = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(plistID, 1);
            
            // Set chunking, compression and other HDF5 dataset properties
            plistDCreate = H5Pcreate(H5P_DATASET_CREATE);
            dsetSetProps(3, sizeof(ioScalar), nCells_[proc], plistDCreate);
            
            // Create the dataset for points
            sprintf
                (
                    datasetName,
                    "FIELDS/%s/processor%i/%s",
                    mesh_.time().timeName().c_str(),
                    proc,
                    vectorFields_[fieldI].c_str()
                );
            
            dsetID = H5Dcreate2
                (
                    fileID_,
                    datasetName,
                    H5T_SCALAR,
                    fileSpace,
                    plistID,
                    plistDCreate,
                    H5P_DEFAULT
                );
            
            H5Dclose(dsetID);
            H5Pclose(plistID);
            H5Pclose(plistDCreate);
            H5Sclose(fileSpace);
        }
        
        
        // Open correct dataset for this process
        sprintf
            (
                datasetName,
                "FIELDS/%s/processor%i/%s",
                mesh_.time().timeName().c_str(),
                Pstream::myProcNo(),
                vectorFields_[fieldI].c_str()
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
		            vectorData
		        );
        
        // Close/release resources.
        H5Pclose(plistID);
        H5Dclose(dsetID);
        
        // Release memory
        delete [] vectorData;
    }
}


// ************************************************************************* //
