IOH5Write
=========

Library that write OpenFOAM cases as HDF5 archives instead of the default one-file-per-process-per-timestep-per-variable approach. This saves a lot of files, makes it easier to manage, copy, and post-process the results. An XDMF file is used to describe the contents of the HDF5-file and this can easily be opened in ParaView, VisIt or any other common postprocessor tool. The IO part is handled by MPI-IO, which makes it effective on clusters and high-performance computers with thousands of nodes and parallel file systems.


Installation
------------
1. Make sure you have a working copy of OpenFOAM 2.2.0, 2.2.1 or 2.2.x. Make sure that you have all necessary compilers and development libraries, including MPI.
2. Install the HDF5-library. Make sure that you install or compile it with parallel/MPI support. In Ubuntu this is done by installing the package ``libhdf5-openmpi-dev``. I guess it is necessary to compile both OpenFOAM and HDF5 against the same MPI library and version. I use the OpenMPI that is supplied with my system, and have compiled both OpenFOAM and HDF5 against this.
3. Grab a copy of this repository, and enter the code directory. You can for example place the code in your ``~/OpenFOAM/username-2.2.x`` folder.
4. Set the environment variable ``HDF5_DIR`` to your HDF5 installation directory (this might f.ex. be ``/usr``). Make the variable "visible" for the compile script (e.g. ``export HDF5_DIR=/usr``) before you compile.
5. Compile the code with the common ``./Allwmake`` and wait.

If you encounter any problems during the installation, this is probably because you have a problem with either the OpenFOAM installation, HDF5 library or have failed to follow the instructions properly. You should pay special attention to the HDF5 library and make sure that the parallel support is enabled.


Some hints on the HDF5-compilation
----------------------------------
I generally build HDF5 from scratch, as the pre-build binaries available around on the different systems I use is all build with different options. Building HDF5 does not require any special libraries, but it is easy to miss that HFD5 automatically disable the building of shared libraries (which is necessary for OpenFOAM) when building parallel versions.

I have found the following configuration well suited for all my needs:
``CC=mpicc CFLAGS=-fPIC LDFLAGS=-fPIC ./configure --prefix=/opt/HDF5/1.8.11/build --enable-parallel --enable-shared``
when building HDF5. This might of course not necessarily suit everyone, especially not the installation directory, but I find it useful to organize all my compiled-from-scratch software in ``/opt``.


Choosing between single and double precision IO
-----------------------------------------------
The solution of your problem (that is going to be written) is usually found by an iterative method, that gives an approximate answer. This approximation typically has 6-8 significant digits, far from the 15 significant digits in double precision. To save space (a lot actually) it is possible to write the solution data in single precision, independent on the precision settings in OpenFOAM.

This choice of output precision is done at compile time, and the default is single precision (32 bit floating point numbers). To switch, you set either -DWRITE_SP (for single prec.) or -DWRITE_DP (for double prec.) in the ``src/postProcessing/functionObjects/IOh5Write/Make/options`` file. See around line 9 for this option.


Writing XDMF files
------------------
The XDMF files is written *after* the simulation is finished by using the python script 'writeXDMF.py'. The script will, if not supplied with any additional arguments, parse the file 'h5Data/h5Data0.h5', and write the resulting XDMF files in a folder called 'xdmf'. One XDMF-file will be created for the field/mesh data, and one XDMF-file will be created for each cloud of particles. Usage instructions can be given with the option --help.

In case someone is interested, two (obsolete) Matlab-scripts that parse HDF5-files is also supplied. These do not have the same functionality or usability as the Python-script mentioned above, and they are not maintained.


Testing
-------
Test the code by running the attached cavity tutorial with the attached ``./Allrun`` script. A directory called h5Data should be created, in which the file h5Data0.h5 is created. To use the code on other cases, look in the ``controlDict`` file in this case.


Known bugs and limitations
--------------------------
There are a few known bugs and limitations:

1. The code only work in parallel. This is a consequence of the design of OpenFOAM, since ``MPI_Init`` is never called for serial runs, hence the parallel HDF5 library cannot use MPI-IO.
2. The XDMF file must be written after the simulation. This is not a bug, but a slight limitation.
3. No boundary data is written. This is a consequence of laziness. I have never needed this for my research, hence only the internal cell-centred data is written. Again, this is not a difficult addition, and if anyone had the time to add this, please contact me if you want me to include this in the public release.
4. The code is not very well structured, and does not utilize many of the object-oriented features C++ gives. This is partly because the HDF5 library is a pure C library, requiring you to deal with pointers to arrays and stuff, partly due to my lack of C++ skills. This is on top of the list of things that needs to be done, perhaps I will fix it when I find time.


Found yet another bug? Got suggestions for improvements?
----------------------------------------------
Feel free to contact me in the discussion thread at [http://www.cfd-online.com/Forums/openfoam-programming-development/122579-hdf5-io-library-openfoam.html](www.cfd-online.com).

