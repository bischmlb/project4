#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_write( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_mkdir(const char *path, mode_t mode);
int lfs_mknod(const char *path, mode_t mode, dev_t dev);
int lfs_truncate(const char *path, off_t size, struct fuse_file_info *fi);

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = NULL,
	.rmdir = NULL,
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
	char *filename[32];
	time_t last_modified;
	time_t created;
	int *data_blocks[15];
	int *dp;
	char *extra_data[313]; // used by root inode to track free blocks.
} lfs_inode;

/*
struct root_data {
	char blocks[2500];
} root_data;
*/

#define BLOCK_SIZE 4096

static int disk = -1;
static char *diskPath = "disk";

int write_inode(lfs_inode inode, int block){
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
	printf("disk write: %d, block: %d, offset: %d,%d\n",disk,block, loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	// only allow writing within 1 block.
	count = write(disk, &(inode->mode), sizeof(inode->mode));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, &(inode->size), sizeof(inode->size));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, &(inode->uid), sizeof(inode->uid));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, inode->filename, sizeof(inode->filename));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, &(inode->last_modified), sizeof(inode->last_modified));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, &(inode->created), sizeof(inode->created));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, inode->data_blocks, sizeof(inode->data_blocks));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, inode->dp, sizeof(inode->dp));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	count = write(disk, inode->extra_data, sizeof(inode->extra_data));
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}

	close_disk();
	return count;
}

int lfs_mknod(const char *path, mode_t mode, dev_t dev){
	printf("mknod called\n");


	return 0;
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
	root_inode = malloc(BLOCK_SIZE);
	printf("size of inode: %d\n",BLOCK_SIZE);
	read_disk(0, root_inode, 0, BLOCK_SIZE);
	printf("inode uid: %d\n", root_inode->uid);
	if(root_inode->uid != 1){
		root_inode->mode = S_IFDIR | 0755;
		root_inode->size = BLOCK_SIZE;
		root_inode->uid = 1;
		memset(root_inode->filename,0,sizeof(char)*32);
		root_inode->filename[0] = '/';
		root_inode->filename[1] = '\0';
		root_inode->last_modified = time(NULL);
		root_inode->created = time(NULL);
		memset(root_inode->data_blocks,0,sizeof(int)*15);
		//root_inode->dp = 0;
		//root_inode->extra_data = {0};
		root_inode->extra_data[0] = 1;
		write_disk(0, root_inode, 0, BLOCK_SIZE);
	}
	free(root_inode);

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
	printf("disk read: %d, block: %d, offset: %d,%d\n",disk,block, loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	memset(buff,0,size); // clean area first.
	count = read(disk, buff, size);
	if (count == -1){
		printf("failed to read at %d + %d\n",loffset,offset);
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
	printf("disk write: %d, block: %d, offset: %d,%d\n",disk,block, loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	// only allow writing within 1 block.
	count = write(disk, buff, size);
	if (count == -1){
		printf("failed to write at %d + %d\n",loffset,offset);
		close_disk();
		return -errno;
	}
	close_disk();
	return count;
}

int find_slash(const char *string, size_t offset){
	int i;
	for (i = offset; i < strlen(string); i++ ){
		if (string[i] == '/'){
			return i;
		}
	}
	return -1;
}

int path_to_inode(const char *path, struct lfs_inode *cur_inode){
	char delim[1];
	char *slicer;
	int count, slash_index, i, found;
	struct lfs_inode *new_inode;

	// verify that path is somewhat valid.
	if(path[0] != '/'){
		perror("first char not '/'");
		return -1;
	}

	// if we're looking for root inode, return once root has been read.
	//printf("are we looking for root?\n");
	if (path[1] == '\0') {
		read_disk(0,cur_inode, 0, BLOCK_SIZE);
		//printf("we were, returning\n");
		return 0;
	}
	//printf("nahh\n");

	delim[0] = "/";
	slicer = strtok(path, delim);
	//slicer = strtok(NULL, delim);

	// read root inode first.
	count = read_disk(0,cur_inode, 0, BLOCK_SIZE);
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
			printf("about to check a data_blocks index %d\n",i);
			// if we reach an index with 0, that means the dir can't be found.
			if (cur_inode->data_blocks[i] == 0){
				printf("no blocks found at %s i=%d\n", cur_inode->filename,i);
				return -1; // we weren't able to reach path.
			}
			printf("About to read at block %d\n",cur_inode->data_blocks[i]);
			// read referenced inode and check filename and mode (must be dir)
			count = read_disk(cur_inode->data_blocks[i],new_inode, 0, BLOCK_SIZE);
			if (count == -1){
				printf("read failed in path_to_inode loop");
				return -1;
			}
			printf("reading node with UID %d\n",new_inode->uid);
			printf("Read %d bytes from %s node found in %s\n",count,new_inode->filename, cur_inode->filename);
			printf("is that equal to %s?\n",slicer+1); // no root /
			if (strcmp(new_inode->filename,slicer+1) == 0){ // dir found.
				printf("yep, they're equal\n");
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

const char* path_to_folder(const char *path)
{
	char delim[] = "/";

	char *ptr = strtok(path, delim);

	// is there a better way than a double call to strtok?
	while(ptr != NULL)
	{
		if (strtok(NULL, delim) == NULL){
			return ptr;
		}
		ptr = strtok(NULL, delim);
	}

	return NULL;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;
	struct lfs_inode *inode;
	const char *filename;
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
	res = path_to_inode(path,inode);
	printf("getattr path_to_inode res=%d\n",res);
	// validate that the found inode is the one the path points to (does it exist?)
	printf("getting intended filename\n");
	filename = path_to_folder(path);
	printf("comparing to found inode\n");
	printf("comparing path to %s\n",filename);
	printf("with inode found %s\n",inode->filename);
	if (filename != NULL && strcmp(filename,inode->filename) == 0){
		//printf("found correct inode\n");
		printf("found inode for %s\n", inode->filename);
	} else {
		printf("no file at path, doesn't exist\n");
		return -ENOENT;
	}
	if (0==0){
		stbuf->st_mode=S_IFDIR | 0755;//inode->mode;
		stbuf->st_nlink=2;
		//stbuf->st_size=inode->size;
		printf("found dir, returning 0\n");
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
	struct lfs_inode *read_inode;
	printf("readdir: (path=%s)\n", path);

	if(strcmp(path, "/") != 0){
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	cur_inode = malloc(BLOCK_SIZE);
	path_to_inode(path, cur_inode);

	read_inode = malloc(BLOCK_SIZE);
	// REMEMBER TO ADD SUPPORT FOR INDIRECT FILES / DIRS.
	for (i=0; i<15; i++){
		if (cur_inode->data_blocks[i] == 0){
			break;
		}
		// read referenced inode called filler() on filename.
		count = read_disk(cur_inode->data_blocks[i],read_inode, 0, BLOCK_SIZE);
		if (count == -1){
			perror("read failed in readdir loop");
			return -ENOENT;
		}
		printf("read %d, from %s gonna fill\n", count, cur_inode->filename);
		printf("filling with %s\n",read_inode->filename);
		filler(buf,read_inode->filename, NULL, 0);
		printf("filled\n");
	}
	//filler(buf, "hello", NULL, 0);
	//filler(buf, "testdirectory", NULL, 0);
	free(read_inode);
	free(cur_inode);
	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
	int res;

  printf("open: (path=%s)\n", path);
	if (0 == 0){
		return 0;
	}
}

/* needs impl */
int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
  printf("read: (path=%s)\n", path);
	int count = read_disk(0,buf, offset, size);
	if (count == -1){
		return -errno;
	}
	return count;
}

/* needs impl */
int lfs_write( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
  printf("write: (path=%s)\n", path);
	int count = write_disk(0,buf, offset, size);
	if (count == -1){
		return -errno;
	}
	return count;
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

// find single block. Use get_contigous_blocks() for sequential free blocks.
int claim_free_block(){
	int count, i;
	struct lfs_inode *root_inode;

	root_inode = malloc(BLOCK_SIZE);
	count = read_disk(0, root_inode, 0, BLOCK_SIZE);
	if (count == -1){
		return -1;
	}

	// first is taken by root, fix magic number
	for (i=5; i<2500;i++){
		if (root_inode->extra_data[i] == 0){ // found free
			// update inode, write to disk and return block.
			root_inode->extra_data[i] = 1;
			count = write_disk(0, root_inode, 0, BLOCK_SIZE); // update on disk.
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

int lfs_mkdir(const char *path, mode_t mode){
	printf("mkdir path: %s\n", path);
	struct lfs_inode *cur_inode;
	struct lfs_inode *new_inode;
	const char *filename;
	int res, slot, block;
	cur_inode = malloc(BLOCK_SIZE);
	path_to_inode(path, cur_inode);
	filename = path_to_folder(path);
	printf("creating in directory: %s\n", cur_inode->filename);
	printf("name of new dir: %s\n",filename);

	// find slot for new inode in data_blocks[15]
	// choose free block (how do we find this?)
	// create new inode and write it to disk at free block.
	// update list of free blocks.
	slot = get_inode_slot(cur_inode);
	if (slot == -1){
		return -1;
	}
	block = claim_free_block();
	if (block == -1){
		return -1;
	}
	printf("using slot %d and block %d\n", slot, block);

	new_inode = malloc(BLOCK_SIZE);
	new_inode->mode = mode;
	new_inode->size = BLOCK_SIZE;
	new_inode->uid = block + 1; // root inode at block 0 is #1
	memset(new_inode->filename,0,sizeof(char)*32);
	memcpy(new_inode->filename,filename,32); // magic number
	new_inode->last_modified = time(NULL);
	new_inode->created = time(NULL);
	printf("writing new inode to disk at block %d\n",block);
	printf("writing s %s and d %d\n",new_inode->filename,&new_inode->filename);
	write_disk(block, new_inode, 0, BLOCK_SIZE);

	// how do we know what block this is on?
	cur_inode->data_blocks[slot] = block;
	write_disk(cur_inode->uid - 1, cur_inode, 0, BLOCK_SIZE);
	printf("Wrote to disk, node with name %s references block %d with name %s\n",cur_inode->filename,block,new_inode->filename);
	// free inodes
	free(cur_inode);
	free(new_inode);

	return 0; // not implemented.
}



int main( int argc, char *argv[] ) {
	disk_init();

	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
