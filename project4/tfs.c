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

int i=0;
struct superblock *sp;
unsigned char buffer[BLOCK_SIZE];
bitmap_t inode_map;
bitmap_t dblock_map;
// Declare your in-memory data structures here

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	// Step 1: Read inode bitmap from disk
	bio_read(sp->i_start_blk, inode_map);
	//memcpy(inode_map,buffer,MAX_INUM/8);
	//memset(buffer,'\0',BLOCK_SIZE);

	// Step 2: Traverse inode bitmap to find an available slot
	for(i=0;i<MAX_INUM;i++){
		if(get_bitmap(inode_map,i)==0){
			//found available inode
			//Step 3: Update inode bitmap and write to disk 
			set_bitmap(inode_map,i);
			bio_write(sp->i_start_blk,inode_map);
			free(inode_map);
			return i+sp->i_start_blk;
		}
	}
	free(inode_map);
	printf("unable to find available inode\n");
	return -1;
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {
	// Step 1: Read data block bitmap from disk
	dblock_map = malloc(sizeof(MAX_DNUM/8));
	bio_read(sp->d_start_blk,dblock_map);

	// Step 2: Traverse data block bitmap to find an available slot
	i = 0;
	for(i = 0; i < MAX_DNUM;i++){
		if(get_bitmap(dblock_map,i)==0){
			// Step 3: Update data block bitmap and write to disk 
			set_bitmap(dblock_map,i);
			bio_write(sp->d_start_blk,dblock_map);
			printf("got available block\n");
			free(dblock_map);
			return i+sp->d_start_blk;
		}
	}
	free(dblock_map);
	printf("failed to get available block\n");
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
  // Step 1: Get the inode's on-disk block number
	unsigned int block_number = ino/(BLOCK_SIZE/sizeof(struct inode));
  // Step 2: Get offset of the inode in the inode on-disk block
	unsigned int inode_offset = ino%(BLOCK_SIZE/sizeof(struct inode));
  // Step 3: Read the block from disk and then copy into inode structure
	struct inode *local_inode = malloc(BLOCK_SIZE);
	//might need to change so that we bio_read into  char*buffer first
		//then into local_inode
	bio_read(block_number+sp->i_start_blk,local_inode);
	memcopy(inode, &local_inode[inode_offset],sizeof(struct inode));
	free(local_inode);
	printf("did things in readi\n");
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {
	// Step 1: Get the block number where this inode resides on disk
	printf("%d blocksize/inode %d sizeof inode\n",BLOCK_SIZE/sizeof(struct inode),sizeof(struct inode));
	unsigned int block_number = ino/(BLOCK_SIZE/sizeof(struct inode));
	
	// Step 2: Get the offset in the block where this inode resides on disk
	unsigned int inode_offset = ino%(BLOCK_SIZE/sizeof(struct inode));
	
	// Step 3: Write inode to disk 
	struct inode *local_inode = malloc(BLOCK_SIZE);
	bio_read(block_number,local_inode);
    memcopy(&local_inode[inode_offset], inode, sizeof(struct inode));
    bio_write(block_number+sp->i_start_blk, inode);
	free(local_inode);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {
  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode* inode = (struct inode*)malloc(sizeof(struct inode));
	readi(ino, inode);
  // Step 2: Get data block of current directory from inode
	struct dirent* directEnt = (struct dirent*)malloc(sizeof(struct dirent));
	int d_block =0;
	int direct = 0;
	for(i = 0; i< 16; i++){
		if(inode->direct_ptr[i] != 0){
			bio_read(inode->direct_ptr[i], buffer);
			int j =0;
			// Step 3: Read directory's data block and check each directory entry.
			for(j = 0; j<16; j++){
				if(directEnt->valid == 1){
					//If the name matches, then copy directory entry to dirent structure
					if(strcmp(directEnt->name, fname) == 0){
						memcpy(dirent, &directEnt, sizeof(struct dirent));
						free(directEnt);
						return 0;
					}
				}
			}
		}
	}
  
	free(directEnt);
	return -1;
	//maybe return -1
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	struct dirent* directEnt = malloc(sizeof(struct dirent));

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	if(dir_find(dir_inode.ino, fname, name_len, directEnt) != 0){
		free(directEnt);
		return -1;
		//failed
	}	
	// Step 2: Check if fname (directory name) is already used in other entries
	else {
		for(i = 0; i < 16; i ++){
			if(dir_inode.direct_ptr[i] != 0){
				bio_read(dir_inode.direct_ptr[i], directEnt);
				if(strcmp(fname, directEnt->name) == 0){
					free(directEnt);
					return 0;
					//or change to -1, check when testing
				}
			// Step 3: Add directory entry in dir_inode's data block and write to disk
			}
			else{
				// Allocate a new data block for this directory if it does not exist
				int nxtAvailBlk = get_avail_blkno();
				dir_inode.direct_ptr[i] = nxtAvailBlk; 
				directEnt->ino = f_ino;
				directEnt->valid = 1;
				struct dirent *newEnt = malloc(sizeof(struct dirent));
				strcpy(newEnt->name, fname);
				//Write directory entry
				bio_write(nxtAvailBlk, newEnt);
				free(newEnt);
				break;
			}
		}
	}
	// Update directory inode

	free(directEnt);
	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {
	struct dirent *directEnt = malloc(sizeof(struct dirent));
	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	int exists = 0;
	for(i = 0; i< 16; i++){
		if(dir_inode.direct_ptr[i] != 0){
			bio_read(dir_inode.direct_ptr[i], directEnt);
			// Step 2: Check if fname exist
			if(strcmp(fname, directEnt->name) == 0){
				//dir_inode.direct_ptr[i] = 0;
				//bio_write (sp->i_start_blk + dir_inode.ino, &dir_inode);
				exists = 1;
			}
			break;
		}
	}
	
	if(exists==0){
		printf("no file found\n");
		free(directEnt);
		return -1;
	}
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	dir_inode.direct_ptr[i] = 0;
	bio_write (sp->i_start_blk + dir_inode.ino, &dir_inode);
	free(directEnt);
	exists = 0;
	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
    char*tokens = strtok(path,"/");
    struct dirent d;
    while(tokens != NULL){
		//delete after testing
        printf("%s\n",tokens);
        tokens = strtok(NULL,"/");
		int find = dir_find(ino, tokens, strlen(tokens), &d);
		if(find != 0){
			//printf("Invalid Path\n");
			return -1;
		}
		ino = d.ino;
		//move onto next token
		tokens = strtok(NULL, "/");
    }
	// Note: You could either implement it in a iterative way or recursive way
	readi(ino, inode);
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
	bio_write(0,&sp);
	printf("wrote superblock\t");

	// initialize inode bitmap
	//this might need to be global
	//i also am not sure what the size  of it should be
	inode_map = malloc(sizeof(MAX_INUM/8));
	memset(inode_map, 0, MAX_INUM/8);
	sp->i_bitmap_blk = 1;
	sp->i_start_blk = 3;
	printf("setup inode stuff\t");

	// initialize data block bitmap
	dblock_map = malloc(sizeof(MAX_DNUM/8));
	memset(dblock_map, 0, MAX_DNUM/8);
	bio_write(2,dblock_map);
	printf("wrote datablock\t");

	sp->d_bitmap_blk = 2;
	free(dblock_map);
	//starts the datablock just after the inode blocks
	sp->d_start_blk = 3+(MAX_INUM/16);
	//unsure where the start would be right now

	// update bitmap information for root directory
	set_bitmap(inode_map, 0);
	bio_write(1,inode_map);
	free(inode_map);
	printf("wrote inode\t");

	// update inode for root directory
	struct inode* root_d = (struct inode*)malloc(MAX_INUM*sizeof(struct inode));
    root_d->ino = 0;
    root_d->vstat.st_ino = 0;
    root_d->valid = 1;
    root_d->size = sizeof(struct dirent);
    root_d->vstat.st_size = sizeof(struct dirent);
    root_d->type = S_IFDIR;
    root_d->vstat.st_mode = S_IFDIR;
    root_d->link = 0;
    root_d->vstat.st_nlink = 0;
    root_d->vstat.st_blksize = BLOCK_SIZE;
    root_d->vstat.st_blocks = 1;

	dev_open(diskfile_path);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {
	// Step 1a: If disk file is not found, call mkfs
	if(dev_open(diskfile_path)==-1){
		tfs_mkfs();
	}
    // Step 1b: If disk file is found, just initialize in-memory data structures
    // and read superblock from disk
	sp = (struct superblock*)malloc(sizeof(struct superblock));
	bio_read(0, &sp);
	//memcpy(sp, buffer, sizeof(struct superblock));
	//need to then  read the inote  and data bitmaps?
	//make sure to reset the buffer
	memset(buffer, BLOCK_SIZE, '\0');
	return NULL;
}

static void tfs_destroy(void *userdata) {
	// Step 1: De-allocate in-memory data structures

	free(sp);

	free(inode_map);

	free(dblock_map);

	// Step 2: Close diskfile
	dev_close();
}

static int tfs_getattr(const char *path, struct stat *stbuf) {
	// Step 1: call get_node_by_path() to get inode from path
	struct inode* local_inode = (struct inode*)malloc(sizeof(struct inode));
	int found = get_node_by_path(path,0,local_inode);
	if (found ==-1){
		return -ENOENT;
	}
	// Step 2: fill attribute of file into stbuf from inode
	stbuf = &local_inode->vstat;
	//idk if the time thing is right or stbuf needs more stuff
	//bit permissions
	//1 is exec, 2 is write, 4 is read, 5+ is a combo of the 3
	if(strcmp(path,"/")==0){
		//defines as a file where owner has all permisions group have exec and read, so does users
		stbuf->st_mode   = S_IFDIR | 0755;
		stbuf->st_nlink  = 2;
		time(&stbuf->st_mtime);
	}
	else{
		stbuf->st_mode   = S_IFREG | 0644;
		stbuf->st_nlink  = 1;
		stbuf->st_size = 1024;
		time(&stbuf->st_mtime);
	}
	free(local_inode);
	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {
	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* local_inode = (struct inode*)malloc(sizeof(struct inode));
	int found = get_node_by_path(path, 0, local_inode);
	// Step 2: If not found, return -1
	if(found==-1){
		free(local_inode);
		return -ENOENT;
	}
	else return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* local_inode = (struct inode*)malloc(sizeof(struct inode));
	if(get_node_by_path(path, 0, local_inode)==-1){
		free(local_inode);
		return -1;
	};
	// Step 2: Read directory entries from its data blocks, and copy them to filler
	struct dirent* dirEnt = malloc(BLOCK_SIZE);
	int j;
	//block
	for(i = 0; i < 16; i ++){
		if(local_inode->direct_ptr[i]!=0){
			bio_read(local_inode->direct_ptr[i], dirEnt);
			//directory
			for(j=0;j<16;j++){
				if(dirEnt[j].valid==1){
					//check if this works
					//the doc for fuse last link covers this
					filler(buffer,dirEnt[j].name,NULL,0);
				}
			}
		}
	}
	free(local_inode);
	free(dirEnt);
	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	//might need to change this to use strdup along with it
	char* dir = dirname(strdup(path));
	//this will grab the end part
	char* base = basename(strdup(path));

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode* local_inode = (struct inode*)malloc(sizeof(struct inode));
	get_node_by_path(dir, 0, local_inode);

	// Step 3: Call get_avail_ino() to get an available inode number
	local_inode->ino = get_avail_ino();

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	dir_add(local_inode,local_inode->ino,base,strlen(base));
	
	// Step 5: Update inode for target directory
	local_inode->valid = 1;
	local_inode->size = BLOCK_SIZE;
	local_inode->type = 0;
	local_inode->link = 2;
	memset(local_inode->direct_ptr, 0, sizeof(int)*16);

	// Step 6: Call writei() to write inode to disk
	writei(local_inode->ino,&local_inode);

	free(dir);
	free(base);
	free(local_inode);
	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* dir = dirname(strdup(path));
	//this will grab the end part
	char* base = basename(strdup(path));
	// Step 2: Call get_node_by_path() to get inode of target directory
	struct inode* local_inode = malloc(sizeof(struct inode));
	
	local_inode->ino = get_node_by_path(base,0,local_inode);
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
	char* dir = dirname(path);
	char* base = basename(path);
	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {
	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* local_inode = malloc(sizeof(struct inode));
	int found = get_node_by_path(path, 0, local_inode);
	// Step 2: If not find, return -1
	if(found==-1){
		free(local_inode);
		return -1;
	}
	else return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode* local_inode = malloc(sizeof(struct inode));
	int found = get_node_by_path(path, 0, local_inode);
	if(found==-1){
		free(local_inode);
		return -1;
	}
	// Step 2: Based on size and offset, read its data blocks from disk
	int currBlock = offset/BLOCK_SIZE;
	int blockOffset = offset%BLOCK_SIZE;
	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	struct inode* local_inode = malloc(sizeof(struct inode));
	int found = get_node_by_path(path, 0, local_inode);
	if(found==-1){
		free(local_inode);
		return -1;
	}
	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* dir = dirname(path);
	char* base = basename(path);
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

	fuse_stat = fuse_main(argc, argv, &tfs_open, NULL);

	return fuse_stat;
}

