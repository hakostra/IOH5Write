/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |               2012 Håkon Strandenes
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
#include "dictionary.H"
#include "scalar.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(h5Write, 0);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::h5Write::h5Write
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles
)
:
    name_(name),
    obr_(obr),
    mesh_(refCast<const fvMesh>(obr))
{
    // Read dictionary
    read(dict);
    
    
    // Calssify fields
    nFields_ = classifyFields();
    
    
    // Initialize file
    fileCreate();
    
    
    // Only do if some fields are to be written
    if (nFields_)
    {
        // Set length of cell numbers array
        nCells_.setSize(Pstream::nProcs());
        
        // Write mesh and initial conditions
        meshWrite();
        write();
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::h5Write::~h5Write()
{
    // Close the HDF5 dataset
    fileClose();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::h5Write::read(const dictionary& dict)
{
    // Lookup in dictionary
    dict.lookup("objectNames") >> objectNames_;
    dict.lookup("cloudNames") >> cloudNames_;
    dict.lookup("cloudAttribs") >> cloudAttribs_;
    dict.lookup("writeInterval") >> writeInterval_;
    
    // Set next write NOW
    nextWrite_ = 0;
    timeSteps_ = 0;
    
    // Print info to terminal
    int writeprec = sizeof(ioScalar);
    Info<< type() << " " << name() << ":" << endl
        << "  Compiled with " << writeprec << " bytes precision." << endl
        << "  writing every " << writeInterval_ << " iterations:"
        << endl
        << "   ";
    
    // Do a basic check to see if the objectNames_ is accessible
    forAll(objectNames_, i)
    {
        if (obr_.foundObject<regIOobject>(objectNames_[i]))
        {
            Info<< " " << objectNames_[i];
        }
        else
        {
            WarningIn
            (
                "Foam::writeRegisteredObject::read(const dictionary&)"
            )   << "Object " << objectNames_[i] << " not found in "
                << "database. Available objects:" << nl << obr_.sortedToc()
                << endl;
        }

    }
    
    // Also print the cloud names
    forAll(cloudNames_, i)
    {
      Info<< " " << cloudNames_[i];
    }
    Info<< endl << endl;
    
    // Check if writeInterval is a positive number
    if (writeInterval_ <= 0)
    {
        FatalIOErrorIn("h5Write::read(const dictionary&)", dict)
            << "Illegal value for writeInterval " << writeInterval_
            << ". It should be > 0."
            << exit(FatalIOError);
    }
}


void Foam::h5Write::execute()
{
    // Nothing to be done here
}


void Foam::h5Write::end()
{
    // Nothing to be done here
}


void Foam::h5Write::timeSet()
{
    // Nothing to be done here
}


void Foam::h5Write::write()
{
    // Check if we are going to write
    if ( timeSteps_ == nextWrite_ )
    {
        // Write info to terminal
        Info<< "Writing HDF5 data for time " << obr_.time().timeName() << endl;
        
        // Only write field data if fields are specified
        if (nFields_)
        {
            // Re-write mesh if dynamic
            if (mesh_.changing())
            {
                meshWrite();
            }
            
            // Write field data
            fieldWrite();
        }
        
        // Only write cloud data if any clouds is specified
        if (cloudNames_.size())
        {
            cloudWrite();
        }
        
        // Flush file cache (in case application crashes before it is finished)
        H5Fflush(fileID_, H5F_SCOPE_GLOBAL);
        
        // Calculate time of next write
        nextWrite_ = timeSteps_ + writeInterval_;
    }
    
    // Update counter
    timeSteps_++; 
}


// ************************************************************************* //
