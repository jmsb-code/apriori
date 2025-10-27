Collaboration Statement: I completed this homework on my own, without outside help from classmates or otherwise.

Build Requirements:
-make
-g++ compiler
-C++ standard libraries (should be compatible with version 11+ but I compiled with 17)
-C standard libaries

Build Instructions: run 'make' in directory with the following files
-apriori.cpp
-Init.h
-Your dataset

Usage: 
'apriori(.exe) [-d] [-f] [-s] [-e prog_data_file] infile minsup outfile'

Note: The first non-flagged input passed to the program will saved as input filename string, the next will be cast to an int for minsup, the third will be saved as output filename string, any following non-flagged arguments will be disregarded.

Bracketed flags are optional and function as follows:
-d: Print verbose output to stdout about many operations (debug mode)
	Note: Use as first argument to print verbose information about parsing of subsequent arguments as well as input/output file assignments and later operations.

-f: Print first observation to stdout formatted as [entry0, entry1, ...]
	Note: Helper to check that dataset (stored as strings) is being properly imported into program memory

-s: Indicate that output should be sorted lexicographically
	Note: If flag is not set, after algorithm runs the frequent patterns will be cast to ints and resorted before output is written

-e: Print program databank as it stands before termination of program 
	Note: Will write an empty file on successful execution, must be proceeded by path to file

Usage requirements:
Argument infile expects the filename of an ascii text file with an observation per line and entries deliniated by spaces.
Results will be written to outfile, which will be created or overwritten as an ascii text file.
These arguments are interpreted as paths from the current directory.
Datasets that contain terms which are not integers should be run with the -s flag and output will be sorted lexicographically.
Each line of the input is expected to be a set, repeat values within a line of the input file may not function correctly