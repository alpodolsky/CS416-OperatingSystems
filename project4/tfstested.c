/*
 *  Copyright (C) 2020 CS416 Rutgers CS
 *	Tiny File System
 *	File:	tfs.c
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];
struct superblock* sp;
bitmap_t inode_map;
bitmap_t dblock_map;
char bufr[BLOCK_SIZE];
int init =  -1;
int i;
// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	// Step 1: Read inode bitmap from disk
	bio_read(sp->i_start_blk, inode_map);
	//memcpy(inode_map,bufr,MAX_INUM/8);
	//memset(bufr,'\0',BLOCK_SIZE);

	// Step 2: Traverse inode bitmap to find an available slot
	for(i=0;i<MAX_INUM;i++){
		if(get_bitmap(inode_map,i)==0){
			//found available inode
			//Step 3: Update inode bitmap and write to disk 
			set_bitmap(inode_map,i);
			bio_write(sp->i_start_blk,inode_map);
			//free(inode_map);
			return i+sp->i_start_blk;
		}
	}
	//free(inode_map);
	printf("unable to find available inode\n");
	return -1;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {
	// Step 1: Read data block bitmap from disk
	//dblock_map = malloc(sizeof(MAX_DNUM/8));
	bio_read(sp->d_start_blk,dblock_map);
    printf("read block from disk\n");
	// Step 2: Traverse data block bitmap to find an available slot
	i = 0;
	for(i = 0; i < MAX_DNUM;i++){
		if(get_bitmap(dblock_map,i)==0){
			// Step 3: Update data block bitmap and write to disk 
			set_bitmap(dblock_map,i);
			bio_write(sp->d_start_blk,dblock_map);
			printf("got available block\n");
			//free(dblock_map);
			return i+sp->d_start_blk;
		}
	}
	//free(dblock_map);
	printf("failed to get available block\n");
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
	printf("in readi\n");
  // Step 1: Get the inode's on-disk block number
	unsigned int block_number = ino/(BLOCK_SIZE/sizeof(struct inode));
  // Step 2: Get offset of the inode in the inode on-disk block
	unsigned int inode_offset = ino%(BLOCK_SIZE/sizeof(struct inode));
  // Step 3: Read the block from disk and then copy into inode structure
	struct inode *local_inode = malloc(BLOCK_SIZE);
	//might need to change so that we bio_read into  char*buffer first
		//then into local_inode
	bio_read(block_number+sp->i_start_blk,local_inode);
	memcpy(inode, &local_inode[inode_offset],sizeof(struct inode));
	free(local_inode);
	printf("did things in readi\n");
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {
	// Step 1: Get the block number where this inode resides on disk
	//printf("%d blocksize/inode %d sizeof inode\n",BLOCK_SIZE/sizeof(struct inode),sizeof(struct inode));
	printf("in writei\n");
	unsigned int block_number = ino/(BLOCK_SIZE/sizeof(struct inode));
	
	// Step 2: Get the offset in the block where this inode resides on disk
	unsigned int inode_offset = ino%(BLOCK_SIZE/sizeof(struct inode));
	
	// Step 3: Write inode to disk 
	struct inode *local_inode = malloc(BLOCK_SIZE);
	bio_read(block_number,local_inode);
    memcpy(&local_inode[inode_offset], inode, sizeof(struct inode));
    bio_write(block_number+sp->i_start_blk, inode);
	free(local_inode);
	printf("left writei\n");
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {
  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
	printf("in dir find\n");
	struct inode* inode = (struct inode*)malloc(sizeof(struct inode));
	readi(ino, inode);
  // Step 2: Get data block of current directory from inode
	struct dirent* directEnt = (struct dirent*)malloc(sizeof(struct dirent));
	//int d_block =0;
	//int direct = 0;
	for(i = 0; i< 16; i++){
		if(inode->direct_ptr[i] != 0){
			bio_read(inode->direct_ptr[i], bufr);
			int j =0;
			// Step 3: Read directory's data block and check each directory entry.
			for(j = 0; j<16; j++){
				if(directEnt->valid == 1){
					//If the name matches, then copy directory entry to dirent structure
					if(strcmp(directEnt->name, fname) == 0){
						memcpy(dirent, &directEnt, sizeof(struct dirent));
						free(directEnt);
						printf("left dirfind inside\n");
						return 0;
					}
				}
			}
		}
	}
	printf("left dirfind\n");
	free(directEnt);
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	if(strcmp(path,"/")==0){
		readi(0, inode);
		return 0;
	}   
	char*tokens = strtok(strdup(path),"/");
    struct dirent d;

    while(tokens != NULL){
		//delete after testing
        //printf("%s\n",tokens);
        tokens = strtok(NULL,"/");
		int find = dir_find(ino, tokens, strlen(tokens), &d);
		if(find != 0){
			printf("Error in getnode\n");
			return -1;
		}
		ino = d.ino;
		//move onto next token
		tokens = strtok(NULL, "/");
    }
	// Note: You could either implement it in a iterative way or recursive way
	readi(ino, inode);
	printf("ok in getnode\n");
	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {
	printf("in tfs_mkfs\n");
	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	// write superblock information
	sp = malloc(sizeof(struct superblock));
	sp->magic_num=MAGIC_NUM;
	sp->max_inum=MAX_INUM;
	sp->max_dnum=MAX_DNUM;
	sp->i_bitmap_blk = 1;
	sp->d_bitmap_blk = 2;
	sp->i_start_blk = 3;	
	sp->d_start_blk = 4+(MAX_INUM*(sizeof(struct inode)));
	bio_write(0,sp);
	printf("wrote superblock\t");

	// initialize inode bitmap
	inode_map = malloc(MAX_INUM/8);
	memset(inode_map, 0, MAX_INUM/8);
	printf("setup inode stuff\t");

	// initialize data block bitmap
	dblock_map = malloc(sizeof(MAX_DNUM/8));
	memset(dblock_map, 0, MAX_DNUM/8);

	// update bitmap information for root directory
	set_bitmap(inode_map, 0);
	set_bitmap(dblock_map,0);
	bio_write(sp->i_bitmap_blk,inode_map);
	bio_write(sp->d_bitmap_blk,dblock_map);

	// update inode for root directory
	struct inode root_d;
    root_d.vstat.st_ino = 0;
    root_d.valid = 1;
    root_d.vstat.st_size = 0;
	root_d.type = S_IFDIR;
    root_d.vstat.st_mode = 0755;
    root_d.vstat.st_nlink = 2;
    root_d.vstat.st_blksize = BLOCK_SIZE;
    root_d.vstat.st_blocks = 0;
	root_d.vstat.st_gid = getgid();
	root_d.vstat.st_uid = getuid();
	for(i=0;i<16;i++){
		root_d.direct_ptr[i]=0;
	}
	root_d.direct_ptr[0]= sp->d_start_blk;
	writei(0,&root_d);
	printf("finished in mkfs\n");

	//free(inode_map);
	//free(dblock_map);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {
	// Step 1a: If disk file is not found, call mkfs
	printf("init\n\n");
	//this method confused me
		//no matter what value dev_open gave it wouldnt call mkfs
	if(init<0){
		printf("calling mkfs\n");
		init = 0;
		tfs_mkfs();
	}
  	// Step 1b: If disk file is found, just initialize in-memory data structures
  	// and read superblock from disk
	else{
		printf("in  else\n");
		sp = (struct superblock*)malloc(sizeof(struct superblock));
		unsigned char buf[BLOCK_SIZE];
		bio_read(0,buf);
		memcpy(sp,buf,sizeof(struct superblock));
		printf("did bioread");
	}
	printf("byeeeee init\n");
	return NULL;
}

static void tfs_destroy(void *userdata) {
	// Step 1: De-allocate in-memory data structures
	free(sp);
	free(inode_map);
	free(dblock_map);
	// Step 2: Close diskfile
	dev_close(diskfile_path);

}

static int tfs_getattr(const char *path, struct stat *stbuf) {
// Step 1: call get_node_by_path() to get inode from path
	struct inode* local_inode = (struct inode*)malloc(sizeof(struct inode));
	printf("before found\n");
	int found = get_node_by_path(path,0,local_inode);
	if (found == -1){
		printf("couldnt find node\n");
		free(local_inode);
		return -ENOENT;
	}
	printf("before big stoof\n");
	// Step 2: fill attribute of file into stbuf from inode
	
	stbuf->st_ino=local_inode->vstat.st_ino;
	stbuf->st_mode = local_inode->vstat.st_mode;
	stbuf->st_nlink=local_inode->vstat.st_mode;
	stbuf->st_uid = local_inode->vstat.st_uid;
	stbuf->st_gid = local_inode->vstat.st_gid;
	stbuf->st_size = local_inode->vstat.st_size;
	stbuf->st_atime = local_inode->vstat.st_atime;
	stbuf->st_mtime = local_inode->vstat.st_mtime;
	printf("passed the big stoof\n");

	free(local_inode);
	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk
	

	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}

