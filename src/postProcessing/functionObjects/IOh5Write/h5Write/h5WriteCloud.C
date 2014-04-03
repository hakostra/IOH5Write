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

#include "basicKinematicCloud.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::h5Write::cloudWrite()
{
    forAll(cloudNames_, cloudI)
    {
        Info<< "  cloudWrite: " << cloudNames_[cloudI] << endl;
        
        // Thanks to Johan Spång for providing instructions on how to access
        // cloud data
        // (http://www.cfd-online.com/Forums/openfoam/75528-access-particle-data-functionobject.html)
        const kinematicCloud& cloud = obr_.lookupObject<kinematicCloud>
            (
                cloudNames_[cloudI]
            );
        basicKinematicCloud *q = (basicKinematicCloud*) &cloud;
        
        
        // Number of particles on this process
        label myParticles = (*q).size();
        
        // Find the number of particles on each process
        List<label> nParticles(Pstream::nProcs());
        nParticles[Pstream::myProcNo()] = myParticles;
        Pstream::gatherList(nParticles);
        Pstream::scatterList(nParticles);
        
        // Sum total number of particles on all processes
        label nTot = sum(nParticles);
        
        // If the cloud contains no particles, jump to the next cloud
        if (nTot == 0)
        {
            Info<< "    " << cloudNames_[cloudI] <<": No particles in cloud. "
                << "Skipping write." <<endl;
            continue;
        }
        
        // Calculate offset values
        List<label> offsets(Pstream::nProcs());
        offsets[0] = 0;
        for (label proc=1; proc < offsets.size(); proc++)
        {
            offsets[proc] = offsets[proc-1] + nParticles[proc-1];
        }
        
        
        // Define a variable for dataset name
        char datasetName[80];
        
        
        // Allocate memory for 1-comp. dataset of type 'label'
        label* particleLabel;
        particleLabel = new label[myParticles];
        
        // Write original processor ID
        if (findStrings(cloudAttribs_, "origProc"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleLabel[i] = pIter().origProc();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/origProc",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleLabel,
                    datasetName,
                    H5T_NATIVE_INT
                );
        }  
        
        // Write original ID
        if (findStrings(cloudAttribs_, "origId"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleLabel[i] = pIter().origId();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/origId",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleLabel,
                    datasetName,
                    H5T_NATIVE_INT
                );
        } 
        
        // Write cell number
        if (findStrings(cloudAttribs_, "cell"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleLabel[i] = pIter().cell();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/cell",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleLabel,
                    datasetName,
                    H5T_NATIVE_INT
                );
        }
        
        // Write current process ID
        if (findStrings(cloudAttribs_, "currProc"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleLabel[i] = Pstream::myProcNo();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/currProc",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleLabel,
                    datasetName,
                    H5T_NATIVE_INT
                );
        }
        
        // Free memory for 1-comp. dataset of type 'label'
        delete [] particleLabel;
        
        
        // Allocate memory for 1-comp. dataset of type 'ioScalar'
        ioScalar* particleScalar1;
        particleScalar1 = new ioScalar[myParticles];
        
        // Write density rho
        if (findStrings(cloudAttribs_, "rho"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar1[i] = pIter().rho();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/rho",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleScalar1,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Write diameter d
        if (findStrings(cloudAttribs_, "d"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar1[i] = pIter().d();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/d",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleScalar1,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Write age
        if (findStrings(cloudAttribs_, "age"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar1[i] = pIter().age();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/age",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    1,
                    (void*) particleScalar1,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Free memory for 1-comp. dataset of type 'ioScalar'
        delete [] particleScalar1;
        
        
        // Allocate memory for 3-comp. dataset of type 'ioScalar'
        ioScalar* particleScalar3;
        particleScalar3 = new ioScalar[myParticles*3];
        
        // Write position
        if (findStrings(cloudAttribs_, "position"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar3[3*i+0] = pIter().position().x();
                particleScalar3[3*i+1] = pIter().position().y();
                particleScalar3[3*i+2] = pIter().position().z();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/position",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    3,
                    (void*) particleScalar3,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Write velocity U
        if (findStrings(cloudAttribs_, "U"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar3[3*i+0] = pIter().U().x();
                particleScalar3[3*i+1] = pIter().U().y();
                particleScalar3[3*i+2] = pIter().U().z();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/U",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    3,
                    (void*) particleScalar3,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Write slip velocity Us = U - Uc
        if (findStrings(cloudAttribs_, "Us"))
        {
            label i = 0;
            forAllIter(basicKinematicCloud, *q, pIter)
            {
                particleScalar3[3*i+0] = pIter().U().x() - pIter().Uc().x();
                particleScalar3[3*i+1] = pIter().U().y() - pIter().Uc().y();
                particleScalar3[3*i+2] = pIter().U().z() - pIter().Uc().z();
                i++;
            }
            
            sprintf
                (
                    datasetName,
                    "CLOUDS/%s/%s/Us",
                    cloudNames_[cloudI].c_str(),
                    mesh_.time().timeName().c_str()
                );
            
            cloudWriteAttrib
                (
                    myParticles,
                    offsets[Pstream::myProcNo()],
                    nTot,
                    3,
                    (void*) particleScalar3,
                    datasetName,
                    H5T_SCALAR
                );
        }
        
        // Free memory for 3-comp. dataset of type 'ioScalar'
        delete [] particleScalar3;
        
    }
}


void Foam::h5Write::cloudWriteAttrib
(
    label myParticles,
    label offset,
    label nTot,
    label nCmps,
    void* databuf,
    char* datasetName,
    hid_t H5type
)
{
    // Some variable declarations
    hsize_t dimsf[2];
    hsize_t start[2];
    hsize_t count[2];
    hid_t fileSpace;
    hid_t memSpace;
    hid_t dsetID;
    hid_t plistLCreate, plistDCreate, plistWrite;
    
    // Set dimension, start and count values
    start[0] = offset;
    start[1] = 0;
    count[0] = myParticles;
    count[1] = nCmps;
    dimsf[0] = nTot;
    dimsf[1] = nCmps;
    
    // Set property to create parent groups as neccesary
    plistLCreate = H5Pcreate(H5P_LINK_CREATE);
    H5Pset_create_intermediate_group(plistLCreate, 1);
    
    // Set chunking, compression and other HDF5 dataset properties
    plistDCreate = H5Pcreate(H5P_DATASET_CREATE);
    dsetSetProps(nCmps, sizeof(ioScalar), nTot, plistDCreate);
    
    // Create property list for dataset write.
    plistWrite = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plistWrite, H5_XFER_MODE);
    
    // Cretate filespace for data
    label nDims = 2;
    if (nCmps == 1)
    {
        nDims = 1;
    }
    
    fileSpace = H5Screate_simple(nDims, dimsf, NULL);
    H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL);
    
    // Create dataset
    dsetID = H5Dcreate2
        (
            fileID_,
            datasetName,
            H5type,
            fileSpace,
            plistLCreate,
            plistDCreate,
            H5P_DEFAULT
        );
    
    // Write actual data only if process holds data
    if ( myParticles > 0 )
    {
        memSpace = H5Screate_simple(nDims, count, NULL);
        H5Dwrite
            (
                dsetID, 
                H5type,
                memSpace,
                fileSpace,
                plistWrite,
                databuf
            );
        H5Sclose(memSpace);
    }
    
    // Close open handles
    H5Dclose(dsetID);
    H5Sclose(fileSpace);
    H5Pclose(plistLCreate);
    H5Pclose(plistDCreate);
    H5Pclose(plistWrite);
}

// ************************************************************************* //
