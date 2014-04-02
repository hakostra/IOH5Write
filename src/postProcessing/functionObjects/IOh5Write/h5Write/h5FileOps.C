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
#include "fileName.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::h5Write::fileCreate()
{
    Info<< "h5Write::fileCreate:" << endl;
    
    
    // Create directory if nonexisting
    if (!isDir("h5Data"))
    {
      mkDir("h5Data");
    }
    
    
    // Find and create filename
    char dataFile[80];
    int i = 0;
    do
    {
        sprintf(dataFile, "h5Data/%s%i.h5", name_.c_str(), i);
        i++;
    }
    while (isFile(dataFile));
    
    
    // Set up file access property list with parallel I/O access
    hid_t plistID = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plistID, MPI_COMM_WORLD, MPI_INFO_NULL);
    
    // Create a new file collectively.
    fileID_ = H5Fcreate
        (
            dataFile,
            H5F_ACC_TRUNC, 
            H5P_DEFAULT,
            plistID
        );
    
    // Close the property list
    H5Pclose(plistID);
    
    // Print info to terminal
    Info<< "  Chosen filename " << dataFile << endl << endl;
}


void Foam::h5Write::fileClose()
{
    MPI_Barrier(MPI_COMM_WORLD);
    Info<< "h5Write::fileClose" << endl << endl;
    
    // Close the file
    H5Fclose(fileID_);
    
}

// ************************************************************************* //
