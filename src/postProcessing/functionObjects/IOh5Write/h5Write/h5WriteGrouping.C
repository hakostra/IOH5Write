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

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


Foam::label Foam::h5Write::appendFieldGroup
(
    const word& fieldName,
    const word& fieldType
)
{
    if (fieldType == volScalarField::typeName)
    {
        scalarFields_.append(fieldName);
        return 1;
    }
    else if (fieldType == volVectorField::typeName)
    {
        vectorFields_.append(fieldName);
        return 1;
    }
    else if (fieldType == volSphericalTensorField::typeName)
    {
        //sphericalTensorFields_.append(fieldName);
        return 0;
    }
    else if (fieldType == volSymmTensorField::typeName)
    {
        //symmTensorFields_.append(fieldName);
        return 0;
    }
    else if (fieldType == volTensorField::typeName)
    {
        //tensorFields_.append(fieldName);
        return 0;
    }

    return 0;
}


Foam::label Foam::h5Write::classifyFields()
{
    label nFields = 0;
    
    // Check currently available fields
    wordList allFields = mesh_.sortedNames();
    labelList indices = findStrings(objectNames_, allFields);

    forAll(indices, fieldI)
    {
        const word& fieldName = allFields[indices[fieldI]];

        nFields += appendFieldGroup
        (
            fieldName,
            mesh_.find(fieldName)()->type()
        );
    }

    return nFields;
}

// ************************************************************************* //
