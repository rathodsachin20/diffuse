//Implements system calls for the file system

#include "layer2.h"

/* Gives block number stored at 'index' in block list at indirect_block_no
*  Allocates new block is 'allocate' is true and block did not exist.
*/
block_num get_block_in_list(block_num indirect_block_no, int index, bool allocate, FILE* fp){
    struct block_list indirect_bl;
    read_block_list(&indirect_bl, indirect_block_no, fp);
    block_num block_no = indirect_bl.list[index];
    if(block_no == 0 && allocate){
        block_no = allocate_block(fp);
        if(!block_no) return 0;
        indirect_bl.list[index] = block_no;
        write_block_list(&indirect_bl, indirect_block_no, fp);
    }
    return block_no;
}

/* Returns a block number on disk for the given offset in a file, by travesing
*  direct and indirect blocks. 
*  If no block exists at given offset and if allocate is true, it will allocate
*  a new block. Otherwise, it returns 0.
*/
block_num get_file_block_num(int block_index, block_num inode_num, bool allocate, FILE* fp){
    //return 6;
    struct inode node;
    read_inode(fp, inode_num, &node);
    //block_num block_index = offset / BLOCK_SIZE;
    block_num block_no = 0;
    int i;
    int n = BLOCK_SIZE / sizeof(block_num); // Number of entries per blocks
    if(block_index < INODE_NUM_DIRECT_BLOCKS){
        block_no = node.direct_blocks[block_index];
        if(block_no == 0){
            if( allocate ){
                block_no = allocate_block(fp);
                if(block_no == 0){
                    // No free blocks available
                    return 0;
                }
                node.direct_blocks[block_index] = block_no;
                write_inode(fp, inode_num, &node);
            }
            else { // No new allocation asked
               printf("Block not allocated or asked to. in direct get_file_block_num");
               return 0;
            }
        }
    }
    else if(block_index < (INODE_NUM_DIRECT_BLOCKS + n)){ // Single indirect
        int index = block_index -  INODE_NUM_DIRECT_BLOCKS;
        block_num indirect_block_no = node.single_indirect_block;
        if(indirect_block_no != 0){
            block_no = get_block_in_list(indirect_block_no, index, allocate, fp);
        }
        else { // indirect_block_no == 0
            if(allocate) {
                indirect_block_no = allocate_block_list(fp);
                if(!indirect_block_no) return 0;
                block_no = get_block_in_list(indirect_block_no, index, allocate, fp);

                node.single_indirect_block = indirect_block_no;
                write_inode(fp, inode_num, &node);

            }
            else{
                return 0;
            }
        }
    }
    else if(block_index < (INODE_NUM_DIRECT_BLOCKS + n + n*n)){ // Double Indirect :O
        
        int dd_index = block_index - INODE_NUM_DIRECT_BLOCKS - n;
        int d_index = dd_index / n;
        int s_index = dd_index % n;
        struct block_list d_indirect_bl;
        block_num d_indirect_block_no = node.double_indirect_block;
        if(!d_indirect_block_no){ // No Double indirect block
            if(allocate){
                d_indirect_block_no = allocate_block_list(fp);
                if(!d_indirect_block_no) return 0;
                node.double_indirect_block = d_indirect_block_no;
                write_inode(fp, inode_num, &node);
            }
            else{
                return 0;
            }
        }

        read_block(&d_indirect_bl, d_indirect_block_no, 0, sizeof(struct block_list), fp);
        block_num s_indirect_block_no = d_indirect_bl.list[d_index];
        if(!s_indirect_block_no){ // No Single indirect block
            if(allocate){
                s_indirect_block_no = allocate_block_list(fp);
                if(!s_indirect_block_no) return 0;
                d_indirect_bl.list[d_index] = s_indirect_block_no;
                write_block_list(&d_indirect_bl, d_indirect_block_no, fp);
            }
            else{
                return 0;
            }
        }
        block_no = get_block_in_list(s_indirect_block_no, s_index, allocate, fp);

    }
    else if(block_index < (INODE_NUM_DIRECT_BLOCKS + n + n*n+ n*n*n)){ // Triple Indirect :O :O
        int tt_index = block_index - INODE_NUM_DIRECT_BLOCKS - n - n*n;
        int t_index = tt_index / n*n;
        int d_index = (tt_index % (n*n)) / n;
        int s_index = (tt_index % (n*n)) % n;
        struct block_list t_indirect_bl;
        struct block_list d_indirect_bl;
        block_num t_indirect_block_no = node.triple_indirect_block;
        if(!t_indirect_block_no){ // No Double indirect block
            if(allocate){
                t_indirect_block_no = allocate_block_list(fp);
                if(!t_indirect_block_no) return 0;
                node.triple_indirect_block = t_indirect_block_no;
                write_inode(fp, inode_num, &node);
            }
            else{
                return 0;
            }
        }
        read_block(&t_indirect_bl, t_indirect_block_no, 0, sizeof(struct block_list), fp);
        block_num d_indirect_block_no = t_indirect_bl.list[t_index];
        if(!d_indirect_block_no){ // No Double indirect block
            if(allocate){
                d_indirect_block_no = allocate_block_list(fp);
                if(!d_indirect_block_no) return 0;
                t_indirect_bl.list[t_index] = d_indirect_block_no;
                write_block_list(&t_indirect_bl, t_indirect_block_no, fp);
            }
            else{
                return 0;
            }
        }
        read_block(&d_indirect_bl, d_indirect_block_no, 0, sizeof(struct block_list), fp);
        block_num s_indirect_block_no = d_indirect_bl.list[d_index];
        if(!s_indirect_block_no){ // No Single indirect block
            if(allocate){
                s_indirect_block_no = allocate_block_list(fp);
                if(!s_indirect_block_no) return 0;
                d_indirect_bl.list[d_index] = s_indirect_block_no;
                write_block_list(&d_indirect_bl, d_indirect_block_no, fp);
            }
            else{
                return 0;
            }
        }
        block_no = get_block_in_list(s_indirect_block_no, s_index, allocate, fp);

    }
    else{
        printf("File Max Size Limit Reached!!!");
        block_no = 0;
    }
    printf("get_file_block_no returning:%ld", block_no);
    return block_no;
}


//int add_inode_entry(const char* filepath, block_num file_inum, FILE* fp){
void add_inode_entry(const char* filepath, block_num file_inum, FILE* fp){
    printf("\tin add_inode_entry\n");
    printf("size of struct dir:%ld\n", sizeof(struct directory));
    block_num parent_inum = get_parent_inode_num(filepath, fp);
    struct directory dir;
    struct inode parent_node;
    printf("Here-10\t");
    read_inode(fp, parent_inum, &parent_node);
    //TODO: iterate to get last unfilled block and add entry there
    block_num block_no = parent_node.direct_blocks[0];
    parent_node.last_filled_block_index = 0;
    //block_num block_no = 1 + NUM_INODE_BLOCKS;
    printf("Here-8\t");
    if(block_no == 0){
        printf("Here-7\t");
        block_no = allocate_block(fp);
        parent_node.direct_blocks[0] = block_no;
        write_inode(fp, parent_inum, &parent_node);
    }
    
    
    printf("Here0\t");
    read_block(&dir, block_no, 0, sizeof(struct directory), fp);
    dir.inode_num[0] = file_inum;
    printf("Here4\t");
    dir.name[0][MAX_FILE_NAME_LEN-1] = '\0';
    strcpy(dir.name[0], &filepath[0]);
    printf("Here5\t");
    write_block(&dir, block_no, 0, sizeof(struct directory), fp);
    struct directory dir2;
    read_block(&dir2, block_no, 0, sizeof(struct directory), fp);
    printf("DIR DISK CONTENTS:%ld, %s\n", dir2.inode_num[0], dir2.name[0]);
    printf("INODE ENTRY ADDED TO PARENT.\n");
    //return 0;
}


block_num get_parent_inode_num(const char* filepath, FILE* fp){
    return 1L;
}


block_num fs_namei(FILE* fp, const char* filep){
    char* fname;
    struct inode working_inode;
    char* filepath;
    int len = strlen(filep);
    block_num inode_no = 1;
    
    filepath = (char*) malloc(sizeof(char) * (len+1));
    strcpy(filepath, filep);
    fname = strtok(filepath,"/");
    
    if(strncmp(&filepath[0],"/",1)==0){
        printf("in fs_namei: path starts with root.\n");
        while(fname != NULL){
            printf("in directory: %s\n", fname);            
            read_inode(fp, inode_no, &working_inode);
            struct directory dr;
            //loop through the direct blocks to find name
            int n = 0;
            int found=0;
            while(n <= working_inode.last_filled_block_index && !found){
                //block_num block_no = working_inode.direct_blocks[n];
                block_num block_no = get_file_block_num(n, inode_no, false, fp);
                if(block_no == 0){
                    continue;
                    //printf("NO SUCH FILE!\n");
                    //return 0;
                }
                read_block(&dr, block_no, 0, sizeof(struct directory), fp);
                int i;
                for(i=0; i<BLOCK_SIZE/NAMEI_ENTRY_SIZE; i++){
                    printf("dir file:%s\n", dr.name[i]);
                    if( strcmp(fname,dr.name[i])==0 ){
                        inode_no = dr.inode_num[i];
                        found=1;
                        printf("PATH FOUND!!!\n");
                        break;
                    }
                }
                n++;
            }
            fname = strtok(NULL,"/");
        }
    }
    //printf("In fs_namei: %s: path not found\n", filepath);
    return inode_no;
}

int fs_create(const char *filepath, mode_t mode, struct fuse_file_info * ffi, FILE* fp){
    if(filepath == NULL)
        return -1; 
    //using namei find the directory and the data block to write into
    //namei(fp,filepath);

    int len = strlen(filepath);
    
    char* fname = (char*) malloc(sizeof(char) * (len+1));
    strcpy(fname, filepath);
    char* pch = strrchr(filepath, '/');
    strncpy(fname, &filepath[pch - filepath + 1], len);


    struct inode node;
    block_num inode_num = allocate_inode(fp, &node);
    printf("in fs_create: got inode no. %ld", inode_num);
    read_inode(fp, inode_num, &node);
    node.owner_id = 121 ;
    node.group_id = 1;
    node.type = 1;
    //node.file_modified = time(0);
    node.direct_blocks[0] = 0; //TODO: this should not be needed
    //node.direct_blocks[0] = allocate_block(fp);
    write_inode(fp, inode_num, &node);
    
    // Make entry in parent dir
    add_inode_entry(fname, inode_num, fp);
    printf("File Created.\n");
    printf("fname is %s.\n", fname);
    return 0;
}

int fs_create_dir(const char *filepath, mode_t mode, struct fuse_file_info * ffi, FILE* fp){
    if(filepath == NULL)
        return -1; //have to check how to return error number
    //using namei find the directory and the data block to write into
    //namei(fp,filepath);
    struct inode nodep;
    block_num inode_num = allocate_inode(fp, &nodep);
    printf("in fs_create_dir: got inode no. %ld\n", inode_num);
    read_inode(fp, inode_num, &nodep);
    nodep.owner_id = 121 ;
    nodep.group_id = 1;
    nodep.type = 2;
    //nodep.file_modified = time(0);
    nodep.direct_blocks[0] = allocate_block(fp);
    // TODO: add entries for . and ..
    write_inode(fp, inode_num, &nodep);
    return 0;
}

int fs_open(const char* filepath, struct fuse_file_info* ffi, FILE* fp){
    //check for errors
    //if file does not exist then create a new file
    //use namei to get inode
    //get the data block using inode
    //write back the data block and inode when done
    block_num inode_num = fs_namei(fp, filepath);
    if(inode_num == 0){
        printf("file doesnt exist, creating...");
        fs_create(filepath, 0, ffi, fp);
    }
    else{
        printf("do nothing");
    }
    
    return 0;
}

int fs_read(const char *filepath, char *buf, size_t count, off_t offset, struct fuse_file_info * ffi, FILE* fp){
    /*check if user has permission to read
     if(inode->file_size < offset)
     return error;
     */
    block_num inode_num = 0;
    block_num block_no = 0;
    inode_num = fs_namei(fp, filepath);
    if(inode_num == 0)
        printf("file does not exist.\n");
    struct inode node;
    //read_inode(fp, inode_num, &node);
    block_no = get_file_block_num(offset/BLOCK_SIZE, inode_num, false, fp);
    offset = offset % BLOCK_SIZE; // offset within block
    //block_no = nodep.direct_blocks[0];
    read_block(buf, block_no, offset, count, fp);
    return count;
    //get data from block and display
}

int fs_write(const char* filepath, long offset, const char* buffer, long size, FILE* fp){
    // Get inode using silepath
    block_num inode_num = fs_namei(fp, filepath);
    mode_t mode = NULL; // TODO: Change this to defaults
    //struct fuse_file_info  ffi = NULL; // TODO: Change this to incoming parameter
    
    // Write expects file to be already created
    printf("File name:%s inode:%ld\n", filepath, inode_num);
    if(inode_num == 0){
        //fs_create(filepath, mode, ffi, fp);
        //inode_num = fs_namei(fp, filepath);
        printf("%s : No such file exists!\n", filepath);
        return -1;
    }
    
    block_num block_no = 0;
    struct inode node;
    //read_inode(fp, inode_num, &node);
    // Calculate from offset the block number and determine if it is direct, indirect block
    block_no = get_file_block_num(offset/BLOCK_SIZE, inode_num, true, fp);
    write_block(buffer, block_no, offset % BLOCK_SIZE, size, fp);
    if(ferror(fp))
        perror("Error writing data ");
    fflush(fp);
    
    //write_inode(fp, inode_num, &node);
    printf("file write complete.\n");
    return 0;
}

