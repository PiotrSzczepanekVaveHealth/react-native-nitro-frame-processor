CONTEXTVISION COMPANY CONFIDENTIAL
ContextVision CVIE SDK for Image Enhancement
Copyright (c) 2011-2026 ContextVision AB

=======================================================================
Release Notes
=======================================================================

Version 8.2.4.24143
--------------------------
NOTE: The parameter file in this SDK should not be used to 
evaluate neither image quality nor processing speed.

The versions of the included libraries / programs are:

    Library
    -------------------------------
    cvie64.so:  8.2.4.24143

    Test program
    -------------------------------
    cvietest64:  8.2.4.24143


=======================================================================
SDK Contents
=======================================================================
Description of the included files and directories:

Files           Explanation
-----------------------------------

include/cvie.hpp
                Header file for the CVIE++ API.
include/cvie.h
                Header file for the CVIE C API.

include/cvlm.hpp
                Header file for the CVLM++ API which is used for license management.
include/cvlm.h
                Header file for the CVLM C API which is used for license management.

include/cvem.h
                Error management functionality shared between CVIE and CVLM.

include/dynamic_library.hpp
                C++ wrapper class for loaded library functions.
include/dynamic_library.h
                Functionality to simplify explicit loading of the CVIE dynamic library.


include/cvie2.h
                Header for the deprecated CVIE2 C API.

include/cvie2.hpp
                Header for the deprecated CVIE2 C++ API.

include/us2d_tp.h
                Tuning Parameter definitions.

bin/cvie64.so
                This is the CVIE library.

bin/cvietest64
                This is a test program that uses cvie64.so. 
                The program can perform image enhancement using raw (headerless)
                files as input and output images. 
                See examples below on how to use cvietest64.

images/
                This directory contains one or more test images.

volumes/
                This directory contains one or more test volumes.

par/
                This directory contains one or more default parameter files.
                To use for testing only.
			
READMESDK.txt
                This text file.
			

=======================================================================
How to use cvietest
=======================================================================

The cvietest program can be used to test the CVIE library.

Using cvietest
----------------
First go to the bin directory. 
Example:

  $ cd bin

To get help about the options to cvietest64 just run the program without any
arguments. 

Example:

  $ ./cvietest64

Example of how to set a license activation key using ./cvietest64:

First use the -M switch to check available Product IDs:

  $ ./cvietest64 -M
  Index     Product ID
  ---------------------------------------------------------------------
  0         <Product ID>

Then use the -H switch to list available License Methods:

  $ ./cvietest64 -H
  Index     License Method           Device ID
  ---------------------------------------------------------------------
  0         Sentinel Super Pro       <Device ID>
  1         NIC                      <Device ID>

The Device ID is an identifier of the licensed hardware. Note that with
dongles the Device ID is the ID of the dongle as such, not the computer.
However, to use the dongle in another computer the activation key must be
installed there too as it is stored on the computer disk.

The Device ID can be used to retrieve an Activation Key for a Product ID for this
particular hardware. This is done in the ContextVision Customer Portal.

When you have retrieved an Activation Key, use the -K option to install it in the
computer. When doing this you must also provide the intended Product ID.

  $ ./cvietest64 -K <Product ID> <16 character activation key>

Now the license should be installed and this can be verified using the -C
switch.

  $ ./cvietest64 -C <Product ID>
  License OK!
