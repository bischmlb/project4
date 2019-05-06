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
	char *filename;
	time_t last_modified;
	time_t created;
	int *data[15];
	int *dp;
}lfs_inode;

struct root_data {
	char blocks[2500];
}root_data;

#define MAX_DIRECTORIES 5
#define MAX_DIRECTORY_SIZE 24
#define BLOCK_SIZE 4096

static char *dirs[MAX_DIRECTORIES][MAX_DIRECTORY_SIZE];
static int mkdirID = 0;
static int disk = -1;
static char *diskPath = "disk";

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
	root_inode = malloc(sizeof(lfs_inode));
	printf("size of inode: %d\n",sizeof(lfs_inode));
	read_disk(0, root_inode, 0, sizeof(lfs_inode));
	printf("inode uid: %d\n", root_inode->uid);
	if(root_inode->uid != 1){
		root_inode->mode = S_IFDIR | 0755;
		root_inode->size = BLOCK_SIZE;
		root_inode->uid = 1;
		root_inode->filename = "/";
		root_inode->last_modified = time(NULL);
		root_inode->created = time(NULL);
		root_inode->data[15] = malloc(sizeof(int)*15);
		root_inode->dp = 0;
		write_disk(0, root_inode, 0, BLOCK_SIZE);
	}


}

int open_disk(){
	disk = open(diskPath, O_RDWR);
	if (disk == -1){
		printf("failed to open disk\n");
		return -errno;
	}
	printf("disk opened\n");
	return 0;
}

int close_disk(){
	int res;
	res = close(disk);
	if (res == -1){
		printf("failed to close disk\n");
		return -errno;
	}
	printf("disk closed\n");
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
	loffset = lseek(disk, block * BLOCK_SIZE + offset, SEEK_SET);
	//printf("disk: %d, offset: %d,%d\n",disk,loffset,offset);
	if (loffset == -1){
		close_disk();
		return -errno;
	}
	count = read(disk, buff, size);
	if (count == -1){
		//printf("failed to read at %d + %d\n",loffset,offset);
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
	loffset = lseek(disk, block * BLOCK_SIZE + offset, SEEK_SET);
	printf("disk: %d, offset: %d,%d\n",disk,loffset,offset);
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

lfs_inode path_to_inode(const char *path){
	char delim[];
	char *slicer;
	int count, slash_index;

	delim = "/";
	slicer = strtok(path, delim);
	struct lfs_inode new_inode;
	if(path[0] != '/'){
		perror("first char '/'");
		return -EINVAL;
	}
	newInode = malloc(sizeof(lfs_inode));
	count = read_disk(0,buf, offset, size);
	if (count == -1){
		return -errno;
	}
	while(slicer != NULL){
		// read inode, compare each link in inode with previous found.
		// when comparison is true, the new link has been found and slicer is updated to next dir.

	}

}


int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;
	printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));


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
	}else if( strcmp ( path, dirs[mkdirID] ) == 0) {
		//printf("kdkdk: %s \n", dirs[mkdirID]);
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}else{
		res = -ENOENT;
	}
	return res;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	if(strcmp(path, "/") != 0){
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "hello", NULL, 0);
	filler(buf, "testdirectory", NULL, 0);
/*	for (int i = 0; i < MAX_DIRECTORIES; i++){
		filler(buf, dirs[i], NULL, 0);
		printf("directories:%s \n", dirs[i]);
	}
*/
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
  printf("read: (path=%s)\n", path);
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

int lfs_mkdir(const char *path, mode_t mode){
	int res;
	printf("THIS: %s \n", path);
	if(mkdirID == MAX_DIRECTORIES){
		return -errno;
	}
	res = mkdir(path, mode);
	strcpy(dirs[mkdirID], path);
	mkdirID++;
	//for (int i = 0; i < MAX_DIRECTORIES; i++){
	//	printf("directories: %s \n", dirs[i]);
//	}

	if(res == -1){
		return -errno;
		}

	return 0;
}



int main( int argc, char *argv[] ) {
	disk_init();

	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
