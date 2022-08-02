Index_allocation
authored by Amir Abu Shanab.
208586214
==Description==

Disk_Sim.cpp :-
Disk allocation simulator (Index_allocation) written in c++ that simulates the way blocks are allocated to files and the way data is written onto a file.

== Functions==

9 functions :-

1) void listAll – this function prints out all the disk content and all the files that were created.
2) void fsFormat - this function takes in an int value to set the block size(default is 4),it deletes all the content of the file,
   initialize all the different variables that are used in this program.
3) int CreateFile – this function takes in thr file name as a string and checks if the file was already created before
   by going over the MainDir, if the file was not created before it creates a new one with the name that was given in
   and sends that file to the OpenFile function to give it an Fd number and open it.
4) string CloseFile – this function takes in the file fd as an int number and goes over the MainDir to look for the file
   if the file is found and it isn't closed already it goes ahead and closes the file and remove its Fd.
5) int WriteToFile - this function takes in 3 arguments, 1 is the File fd to find it in the MainDir vector,2 is the data that we want to write
   in the file,3 is the data length using this argument we can check if there is place for the data in the file and in the disk, and if so we write it
   in the correct place in the file, this function checks if the file has no index block and if so it goes ahead and gives it one by checking in the
   BitVector array for an empty block, after that it does the same precess for the buffer it looks for an empty place in the disk to write the data in.
6) int ReadFromFile - this function takes in 3 arguments, 1 is the File Fd so we can find it in the MainDir vector ,2 the buffer thar we want to store the read data in,
   3 is the length of the data that we desire to read from the file, this function goes over the index block of the file that we want to read from and jumps to the
   location that was taken from the index block and reads n = length characters from the file.
7) int DelFile - his function takes in the file name as an argument, and looks for the file in the MainDir vector, if the file was found, and it is closed
   meaning it is not in use , this function deletes all the content of the file and frees everything that is allocated to this file.
8) int FD_finder - this function goes over the FD_vector array and looks for the smallest FD available, this function is used when opening a file since it needs
   to be given an fd.
9) int BV_finder - this function goes over the BitVector vector and looks for an empty block on the disk, since this vector keeps track of the number of
   empty blocks in the Disk.

==Program Files==

Disk_Sim.cpp

==How to compile? ==

g++ main.cpp Disk_Sim.cpp -o run
./run

== Input: ==

The input if this program is dependant of the user :-

1) ListAll function.
2) fsFormat.
3) CreateFile.
4) OpenFile.
5) CloseFile.
6) WriteToFile.
7) ReadFromFile.
8) DelFile.

==Output: ==

calling the list all function will display all the content inside the disk.
and every other function has a return value.