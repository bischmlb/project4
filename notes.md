# Notes

Block Size
 - 4096 bytes (typical size of memory page, 1:1 mapping when reading / writting block data)
 - a smaller block size could allow for less waste

Inode
 - access / mode bitmap?
 - size size_t
 - timestamps (there's some default time unit)
 - 15 direct data pointers (?)
 - 1 pointer to list of pointers containing the same
 - unique ID
 - filename (*char [32])

Standard Inode

| Data                   | Bytes     |
|------------------------|-----------|
| unique_id              | 32        |
| size                   | 16        |
| last_modified          | 32        |
| created                | 32        |
| filename               | 64        |
| 15 direct datapointers | 15*32=480 |
| data_array             | 32        |
| data                   | 3408      |

Root Inode

| Data                   | Bytes     |
|------------------------|-----------|
| unique_id              | 32        |
| size                   | 16        |
| last_modified          | 32        |
| created                | 32        |
| filename               | 64        |
| 15 direct datapointers | 15*32=480 |
| data_array             | 32        |
| block_bitmap           | 313       |
| leftover_bytes         | 3095      |


Disk / root Inode
 - root inode stored at 0
 - contains info about disk (available blocks)
 - Free space is found via root / disk inode.
 - File count is limited by block count (2499)
 - Maximum file / dir count is block count - 1 (root inode)
 - continguos space found via bitmap contained in root inode

Implementation notes
 - mode / access for inodes
 - REMEMBER TO ADD SUPPORT FOR INDIRECT FILES / DIRS. (readdir, path_to_inode)
 - Magic number in datapointers array in inode! 15 and 2500
 - path_to_inode should only work on directories (check mode bit)
