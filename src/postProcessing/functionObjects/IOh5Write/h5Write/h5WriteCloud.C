/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |               2012-2013 Håkon Strandenes
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
        
        
        // Allocate memory
        label* origProc;    origProc = new label[myParticles];
        label* origId;      origId   = new label[myParticles];
        label* cell;        cell     = new label[myParticles];
        
        ioScalar* position; position = new ioScalar[myParticles*3];
        ioScalar* U;        U        = new ioScalar[myParticles*3];
        ioScalar* Us;       Us       = new ioScalar[myParticles*3];
        
        ioScalar* rho;      rho      = new ioScalar[myParticles];
        ioScalar* d;        d        = new ioScalar[myParticles];
        ioScalar* age;      age      = new ioScalar[myParticles];
        
        
        label i = 0;
        
        // Loop through the particles and construct the arrays
        forAllIter(basicKinematicCloud, *q, pIter)
        {
            origProc[i]     = pIter().origProc();
            origId[i]       = pIter().origId();
            cell[i]         = pIter().cell();
            
            position[3*i+0] = pIter().position().x();
            position[3*i+1] = pIter().position().y();
            position[3*i+2] = pIter().position().z();
            
            U[3*i+0]        = pIter().U().x();
            U[3*i+1]        = pIter().U().y();
            U[3*i+2]        = pIter().U().z();
            
            Us[3*i+0]       = pIter().U().x() - pIter().Uc().x();
            Us[3*i+1]       = pIter().U().y() - pIter().Uc().y();
            Us[3*i+2]       = pIter().U().z() - pIter().Uc().z();
            
            rho[i]          = pIter().rho();
            d[i]            = pIter().d();
            age[i]          = pIter().age();
            
            i++;
        }
        
        
        
        // Some variable declarations
        char datasetName[80];
        hsize_t dimsf[2];
        hsize_t start[2];
        hsize_t count[2];
        hid_t fileSpace;
        hid_t memSpace;
        hid_t dsetID;
        hid_t plistCreate, plistWrite;
        
        // Set dimension, start and count values
        start[0] = offsets[Pstream::myProcNo()];
        start[1] = 0;
        count[0] = myParticles;
        count[1] = 3;
        dimsf[0] = nTot;
        dimsf[1] = 3;
        
        
        // Set property to create parent groups as neccesary
        plistCreate = H5Pcreate(H5P_LINK_CREATE);
        H5Pset_create_intermediate_group(plistCreate, 1);
        
        // Create property list for dataset write.
        plistWrite = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plistWrite, H5_XFER_MODE);
        
        
        // Filespace for particle IDs
        fileSpace = H5Screate_simple(1, dimsf, NULL);
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL);
        
        // Original processor ID
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/origProc",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_NATIVE_INT,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_NATIVE_INT,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    origProc
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // ID on that processor
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/origId",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_NATIVE_INT,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_NATIVE_INT,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    origId
                );
              H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Close filespace
        H5Sclose(fileSpace);
        
        
        // Cell of particle
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/cell",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_NATIVE_INT,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_NATIVE_INT,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    cell
                );
              H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Close filespace
        H5Sclose(fileSpace);
        
        
        
        // Filespace for particle positions and velocities
        fileSpace = H5Screate_simple(2, dimsf, NULL);
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL);
        
        // Position
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/position",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(2, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    position
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Velocity
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/U",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(2, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    U
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        
        // Slip velocity (i.e. difference between particle and fluid velocity)
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/Us",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(2, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    Us
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        
        // Close filespace
        H5Sclose(fileSpace);
        
        
        
        // Filespace for particle density, diameter and age
        fileSpace = H5Screate_simple(1, dimsf, NULL);
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL);
        
        // Density
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/rho",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    rho
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Diameter
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/d",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    d
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Age
        sprintf
            (
                datasetName,
                "CLOUDS/%s/%s/age",
                cloudNames_[cloudI].c_str(),
                mesh_.time().timeName().c_str()
            );
        dsetID = H5Dcreate2
            (
                fileID_,
                datasetName,
                H5T_SCALAR,
                fileSpace,
                plistCreate,
                H5P_DEFAULT,
                H5P_DEFAULT
            );
        if ( myParticles > 0 )
        {
            memSpace = H5Screate_simple(1, count, NULL);
            H5Dwrite
                (
                    dsetID, 
                    H5T_SCALAR,
                    memSpace,
                    fileSpace,
                    plistWrite,
                    age
                );
            H5Sclose(memSpace);
        }
        H5Dclose(dsetID);
        
        // Close filespace and property lists
        H5Sclose(fileSpace);
        H5Pclose(plistCreate);
        H5Pclose(plistWrite);
        
        
        // Release memory
        delete [] origProc;
        delete [] origId;
        delete [] cell;
        
        delete [] position;
        delete [] U;
        delete [] Us;
        
        delete [] rho;
        delete [] d;
        delete [] age;
    }
}


// ************************************************************************* //
