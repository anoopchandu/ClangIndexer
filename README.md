# ClangIndexer
ClangIndexer will index C/C++ files and that index can be searched for declarations of symbols.</br>
Source files to index should be specified in a file and should be given to this program.

When indexing is done program will present a prompt to give location {file, line, column} as input from console then location of declaration present at input location will be printed in console.

## Input file format:</br>
First line : number of files</br>
For each file :</br>
  File name on single line</br>
  number of arguments in next line</br>
  each arguments in new line</br>

### Sample Input file:</br>
1</br>
source.cpp</br>
2</br>
-I/usr/include/glib-2.0</br>
-I/usr/lib/x86_64-linux-gnu/glib-2.0/include</br>
