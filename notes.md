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
 - Magic number in datapointers array in inode! 15 and 2500, 32 dir len
 - path_to_inode should only work on directories (check mode bit)
 - write_disk is writing pointers at struct rather than values.
 - freeblock doesn't work
 - overwritten data isn't completely overwritten (if previous was larger.)
 - data is overwritten after claim_free_block()
 - root is always dir because root is loaded after each time.
 
 
 #Design Notes
  
  Inode design:

| Data                   | Bytes     |
|------------------------|-----------|
| unique_id              | 32        |
| size                   | 16        |
| last_modified          | 32        |
| created                | 32        |
| filename               | 32        |
| 15 direct datapointers | 15*32=480 |
| data_array             | 32        |
| block_bitmap           | 313       |
| leftover_bytes         | 3127      |

  - Inodes stored in blocks on disk, max 313 available, depends on bit map size.
  
 - Block size 4096 bytes(why? standard size), gives us some internal fragmentation.
  - For inodes, doesn't use 4096 size (Block size), therefore rest is unused.
  - If file is under 4096 bytes, it will leave some unused space in the block.
  
 - First-fit with memory management, which can give us some external fragmentation.
 
 - Location of free space is done by bit map, storing whether index/#block is used or free by 1 or 0.
 
 - Limit on filesize, considering indirect files isn't implemented, each file can only store 15 blocks of data (15 * 4096).
 - Limit on number of files/directories, depending on amount blocks not in use 1 inode for file/direct uses 1 block each.
 - Filename max 32 characters.
 
 - Directory structure is tree-structured. 
  - Direct datapointers in inode for directories points to child files/directories
  - Direct datapointers in inode for files points to the data for the files
  - Limit on path is not set, but depends on max amount of directories able to be created.
  - Directories can only be removed if empty.
 

 
 
 
 
 
