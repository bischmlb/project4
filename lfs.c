#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

int disk_init();
int open_disk();
int close_disk();
int claim_sequential_blocks(int);
int claim_free_block();
int free_block(int block);
int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_write( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_mkdir(const char *path, mode_t mode);
int lfs_mknod(const char *path, mode_t mode, dev_t dev);
int lfs_truncate(const char *path, off_t size, struct fuse_file_info *fi);
int lfs_rmdir(const char *path);

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = NULL,
	.rmdir = lfs_rmdir,
	.truncate = lfs_truncate,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = lfs_write,
	.rename = NULL,
	.utime = NULL
};

struct lfs_inode {
	mode_t mode;
	size_t size;
	int uid;
	char filename[32];
	time_t last_modified;
	time_t created;
	int data_blocks[15];
	int dp;
	char extra_data[313]; // used by root inode to track free blocks.
} lfs_inode;

/*
struct root_data {
	char blocks[2500];
} root_data;
*/

#define BLOCK_SIZE 4096
#define MIN(a,b) (((a)<(b))?(a):(b))
static int disk = -1;
static char *diskPath = "disk";

int write_inode(struct lfs_inode *inode, int block){
	int count, total;

	off_t loffset;

	// add size validation. (less than blocksize, more than 0)
	// SEE ABOVE COMMENT

	open_disk();
	if (disk == -1){
		printf("failed to write to disk, not open");
		close_disk();
		return -errno;
	}
	loffset = lseek(disk, block * BLOCK_SIZE, SEEK_SET);
	printf("disk write: %d, block: %d, offset: %ld\n",disk,block, loffset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	total = 0;
	// only allow writing within 1 block.
	count = write(disk, &(inode->mode), sizeof(inode->mode));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, &(inode->size), sizeof(inode->size));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, &(inode->uid), sizeof(inode->uid));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, inode->filename, sizeof(inode->filename));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, &(inode->last_modified), sizeof(inode->last_modified));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, &(inode->created), sizeof(inode->created));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, inode->data_blocks, sizeof(inode->data_blocks));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = write(disk, &(inode->dp), sizeof(inode->dp));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	//printf("writing extra data, extra_data[1]=%d sizeof is %ld\n",inode->extra_data[1],sizeof(inode->extra_data));
	count = write(disk, inode->extra_data, sizeof(inode->extra_data));
	if (count == -1){
		printf("failed to write at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	close_disk();
	return total;
}

int read_inode(struct lfs_inode *inode, int block){
	int count, total;
	off_t loffset;

	// add size validation. (less than blocksize, more than 0)
	// SEE ABOVE COMMENT

	open_disk();
	if (disk == -1){
		printf("failed to read to disk, not open\n");
		close_disk();
		return -errno;
	}
	loffset = lseek(disk, block * BLOCK_SIZE, SEEK_SET);
	printf("disk read: %d, block: %d, offset: %ld\n",disk,block, loffset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	total = 0;
	// only allow writing within 1 block.
	count = read(disk, &(inode->mode), sizeof(inode->mode));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;


	count = read(disk, &(inode->size), sizeof(inode->size));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, &(inode->uid), sizeof(inode->uid));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, inode->filename, sizeof(inode->filename));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, &(inode->last_modified), sizeof(inode->last_modified));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, &(inode->created), sizeof(inode->created));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, inode->data_blocks, sizeof(inode->data_blocks));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;
	//printf("read extra data, extra_data[1]=%d sizeof is %ld\n",inode->extra_data[1],sizeof(inode->extra_data));

	count = read(disk, &(inode->dp), sizeof(inode->dp));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;

	count = read(disk, inode->extra_data, sizeof(inode->extra_data));
	if (count == -1){
		printf("failed to read at %ld\n",loffset);
		close_disk();
		return -errno;
	}
	total+=count;


	close_disk();
	return total;
}

// seems to be needed for echo and nano
int lfs_truncate(const char *path, off_t size, struct fuse_file_info *fi){
/*	const void *msg = "testing\n";
	printf("msg: %d\n",msg);
	printf("msg: %s\n",msg);
	printf("truncate: (path=%s)\n", path);
	int count = write_disk(0, &msg, 0);
	printf("truncate count %d\n",count);
	if (count == -1){
		return -errno;
	} */
	return 0;
}

int disk_init(){
	struct lfs_inode *root_inode;
	int count;
	root_inode = malloc(BLOCK_SIZE);
	printf("size of inode: %d\n",BLOCK_SIZE);
	read_inode(root_inode, 0);
	printf("inode uid: %d\n", root_inode->uid);
	if(root_inode->uid != 1){
		root_inode->mode = S_IFDIR | 0755;
		root_inode->size = BLOCK_SIZE;
		root_inode->uid = 1;
		strcpy(root_inode->filename,"/");
		root_inode->last_modified = time(NULL);
		root_inode->created = time(NULL);
		memset(root_inode->extra_data,1,1);
		count = write_inode(root_inode, 0);
		printf("root initial count: %d\n",count);
	}
	free(root_inode);
	return 0;
}

int open_disk(){
	disk = open(diskPath, O_RDWR);
	if (disk == -1){
		printf("failed to open disk\n");
		return -errno;
	}
	//printf("disk opened\n");
	return 0;
}

int close_disk(){
	int res;
	res = close(disk);
	if (res == -1){
		printf("failed to close disk\n");
		return -errno;
	}
	//printf("disk closed\n");
	return 0;
}

/* read block and put in specified buffer, returns bytes read */
int read_disk( int block, void *buff, int offset, size_t size){
	int count;
	off_t loffset;

	// add size validation. (less than blocksize, more than 0)
	// SEE ABOVE COMMENT

	if (offset >= BLOCK_SIZE){
		return -EINVAL;
	}

	open_disk();
	if (disk == -1){
		//printf("failed to read from disk, not open");
		close_disk();
		return -errno;
	}
	loffset = lseek(disk, (block * BLOCK_SIZE) + offset, SEEK_SET);
	printf("disk read: %d, block: %d, offset: %ld,%d\n",disk,block, loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	memset(buff,0,size); // clean area first.
	count = read(disk, buff, size);
	if (count == -1){
		printf("failed to read at %ld + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	close_disk();
	return count;
}


int write_disk( int block, void *buff, int offset, size_t size){
	int count;
	off_t loffset;

	// add size validation. (less than blocksize, more than 0)
	// SEE ABOVE COMMENT

	if (offset >= BLOCK_SIZE){
		return -EINVAL;
	}

	open_disk();
	if (disk == -1){
		printf("failed to write to disk, not open");
		close_disk();
		return -errno;
	}
	loffset = lseek(disk, (block * BLOCK_SIZE) + offset, SEEK_SET);
	printf("disk write: %d, block: %d, offset: %ld,%d\n",disk,block, loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	// only allow writing within 1 block.
	count = write(disk, buff, size);
	if (count == -1){
		printf("failed to write at %ld + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	close_disk();
	return count;
}

int find_slash(const char *string, size_t offset){
	int i, j;
	j = -1;
	for (i = offset; i < strlen(string); i++ ){
		if (string[i] == '/'){
			j = i;
		}
	}
	return j;
}

int path_to_inode(const char *path, struct lfs_inode *cur_inode){
	char delim[1];
	char *slicer;
	int count, i, found;
	struct lfs_inode *new_inode;

	// verify that path is somewhat valid.
	if(path[0] != '/'){
		perror("first char not '/'");
		return -1;
	}

	// if we're looking for root inode, return once root has been read.
	//printf("are we looking for root?\n");
	if (path[1] == '\0') {
		read_inode(cur_inode, 0);
		//printf("we were, returning\n");
		return 0;
	}
	//printf("nahh\n");

	delim[0] = '/';
	slicer = strtok(path, delim);
	//slicer = strtok(NULL, delim);

	// read root inode first.
	count = read_inode(cur_inode, 0);
	if (count == -1){
		perror("read failed in path_to_inode");
		return -1;
	}

	// malloc before iterating.
	new_inode = malloc(BLOCK_SIZE);
	memset(new_inode,0,BLOCK_SIZE); // clean memory block so only disk info is kept.

	// /dm510/project4
	// iterate over each directory name seperated by /
	// REMEMBER TO ADD SUPPORT FOR INDIRECT FILES / DIRS.
	while(slicer != NULL){
		found = 0;
		for (i=0; i<15 && found != 1; i++){
			//printf("about to check a data_blocks index %d\n",i);
			// if we reach an index with 0, that means the dir can't be found.
			if (cur_inode->data_blocks[i] == 0){
				printf("no blocks found at %s i=%d\n", cur_inode->filename,i);
				return -1; // we weren't able to reach path.
			}
			//printf("About to read at block %d\n",cur_inode->data_blocks[i]);
			// read referenced inode and check filename and mode (must be dir)
			count = read_inode(new_inode,cur_inode->data_blocks[i]);
			if (count == -1){
				printf("read failed in path_to_inode loop");
				return -1;
			}
			//printf("reading node with UID %d\n",new_inode->uid);
			//printf("Read %d bytes from %s node found in %s\n",count,new_inode->filename, cur_inode->filename);
			//printf("is that equal to %s?\n",slicer); // no root /
			if (strcmp(new_inode->filename,slicer) == 0){ // dir found.
				//printf("yep, they're equal\n");
				memcpy(cur_inode, new_inode, BLOCK_SIZE); // set cur inode.
				found = 1; // only stop this loop.
			}
		}
		slicer = strtok(NULL, delim);
		// read inode, compare each link in inode with previous found.
		// when comparison is true, the new link has been found and slicer is updated to next dir.

		//slicer = NULL;
	}
//	printf("Found inode!: ");
//	printf("%s found with address &d\n",cur_inode->filename,&cur_inode);
	free(new_inode);
	return 0;
}

int path_to_parent(const char *path, struct lfs_inode *cur_inode, struct lfs_inode *this_inode){
	char delim[1];
	char *slicer;
	int count, i, found;
	struct lfs_inode *new_inode;

	// verify that path is somewhat valid.
	if(path[0] != '/'){
		perror("first char not '/'");
		return -1;
	}

	// if we're looking for root inode, return once root has been read.
	//printf("are we looking for root?\n");
	if (path[1] == '\0') {
		read_inode(cur_inode, 0);
		//printf("we were, returning\n");
		return 0;
	}
	//printf("nahh\n");

	delim[0] = '/';
	slicer = strtok(path, delim);
	//slicer = strtok(NULL, delim);

	// read root inode first.
	count = read_inode(cur_inode, 0);
	if (count == -1){
		perror("read failed in path_to_inode");
		return -1;
	}

	// malloc before iterating.
	new_inode = malloc(BLOCK_SIZE);
	memset(new_inode,0,BLOCK_SIZE); // clean memory block so only disk info is kept.

	// /dm510/project4
	// iterate over each directory name seperated by /
	// REMEMBER TO ADD SUPPORT FOR INDIRECT FILES / DIRS.
	while(slicer != NULL){
		found = 0;
		for (i=0; i<15 && found != 1; i++){
			//printf("about to check a data_blocks index %d\n",i);
			// if we reach an index with 0, that means the dir can't be found.
			if (cur_inode->data_blocks[i] == 0){
				printf("no blocks found at %s i=%d\n", cur_inode->filename,i);
				return -1; // we weren't able to reach path.
			}
			//printf("About to read at block %d\n",cur_inode->data_blocks[i]);
			// read referenced inode and check filename and mode (must be dir)
			count = read_inode(new_inode,cur_inode->data_blocks[i]);
			if (count == -1){
				printf("read failed in path_to_inode loop");
				return -1;
			}
			//printf("reading node with UID %d\n",new_inode->uid);
			//printf("Read %d bytes from %s node found in %s\n",count,new_inode->filename, cur_inode->filename);
			//printf("is that equal to %s?\n",slicer); // no root /
			if (strcmp(new_inode->filename,slicer) == 0){ // dir found.
				//printf("yep, they're equal\n");
				memcpy(cur_inode, new_inode, BLOCK_SIZE); // set cur inode.
				found = 1; // only stop this loop.
			}
		}
		printf("cur_inode: %s, this_inode %s\n", cur_inode->filename, this_inode->filename);

		for (i=0; i<15; i++){
			if (cur_inode->data_blocks[i] == this_inode->uid -1){
				printf("cur_inode: %s, this_inode %s\n", cur_inode->filename, this_inode->filename);
				free(new_inode);
				return 0;
			}
		}
		slicer = strtok(NULL, delim);
		// read inode, compare each link in inode with previous found.
		// when comparison is true, the new link has been found and slicer is updated to next dir.

		//slicer = NULL;
	}
//	printf("Found inode!: ");
//	printf("%s found with address &d\n",cur_inode->filename,&cur_inode);
	free(new_inode);

	return 0;
}

const char* path_to_folder(const char *path){
	int last_slash;
	char *folder;
	printf("path before find_slash %s\n",path);
	last_slash = find_slash(path, 0);
	folder = malloc(strlen(path)-last_slash-1);
	strcpy(folder,path+last_slash+1);
	printf("path %s, folder %s, slash %d\n",path,folder, last_slash);
	printf("Folder name: %s\n", folder);
	return folder;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;
	struct lfs_inode *inode;
	const char *filename;
	char *filepath;
	printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));

	// no heavy lifting for root dir which also gives weird behavior.
	if( strcmp( path, "/" ) == 0 ||
		strcmp( path, "." ) == 0 ||
		strcmp( path, ".." ) == 0 ){
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	inode = malloc(BLOCK_SIZE);
	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	res = path_to_inode(filepath,inode);
	free(filepath);
	printf("getattr path_to_inode res=%d\n",res);
	if (res == -1){
		return -ENOENT;
	}
	// validate that the found inode is the one the path points to (does it exist?)

	printf("getting intended filename\n");
	filepath = malloc(strlen(path));
	strcpy(filepath,path);
	printf("fp after copy %s\n",filepath);
	filename = path_to_folder(filepath);
	printf("comparing to found inode\n");
	printf("comparing path to %s\n",filename);
	printf("with inode found %s\n",inode->filename);
	if (filename != NULL && strcmp(filename,inode->filename) == 0){
		//printf("found correct inode\n");
		printf("found inode for %s\n", inode->filename);
	} else {
		printf("no file at path, doesn't exist\n");
		free(filepath);
		return -ENOENT;
	}
	if (0==0){
		stbuf->st_mode=inode->mode;
		if (S_ISREG(inode->mode)){
				stbuf->st_nlink=1;
				stbuf->st_size=inode->size;
		} else if (S_ISDIR(inode->mode)){
				stbuf->st_nlink=2;
		} else {
			printf("some other type lmao %d\n",inode->mode);
			return -ENOENT;
		}
		//stbuf->st_size=inode->size;
		printf("found dir, returning 0\n");
		free(filepath);
		return 0;
	}

	if( strcmp( path, "/" ) == 0 ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}else if( strcmp( path, "/testdirectory" ) == 0 ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}else if( strcmp( path, "/hello" ) == 0 ) {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 12;
	}else{
		res = -ENOENT;
	}
	return res;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	int count, i;
	struct lfs_inode *cur_inode;
	struct lfs_inode *dir_inode;
	printf("readdir: (path=%s)\n", path);

	/*if(strcmp(path, "/") != 0){
		return -ENOENT;
	} */

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	cur_inode = malloc(BLOCK_SIZE);
	path_to_inode(path, cur_inode);
	printf("readdir called in %s node\n",cur_inode->filename);
	dir_inode = malloc(BLOCK_SIZE);
	// REMEMBER TO ADD SUPPORT FOR INDIRECT FILES / DIRS.
	for (i=0; i<15; i++){
		if (cur_inode->data_blocks[i] == 0){
			break;
		}
		count = read_inode(dir_inode, cur_inode->data_blocks[i]);
		// read referenced inode called filler() on filename.
		if (count == -1){
			perror("read failed in readdir loop");
			return -ENOENT;
		}
		printf("read %d, from %s at ref #%dgonna fill\n", count, cur_inode->filename, i);
		printf("filling with %s\n",dir_inode->filename);
		filler(buf,dir_inode->filename, NULL, 0);
		printf("filled\n");
	}
	//filler(buf, "hello", NULL, 0);
	//filler(buf, "testdirectory", NULL, 0);
	free(dir_inode);
	free(cur_inode);
	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {

  printf("open: (path=%s)\n", path);
	if (0 == 0){
		return 0;
	}
}

/* needs impl */
int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
	struct lfs_inode *cur_inode;
	char *filepath;
	int i, read, count;
	printf("read: (path=%s)\n", path);
	cur_inode = malloc(BLOCK_SIZE);
	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	path_to_inode(filepath, cur_inode);

	read = 0;
	for (i=0; i<15;i++){
		if (cur_inode->data_blocks[i] != 0){
				count = read_disk(cur_inode->data_blocks[i],buf + read, 0, MIN(BLOCK_SIZE,size - read));
				printf("read %d from block %d\n",count,cur_inode->data_blocks[i]);
				if (count == -1){
					return -errno;
				}
				read+=count;
		} else {
			// no more data.
			break;
		}
	}
	printf("Read: %d, buffer: %s\n",read,buf);
	return read;
}

/* DOES NOT ACCOUNT FOR OFFSET */
int lfs_write( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
	struct lfs_inode *cur_inode;
	char *filepath;
	int blocks, claimed, i, written, count;
	printf("write: (path=%s)\n", path);
	cur_inode = malloc(BLOCK_SIZE);
	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	path_to_inode(filepath, cur_inode);

	// FIND HIGHEST INDEX OF cur_inode->data_blocks AND SUBTRACT IT FROM BLOCKS, DONT CLAIM BLOCKS WE DON'T NEED.
	// TAKE OFFSET INTO ACCOUNT, IF WE ARE WRITING AT THE 3RD BLOCK, WE WILL STILL NEED 3 PREVIOUS.
	blocks = (size / BLOCK_SIZE) + 1;
	printf("requesting %d blocks for %s (%ld)\n",blocks,cur_inode->filename,size);
	claimed = claim_sequential_blocks(blocks);
	if (claimed == -1){
		free(cur_inode);
		free(filepath);
		printf("failed to claim blocks\n");
		return -errno;
	}
	// update file inode.
	for (i=0; i<blocks;i++){
		cur_inode->data_blocks[i] = claimed + i;
		printf("claimed %d for slot %d\n",claimed+i,i);
	}
	write_inode(cur_inode,cur_inode->uid - 1);

	written = 0;
	while (written < size){
			// write at reach block, write from reach index and write a block if possible, but only if an entire block is available.
			count = write_disk(claimed + floor(written/BLOCK_SIZE),buf+written,0,MIN(BLOCK_SIZE,size - written));
			if (count == -1){
				return -errno;
			}
			written+=count;
			printf("Wrote %d/%ld to file %s\n",written,size,cur_inode->filename);
	}
	return written;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	int res;
	printf("release: (path=%s)\n", path);
	res = close(disk);
	if (res == -1)
		return -errno;
	return 0;
}

int get_inode_slot(struct lfs_inode *inode){
	int i;
	for (i=0; i<15; i++){
		if (inode->data_blocks[i] == 0){
			return i;
		}
	}
	return -1;
}

int claim_sequential_blocks(int blocks){
	int count, i, found;
	struct lfs_inode *root_inode;
	char taken = 1;
	root_inode = malloc(BLOCK_SIZE);
	count = read_inode(root_inode, 0);
	if (count == -1){
		return -1;
	}
	found = 0;
	for (i=0; i<2500;i++){
		if (root_inode->extra_data[i] == 0){ // found free
			found++;
			if (found == blocks){
				memcpy(root_inode->extra_data + i + 1 - blocks,&taken,blocks);

				count = write_inode(root_inode, 0); // update on disk.
				if (count == -1){
					return -1;
				}
				free(root_inode);
				printf("Updated root. Block %d-%d taken, wrote %d bytes\n",i + 1 -blocks,i,count);
				return i;
			}
		} else {
			found = 0;
		}
	}
	return -1; // not found.
}

// find single block. Use get_contigous_blocks() for sequential free blocks.
int claim_free_block(){
	int count, i;
	struct lfs_inode *root_inode;
	char taken = 1;
	root_inode = malloc(BLOCK_SIZE);
	count = read_inode(root_inode, 0);
	if (count == -1){
		return -1;
	}
	//printf("Read root inode with name %s\n",root_inode->filename);
	//printf("sizeof %ld\n",sizeof(root_inode->extra_data));
	//printf("sizeof %ld, read %d\n",sizeof(root_inode),count);
	//printf("block 1 before claim: %d\n",root_inode->extra_data[1]);
	// first is taken by root, fix magic number
	for (i=0; i<2500;i++){
		if (root_inode->extra_data[i] == 0){ // found free
			// update inode, write to disk and return block.
			memcpy(root_inode->extra_data + i,&taken,1);
			//printf("block 1 after claim: %d\n",root_inode->extra_data[1]);

			count = write_inode(root_inode, 0); // update on disk.
			if (count == -1){
				return -1;
			}
			free(root_inode);
			printf("Updated root. Block %d taken, wrote %d bytes\n",i,count);
			return i;
		}
	}
	return -1; // not found.
}

int lfs_mknod(const char *path, mode_t mode, dev_t dev){
	printf("mknod path: %s\n", path);
	struct lfs_inode *cur_inode;
	struct lfs_inode *new_inode;
	const char *filename;
	char *filepath;
	int slot, block, cur_block, res;
	res = 0;
	cur_inode = malloc(BLOCK_SIZE);
	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	printf("mknod filepath %s, path %s\n",filepath,path);
	path_to_inode(filepath, cur_inode);
	cur_block = cur_inode->uid - 1;
	strcpy(filepath,path);
	printf("mknod filepath %s, path %s\n",filepath,path);
	filename = path_to_folder(filepath);

	printf("creating in directory: %s\n", cur_inode->filename);
	printf("name of new file: %s\n",filename);

	// find slot for new inode in data_blocks[15]
	// choose free block (how do we find this?)
	// create new inode and write it to disk at free block.
	// update list of free blocks.
	slot = get_inode_slot(cur_inode);
	if (slot == -1){
		res = -1;
	}
	block = claim_free_block();
	if (block == -1){
		res = -1;
	}
	if (res == -1){
		free(cur_inode);
		free(filepath);
		printf("Once of these failed: block: %d, slot: %d\n",block,slot);
		return res;
	}
	// update cur_inode after having claimed a block in root.
	read_inode(cur_inode,cur_block);
	printf("using slot %d and block %d\n", slot, block);

	new_inode = malloc(BLOCK_SIZE);
	new_inode->mode = mode;
	new_inode->size = BLOCK_SIZE;
	new_inode->uid = block + 1; // root inode at block 0 is #1
	//new_inode->filename = malloc(strlen(filename));
	strcpy(new_inode->filename,filename); // magic number
	memset(new_inode->data_blocks,0,sizeof(new_inode->data_blocks));
	new_inode->last_modified = time(NULL);
	new_inode->created = time(NULL);
	printf("writing new inode to disk at block %d\n",block);
	//write_disk(block, new_inode, 0, BLOCK_SIZE);
	write_inode(new_inode,block);

	// how do we know what block this is on?
	cur_inode->data_blocks[slot] = block;
	write_inode(cur_inode, cur_inode->uid - 1);
	printf("Wrote to disk, node with name %s references block %d with name %s\n",cur_inode->filename,block,new_inode->filename);
	// free inodes
	free(cur_inode);
	free(new_inode);
	free(filepath);

	return 0; // not implemented.
}

int lfs_mkdir(const char *path, mode_t mode){
	printf("mkdir path: %s\n", path);
	printf("!!!MKDIR: sizeof path: %ld,%ld\n", sizeof(path),strlen(path));
	struct lfs_inode *cur_inode;
	struct lfs_inode *new_inode;
	const char *filename;
	char *filepath;
	int slot, block, cur_block, res;
	res = 0;
	cur_inode = malloc(BLOCK_SIZE);
	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	printf("mkdir filepath %s, path %s\n",filepath,path);
	path_to_inode(filepath, cur_inode);
	cur_block = cur_inode->uid - 1;
	strcpy(filepath,path);
	printf("mkdir2 filepath %s, path %s\n",filepath,path);
	filename = path_to_folder(filepath);

	printf("creating in directory: %s\n", cur_inode->filename);
	printf("name of new dir: %s\n",filename);

	// find slot for new inode in data_blocks[15]
	// choose free block (how do we find this?)
	// create new inode and write it to disk at free block.
	// update list of free blocks.
	slot = get_inode_slot(cur_inode);
	if (slot == -1){
		res = -1;
	}
	block = claim_free_block();
	if (block == -1){
		res = -1;
	}
	if (res == -1){
		free(cur_inode);
		free(filepath);
		printf("Once of these failed: block: %d, slot: %d\n",block,slot);
		return res;
	}
	// update cur_inode after having claimed a block in root.
	read_inode(cur_inode,cur_block);
	printf("using slot %d and block %d\n", slot, block);

	new_inode = malloc(BLOCK_SIZE);
	new_inode->mode = mode | S_IFDIR;
	new_inode->size = BLOCK_SIZE;
	new_inode->uid = block + 1; // root inode at block 0 is #1
	//new_inode->filename = malloc(strlen(filename));
	strcpy(new_inode->filename,filename); // magic number
	memset(new_inode->data_blocks,0,sizeof(new_inode->data_blocks));
	new_inode->last_modified = time(NULL);
	new_inode->created = time(NULL);
	printf("writing new inode to disk at block %d\n",block);
	//write_disk(block, new_inode, 0, BLOCK_SIZE);
	write_inode(new_inode,block);

	// how do we know what block this is on?
	cur_inode->data_blocks[slot] = block;
	write_inode(cur_inode, cur_inode->uid - 1);
	printf("Wrote to disk, node with name %s references block %d with name %s\n",cur_inode->filename,block,new_inode->filename);
	// free inodes
	free(cur_inode);
	free(new_inode);
	free(filepath);

	return 0; // not implemented.
}

int free_block(int block){
	int count, i;
	struct lfs_inode *root_inode;
	char taken = 0;
	root_inode = malloc(BLOCK_SIZE);
	count = read_inode(root_inode, 0);
	if (count == -1){
		return -1;
	}
	//printf("Read root inode with name %s\n",root_inode->filename);
	//printf("sizeof %ld\n",sizeof(root_inode->extra_data));
	//printf("sizeof %ld, read %d\n",sizeof(root_inode),count);
	printf("block 1 before claim: %d\n",root_inode->extra_data[1]);
	// first is taken by root, fix magic number
	for (i=0; i<2500;i++){
		if (root_inode->extra_data[i] == block){ // found target block
			// update inode, write to disk and return block.
			memcpy(root_inode->extra_data + i + 1,&taken,1);
			printf("block 1 after claim: %d\n",root_inode->extra_data[1]);

			count = write_inode(root_inode, 0); // update on disk.
			if (count == -1){
				return -1;
			}
			free(root_inode);
			printf("Updated root. Block %d freed, freed %d bytes\n",i,count);
			return i;
		}
	}
	return -1; // not found.
}

int lfs_rmdir(const char *path){
	int i;
	char *filepath;
	struct lfs_inode *inode;
	struct lfs_inode *parent;
	printf("RMDIR CALLED\n");

	inode = malloc(BLOCK_SIZE);
	parent = malloc(BLOCK_SIZE);
	//path_to_inode(path,inode);

	filepath = malloc(strlen(path)); // don't want to change const.
	strcpy(filepath,path);
	printf("rmdir filepath %s, path %s\n",filepath,path);
	path_to_inode(filepath, inode);
	strcpy(filepath,path);

	path_to_parent(filepath,parent,inode);


	for (i=0; i < 15; i++){
		if (parent->data_blocks[i] == inode->uid - 1){
			parent->data_blocks[i] = 0;
			printf("We got here");
		}
	}
	printf("%s   /   %s\n",parent->filename, inode->filename);
	printf("We got here too \n");
	write_inode(parent, parent->uid -1);


	//frees the block the directory inode is on.
	free_block(inode->uid - 1);
	printf("After free_block\n");
	free(inode);
	free(parent);
	return 0;
}

int main( int argc, char *argv[] ) {
	disk_init();

	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
