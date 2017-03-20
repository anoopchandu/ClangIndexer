# ClangIndexer
Input file format for specifying files to index.

First line : number of files

For each file :

file name on single line

number of arguments in next line

each arguments in new line

Sample Input:

1

source.cpp

2

-I/usr/include/glib-2.0

-I/usr/lib/x86_64-linux-gnu/glib-2.0/include
