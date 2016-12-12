#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define BUF_SIZE 200
#define INODE_BUFFER 256
#define BLOCK_INAVLID -1

typedef int16_t __s16;
typedef uint32_t __u32;
typedef int32_t __s32;
typedef uint16_t __u16;

static int fd = -1;
static struct group_descriptor* group = NULL;
static char* buf; //buf to hold error messages
static struct inode* inodes = NULL;
static __u32 allocated_inodes = 0;
static int num_groups;
static __u32 last_group_blocks;
static __u32 last_group_inodes;

// Struct copied straight from ext2_fs.h

struct ext2_super_block {
    __u32	s_inodes_count;		/* Inodes count */
    __u32	s_blocks_count;		/* Blocks count */
    __u32	s_r_blocks_count;	/* Reserved blocks count */
    __u32	s_free_blocks_count;	/* Free blocks count */
    __u32	s_free_inodes_count;	/* Free inodes count */
    __u32	s_first_data_block;	/* First Data Block */
    __u32	s_log_block_size;	/* Block size */
    __s32	s_log_frag_size;	/* Fragment size */
    __u32	s_blocks_per_group;	/* # Blocks per group */
    __u32	s_frags_per_group;	/* # Fragments per group */
    __u32	s_inodes_per_group;	/* # Inodes per group */
    __u32	s_mtime;		/* Mount time */
    __u32	s_wtime;		/* Write time */
    __u16	s_mnt_count;		/* Mount count */
    __s16	s_max_mnt_count;	/* Maximal mount count */
    __u16	s_magic;		/* Magic signature */
    __u16	s_state;		/* File system state */
    __u16	s_errors;		/* Behaviour when detecting errors */
    __u16	s_minor_rev_level; 	/* minor revision level */
    __u32	s_lastcheck;		/* time of last check */
    __u32	s_checkinterval;	/* max. time between checks */
    __u32	s_creator_os;		/* OS */
    __u32	s_rev_level;		/* Revision level */
    __u16	s_def_resuid;		/* Default uid for reserved blocks */
    __u16	s_def_resgid;		/* Default gid for reserved blocks */

    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    __u32	s_first_ino; 		/* First non-reserved inode */
    __u16   s_inode_size; 		/* size of inode structure */
    __u16	s_block_group_nr; 	/* block group # of this superblock */
    __u32	s_feature_compat; 	/* compatible feature set */
    __u32	s_feature_incompat; 	/* incompatible feature set */
    __u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
    __u32	s_reserved[230];	/* Padding to the end of the block */
    __u32   s_block_size;        //Block size i.e. 1024 << log_block_size
    __u32   s_frag_size;        //Fragment size i.e. 1024 << log_frag size
} super;

struct group_descriptor{
     __u32 block_bitmap;
     __u32 inode_bitmap;
     __u32 inode_table;
    __u16 free_blocks_count;
    __u16 free_inodes_count;
    __u16 num_directories;
    __u32 is_valid;
    __u32* inodes;
};

struct inode{
    long long inode_number;
    unsigned char file_type;
    __u16 mode;
    __u16 id;
    __u16 g_id;
    __u16 links_count;
    __u32 access_time;
    __u32 mod_time;
    __u32 creation_time;
    __u32 file_size;
    __u32 num_blocks;
    __u32 blocks[15];
};

void error(char* message){
    perror(message);
    exit(EXIT_FAILURE);
}

ssize_t Read(int fildes, void *buf, size_t count){
    ssize_t check;
    if((check = read(fildes, buf, count)) == -1)
        error("Read failed");
    return check;
}

void get_args(int argc, char* argv[]){
    if (argc != 2){
        sprintf(buf, "Please pass in only one argument which is the file system image");
        error(buf);
    }
    char* file_system = argv[1];
    fd = open(file_system, O_RDONLY);
    if (fd < 0){
        sprintf(buf, "Could not open disk image file %s", file_system);
        error(buf);
    }
}

void check_block_count(struct ext2_super_block* super){
    struct stat st;
    fstat(fd, &st);
    off_t size = st.st_size;
    off_t super_size = super->s_block_size * super->s_blocks_count;
    if (super_size > size)
        error("Block count times block size greater than file size");
}

void analyze_superblock(struct ext2_super_block* super){
    if(super->s_magic != 0xEF53){
        sprintf(buf, "Superblock - Magic number is ,%d", super->s_magic);
        error(buf);
    }
    super->s_block_size = 1024 << super->s_log_block_size;
    if(super->s_log_frag_size > 0)
        super->s_frag_size = 1024 << super->s_log_frag_size;
    else
        super->s_frag_size = 1024 >> super->s_log_frag_size;
    
    if (super->s_first_data_block != 0 && super->s_first_data_block != 1)
        error("Superblock - First data block should always be 0 or 1");
    check_block_count(super);
    if (super->s_blocks_count % super->s_blocks_per_group != 0)
        error("Total blocks and blocks per group don't divide evenly");
    if (super->s_inodes_count % super->s_inodes_per_group != 0)
        error("Total inodes and inodes per group don't divide evenly");
    FILE* super_csv = fopen("super.csv", "w");
    fprintf(super_csv, "%x,%d,%d,%d,%d,%d,%d,%d,%d\n",super->s_magic,super->s_inodes_count,super->s_blocks_count,(int)super->s_block_size,(int)super->s_frag_size,super->s_blocks_per_group,super->s_inodes_per_group, super->s_frags_per_group,super->s_first_data_block);
}

void read_inode_block(off_t offset, FILE* fildes, long long inode_num, int* entry_num){
    off_t limit;
    __u32 inode;
    __u16 rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char* name;
    limit = offset + super.s_block_size;
    while(offset < limit){
        lseek(fd, offset, SEEK_SET);
        Read(fd, &inode, 4);
        Read(fd, &rec_len, 2);
        Read(fd, &name_len, 1);
        Read(fd, &file_type, 1);
        if(inode == 0){
            offset += rec_len;
            *entry_num = *entry_num + 1;
            continue;
        }
        else if(rec_len < 8 || rec_len > 1024){
            perror("Entry length is unreasonable");
            offset += rec_len;
            *entry_num = *entry_num + 1;
            continue;
        }
        else if(name_len > rec_len){
            perror("Name length doesn't fit within entry length");
            offset += rec_len;
            *entry_num = *entry_num + 1;
            continue;
        }
        else if(inode > super.s_inodes_count){
            perror("File entry inode number greater than number of inodes");
            offset += rec_len;
            *entry_num = *entry_num + 1;
            continue;
        }
        name = calloc(name_len + 3,1);
        name[0] = '"';
        name[name_len+1] = '"';
        Read(fd, name+1, name_len);
        fprintf(fildes, "%lld,%d,%d,%d,%d,%s\n",inode_num,*entry_num,rec_len,name_len,inode,name);
        offset += rec_len;
        *entry_num = *entry_num + 1;
        free(name);
    }
}

void read_indirect_block(off_t offset, FILE* fildes, long long inode_num, int* entry_num, int directory_only){
    int j;
    int indirect_entry_num = 0;
    for(j = 0; j < super.s_block_size; j+=4){
        __u32 data_block;
        pread(fd, &data_block, 4, offset+j);
        if(data_block && directory_only)
            read_inode_block(data_block*super.s_block_size, fildes, inode_num, entry_num);
        else if(data_block){
            off_t containing_block = offset/super.s_block_size;
            if(containing_block > super.s_blocks_count){
                fprintf(stderr, "Block number out of range %lld", containing_block);
                continue;
            }
            __u32 pointer_value = data_block;
            fprintf(fildes, "%llx,%d,%x\n",containing_block,indirect_entry_num,pointer_value);
            indirect_entry_num++;
        }
        else
            break;
    }
}

void read_double_indirect_block(off_t offset, FILE* fildes, long long inode_num, int* entry_num, int directory_only){
    int j;
    int indirect_entry_num = 0;
    for(j = 0; j < super.s_block_size; j+=4){
        __u32 data_block;
        pread(fd, &data_block, 4, offset+j);
        if(data_block && directory_only)
            read_indirect_block(data_block*super.s_block_size, fildes, inode_num, entry_num, directory_only);
        else if(data_block){
            off_t containing_block = offset/super.s_block_size;
            if(containing_block > super.s_blocks_count){
                fprintf(stderr, "Block number out of range %lld", containing_block);
                continue;
            }
            __u32 pointer_value = data_block;
            fprintf(fildes, "%llx,%d,%x\n",containing_block,indirect_entry_num,pointer_value);
            indirect_entry_num++;
            read_indirect_block(data_block*super.s_block_size, fildes, inode_num, entry_num, directory_only);
        }
        else
            break;
    }
}

void read_directories(){
    FILE* directory_fd = fopen("directory.csv", "w");
    int length = allocated_inodes;
    int i;
    long long inode_num;
    struct inode* cur;
    int quit = 0;
    off_t offset;
    for(i = 0; i < length; i++){
        if(inodes[i].file_type == 'd'){
            cur = &inodes[i];
            inode_num = inodes[i].inode_number;
            int j;
            int entry_num = 0;
            for(j = 0; j < 12; j++){
                if(!cur->blocks[j]){
                    quit = 1;
                    break;
                }
                if(cur->blocks[j] == BLOCK_INAVLID)
                    continue;
                offset = cur->blocks[j]*super.s_block_size;
                read_inode_block(offset, directory_fd, inode_num, &entry_num);
            }
            if(quit){
                quit = 0;
                continue;
            }
            if(!cur->blocks[12])
                continue;
            offset = cur->blocks[12]*super.s_block_size;
            read_indirect_block(offset, directory_fd, inode_num, &entry_num, 1);
            if(!cur->blocks[13])
                continue;
            offset = cur->blocks[13]*super.s_block_size;
            read_double_indirect_block(offset, directory_fd, inode_num, &entry_num, 1);
            if(!cur->blocks[14])
                continue;
            offset = cur->blocks[14]*super.s_block_size;
            for(j = 0; j < super.s_block_size; j+=4){
                __u32 data_block;
                pread(fd, &data_block, 4, offset+j);
                if(data_block)
                    read_double_indirect_block(data_block*super.s_block_size, directory_fd, inode_num, &entry_num, 1);
                else
                    break;
            }
            
        }
    }
    fclose(directory_fd);
}

void read_indirect(){
    int length = allocated_inodes;
    int i;
    FILE* indirect_csv = fopen("indirect.csv", "w");
    off_t offset;
    struct inode* cur;
    for(i = 0; i < length; i++){
        cur = &inodes[i];
        int j;
        int quit = 0;
        for(j = 0; j < 12; j++){
            if(!cur->blocks[j]){
                quit = 1;
                break;
            }
        }
        if(quit)
            continue;
        if(!cur->blocks[12])
            continue;
        offset = inodes[i].blocks[12]*super.s_block_size;
        read_indirect_block(offset, indirect_csv, 0, 0, 0);
        if(!cur->blocks[13])
            continue;
        offset = cur->blocks[13]*super.s_block_size;
        read_double_indirect_block(offset, indirect_csv, 0, 0, 0);
        if(!cur->blocks[14])
            continue;
        offset = cur->blocks[14]*super.s_block_size;
        int indirect_entry_num = 0;
        for(j = 0; j < super.s_block_size; j+=4){
            __u32 data_block;
            pread(fd, &data_block, 4, offset+j);
            if(data_block){
                off_t containing_block = offset/super.s_block_size;
                if(containing_block > super.s_blocks_count){
                    fprintf(stderr, "Block number out of range %lld", containing_block);
                    continue;
                }
                __u32 pointer_value = data_block;
                fprintf(indirect_csv, "%llx,%d,%x\n",containing_block,indirect_entry_num,pointer_value);
                indirect_entry_num++;
                read_double_indirect_block(data_block*super.s_block_size, indirect_csv, 0, 0, 0);
            }
            else
                break;
        }

    }
    fclose(indirect_csv);
}

void read_inodes(){
    int i;
    off_t offset;
    off_t limit;
    long long inode_num = 1;
    inodes = malloc(sizeof(struct inode)*(allocated_inodes));
    int arr_index = 0;
    __u16 mode;
    struct inode* cur;
    FILE* inode_csv = fopen("inode.csv", "w");
    for(i = 0; i < num_groups; i++){
        offset = group[i].inode_table*super.s_block_size;
        if(i == num_groups - 1){
            limit = offset + (128*last_group_inodes);
        }
        else
            limit = offset + (128*super.s_inodes_per_group);
        
        int inode_index = 0;
        while (offset < limit){
            lseek(fd, offset, SEEK_SET);
            if(group[i].inodes[inode_index]){
                cur = &inodes[arr_index];
                Read(fd, &inodes[arr_index].mode, 2);
                mode = inodes[arr_index].mode;
                Read(fd, &inodes[arr_index].id, 2);
                Read(fd, &inodes[arr_index].file_size, 4);
                Read(fd, &inodes[arr_index].access_time, 4);
                Read(fd, &inodes[arr_index].creation_time, 4);
                Read(fd, &inodes[arr_index].mod_time, 4);
                lseek(fd, 4, SEEK_CUR);
                Read(fd, &inodes[arr_index].g_id, 2);
                Read(fd, &inodes[arr_index].links_count, 2);
                Read(fd, &inodes[arr_index].num_blocks, 4);
                lseek(fd, 8, SEEK_CUR);
                Read(fd, &inodes[arr_index].blocks, 60);
                cur->num_blocks = cur->num_blocks/(2 << super.s_log_block_size);
                if(mode & 0x8000)
                    inodes[arr_index].file_type = 'f';
                else if(mode & 0x4000)
                    inodes[arr_index].file_type = 'd';
                else if(mode & 0xA000)
                    inodes[arr_index].file_type = 's';
                else
                    inodes[arr_index].file_type = '?';
                inodes[arr_index].inode_number = inode_num;
                fprintf(inode_csv, "%lld,%c,%o,%d,%d,%d,%x,%x,%x,%d,%d,",cur->inode_number,cur->file_type,cur->mode,cur->id,cur->g_id,cur->links_count,cur->creation_time,cur->mod_time,cur->access_time,cur->file_size,cur->num_blocks);
                int j;
                __u32 block;
                for(j = 0; j < 15; j++){
                    block = cur->blocks[j];
                    if((block < super.s_first_data_block && block != 0) || block > (super.s_blocks_count)){
                        fprintf(stderr, "Invalid block pointer at index %d, --> %x, Super block count is %x\n",j,block,super.s_blocks_count);
                        if (cur->mode == 'd' || j >= 12)
                            cur->blocks[j] = BLOCK_INAVLID;
                    }
                    if(j < 14)
                        fprintf(inode_csv, "%x,", block);
                    else
                        fprintf(inode_csv, "%x\n", block);
                }
                arr_index++;
            }
            inode_num++;
            inode_index++;
            offset += 128;
        }
    }
    fclose(inode_csv);
}

void read_bitmaps(){
    int index;
    int block_bitmap;
    int block_inode;
    off_t offset;
    int block_num = 0;
    int inode_num = 0;
    FILE* bitmap_csv = fopen("bitmap.csv", "w");
    uint8_t* block_bitmap_buffer = malloc(super.s_block_size);
    uint8_t* inode_bitmap_buffer = malloc(super.s_inodes_per_group/8);
    for(index = 0; index < num_groups; index++){
        __u32 block_size;
        __u32 inode_per_group;
        if(index == num_groups - 1){
            block_size = last_group_blocks;
            inode_per_group = last_group_inodes;
        }
        else{
            block_size = super.s_blocks_per_group;
            inode_per_group = super.s_inodes_per_group;
        }
        int group_inode_num = 0;
        group[index].inodes = malloc(sizeof(int)*super.s_inodes_per_group);
        block_bitmap = group[index].block_bitmap;
        offset = (block_bitmap*super.s_block_size);
        pread(fd, block_bitmap_buffer, super.s_block_size, offset);
        int num_bytes = 0;
        while (num_bytes < block_size/8){
            int mask = 1;
            int i;
            uint8_t byte = block_bitmap_buffer[num_bytes];
            for(i = 0; i < 8; i++){
                block_num++;
                if(!(mask & byte))
                    fprintf(bitmap_csv, "%x,%d\n", block_bitmap, block_num);
                mask = mask << 1;
            }
            num_bytes += 1;
        }
        block_inode = group[index].inode_bitmap;
        offset = (block_inode*super.s_block_size);
        pread(fd, inode_bitmap_buffer, super.s_inodes_per_group/8, offset);
        num_bytes = 0;
        while (num_bytes < inode_per_group/8){
            int mask = 1;
            int i;
            uint8_t byte = inode_bitmap_buffer[num_bytes];
            for(i = 0; i < 8; i++){
                inode_num++;
                if(!(mask & byte)){
                    fprintf(bitmap_csv, "%x,%d\n", block_inode, inode_num);
                    group[index].inodes[group_inode_num] = 0;
                }
                else{
                    allocated_inodes++;
                    group[index].inodes[group_inode_num] = 1;
                }
                mask = mask << 1;
                group_inode_num++;
            }
            num_bytes += 1;
        }
    }
    free(block_bitmap_buffer);
    free(inode_bitmap_buffer);
    fclose(bitmap_csv);
}

void read_other_blocks(){
    off_t next_block_location = 1024 + super.s_block_size;
    if(super.s_blocks_count % super.s_blocks_per_group == 0){
        num_groups = super.s_blocks_count/super.s_blocks_per_group;
        last_group_blocks = super.s_blocks_per_group;
    }
    else{
        num_groups = (super.s_blocks_count/super.s_blocks_per_group) + 1;
        last_group_blocks = super.s_blocks_count % super.s_blocks_per_group;
    }
    if(super.s_inodes_count % super.s_inodes_per_group == 0){
        last_group_inodes = super.s_inodes_per_group;
    }
    else{
        last_group_inodes = super.s_inodes_count % super.s_inodes_per_group;
    }
    off_t limit = next_block_location + (32 * num_groups);
    group = malloc(sizeof(struct group_descriptor) * num_groups);
    int index = 0;
    while (next_block_location < limit){
        lseek(fd, next_block_location, SEEK_SET);
        group[index].is_valid = 1;
        off_t block_num;
        if(index == 0)
            block_num = 1;
        else
            block_num = super.s_blocks_per_group * index;
        Read(fd, &group[index].block_bitmap, 4);
        if(group[index].block_bitmap < block_num || group[index].block_bitmap > (block_num + super.s_blocks_per_group)){
            perror("Block bitmap outside of range, ignoring this block..");
            group[index].is_valid = 0;
        }
        Read(fd, &group[index].inode_bitmap, 4);
        if(group[index].inode_bitmap < block_num || group[index].inode_bitmap > block_num + super.s_blocks_per_group){
            perror("Inode bitmap outside of range, ignoring this block..");
            group[index].is_valid = 0;
        }
        Read(fd, &group[index].inode_table, 4);
        if(group[index].inode_table < block_num || group[index].inode_table > block_num + super.s_blocks_per_group){
            perror("Inode table outside of range, ignoring this block..");
            group[index].is_valid = 0;
        }
        Read(fd, &group[index].free_blocks_count, 2);
        Read(fd, &group[index].free_inodes_count, 2);
        Read(fd, &group[index].num_directories, 2);
        index++;
        next_block_location += 32;
    }
    FILE* gd_csv = fopen("group.csv", "w");
    for(index = 0; index < num_groups; index++){
        if(group[index].is_valid){
            fprintf(gd_csv, "%d,%d,%d,%d,%x,%x,%x\n",super.s_blocks_per_group,group[index].free_blocks_count,group[index].free_inodes_count,group[index].num_directories, group[index].inode_bitmap, group[index].block_bitmap, group[index].inode_table);
        }
    }
    fclose(gd_csv);
}

void read_superblock(){
    lseek(fd, 1024, SEEK_SET);
    Read(fd, &super, 1024);
    analyze_superblock(&super);
}

void destroy(){
    int i;
    for(i = 0; i < num_groups; i++)
        free(group[i].inodes);
    free(group);
    free(inodes);
    free(buf);
}

int main(int argc, char * argv[]) {
    buf = malloc(BUF_SIZE);
    get_args(argc, argv);
    read_superblock();
    read_other_blocks();
    read_bitmaps();
    read_inodes();
    read_directories();
    read_indirect();
    destroy();
}
