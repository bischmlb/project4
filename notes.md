# Notes

Block Size
 - 4096 bytes (typical size of memory page)

Inode
 - access / mode bitmap?
 - size size_t
 - timestamps (there's some default time unit)
 - 15 direct data pointers (?)
 - 1 pointer to list of pointers containing the same
 - unique ID
 - filename (*char [32])
 
Disk / root Inode
 - root inode stored at 0
 - contains info about disk (available blocks)
 - Free space is found via root / disk inode.
 - File count is limited by block count.
 - Maximum file / dir count is block count - 1 (root inode)
 - continguos space found via bitmap contained in root inode
