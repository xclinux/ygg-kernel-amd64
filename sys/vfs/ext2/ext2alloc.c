// ext2fs block/inode alloc/free
#include "sys/fs/ext2.h"
#include "sys/string.h"
#include "sys/assert.h"
#include "sys/debug.h"
#include "sys/errno.h"

// #include <assert.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <stdio.h>

int ext2_alloc_block(struct fs *ext2, uint32_t *block_no) {
    struct ext2_info *info = ext2->fs_private;
    char block_buffer[info->block_size];
    uint32_t res_block_no = 0;
    uint32_t res_group_no = 0;
    uint32_t res_block_no_in_group = 0;
    int found = 0;
    int res;

    for (size_t i = 0; i < info->block_group_count; ++i) {
        if (info->block_group_descriptor_table[i].free_blocks > 0) {
            // Found a free block here
            kdebug("Allocating a block in group #%u\n", i);

            if ((res = ext2_read_block(ext2,
                                       info->block_group_descriptor_table[i].block_usage_bitmap_block,
                                       block_buffer)) < 0) {
                return res;
            }

            for (size_t j = 0; j < info->block_size / sizeof(uint64_t); ++j) {
                uint64_t qw = ((uint64_t *) block_buffer)[j];
                if (qw != (uint64_t) -1) {
                    for (size_t k = 0; k < 64; ++k) {
                        if (!(qw & (1 << k))) {
                            res_block_no_in_group = k + j * 64;
                            res_group_no = i;
                            // XXX: had to increment the resulting block_no
                            //      because for some reason linux's ext2
                            //      impl hasn't marked #531 as used in one
                            //      case, but it was actually a L1-indirect
                            //      block. So I just had to make it allocate
                            //      #532 instead as a workaround (though
                            //      block numbering should start with 0)
                            res_block_no = res_block_no_in_group + i * info->sb.block_group_size_blocks + 1;
                            found = 1;
                            break;
                        }
                    }

                    if (found) {
                        break;
                    }
                }
            }
            if (found) {
                break;
            }
        }
    }

    if (!found) {
        return -ENOSPC;
    }

    // Write block usage bitmap
    ((uint64_t *) block_buffer)[res_block_no_in_group / 64] |= (1 << (res_block_no_in_group % 64));
    if ((res = ext2_write_block(ext2,
                                info->block_group_descriptor_table[res_group_no].block_usage_bitmap_block,
                                block_buffer)) < 0) {
        return res;
    }

    // Update BGDT
    --info->block_group_descriptor_table[res_group_no].free_blocks;
    for (size_t i = 0; i < info->block_group_descriptor_table_size_blocks; ++i) {
        void *blk_ptr = (void *) (((uintptr_t) info->block_group_descriptor_table) + i * info->block_size);

        if ((res = ext2_write_block(ext2, info->block_group_descriptor_table_block + i, blk_ptr)) < 0) {
            return res;
        }
    }

    // Update global block count and flush superblock
    --info->sb.free_block_count;
    if ((res = ext2_write_superblock(ext2)) < 0) {
        return res;
    }

    *block_no = res_block_no;
    kdebug("Allocated block #%u\n", res_block_no);
    return 0;
}

int ext2_free_block(struct fs *ext2, uint32_t block_no) {
    _assert(block_no);
    struct ext2_info *info = ext2->fs_private;
    char block_buffer[info->block_size];
    int res;

    uint32_t block_group_no = (block_no - 1) / info->sb.block_group_size_blocks;
    uint32_t block_no_in_group = (block_no - 1) % info->sb.block_group_size_blocks;

    // Read block ussge bitmap block
    if ((res = ext2_read_block(ext2,
                               info->block_group_descriptor_table[block_group_no].block_usage_bitmap_block,
                               block_buffer)) < 0) {
        return res;
    }

    // Update the bitmap
    _assert(((uint64_t *) block_buffer)[block_no_in_group / 64] & (1 << (block_no_in_group % 64)));
    ((uint64_t *) block_buffer)[block_no_in_group / 64] &= ~(1 << (block_no_in_group % 64));

    if ((res = ext2_write_block(ext2,
                                info->block_group_descriptor_table[block_group_no].block_usage_bitmap_block,
                                block_buffer)) < 0) {
        return res;
    }

    // Update BGDT
    ++info->block_group_descriptor_table[block_group_no].free_blocks;
    for (size_t i = 0; i < info->block_group_descriptor_table_size_blocks; ++i) {
        void *blk_ptr = (void *) (((uintptr_t) info->block_group_descriptor_table) + i * info->block_size);

        if ((res = ext2_write_block(ext2, info->block_group_descriptor_table_block + i, blk_ptr)) < 0) {
            return res;
        }
    }

    // Update global block count
    ++info->sb.free_block_count;
    if ((res = ext2_write_superblock(ext2)) < 0) {
        return res;
    }

    kdebug("Freed block #%u\n", block_no);

    return 0;
}

int ext2_inode_alloc_block(struct fs *ext2, struct ext2_inode *inode, uint32_t ino, uint32_t index) {
    struct ext2_info *info = ext2->fs_private;

    int res;
    uint32_t block_no;

    // Allocate the block itself
    if ((res = ext2_alloc_block(ext2, &block_no)) < 0) {
        return res;
    }

    if (index < 12) {
        // Write direct block list entry
        inode->direct_blocks[index] = block_no;
    } else if (index < 12 + (info->block_size / 4)) {
        // Also allocate L1 indirection block if needed
        char buf[info->block_size];
        if (!inode->l1_indirect_block) {
            uint32_t l1_block;
            if ((res = ext2_alloc_block(ext2, &l1_block)) < 0) {
                return res;
            }
            inode->l1_indirect_block = l1_block;
            _assert(inode->l1_indirect_block);
            memset(buf, 0, info->block_size);
        } else {
            // Read indirection block
            if ((res = ext2_read_block(ext2, inode->l1_indirect_block, buf)) < 0) {
                return res;
            }
        }

        ((uint32_t *) buf)[index - 12] = block_no;

        // Write the block back
        if ((res = ext2_write_block(ext2, inode->l1_indirect_block, buf)) < 0) {
            return res;
        }
    } else {
        panic("Inode block index has too high indirection level\n");
    }

    // Flush changes to the device
    return ext2_write_inode(ext2, inode, ino);
}

//int ext2_free_inode_block(struct fs *ext2, struct ext2_inode *inode, uint32_t ino, uint32_t index) {
//    if (index >= 12) {
//        panic("Not implemented\n");
//    }
//    // All sanity checks regarding whether the block is present
//    // at all are left to the caller
//
//    int res;
//    uint32_t block_no;
//
//    // Get block number
//    block_no = inode->direct_blocks[index];
//
//    // Free the block
//    if ((res = ext2_free_block(ext2, block_no)) < 0) {
//        return res;
//    }
//
//    // Write updated inode
//    inode->direct_blocks[index] = 0;
//    return ext2_write_inode(ext2, inode, ino);
//}

int ext2_free_inode(struct fs *ext2, uint32_t ino) {
    _assert(ino);
    struct ext2_info *info = ext2->fs_private;
    char block_buffer[info->block_size];
    uint32_t ino_block_group_number = (ino - 1) / info->sb.block_group_size_inodes;
    uint32_t ino_inode_index_in_group = (ino - 1) % info->sb.block_group_size_inodes;
    int res;

    // Read inode usage block
    if ((res = ext2_read_block(ext2,
                               info->block_group_descriptor_table[ino_block_group_number].inode_usage_bitmap_block,
                               block_buffer)) < 0) {
        return res;
    }

    // Remove usage bit
    _assert(((uint64_t *) block_buffer)[ino_inode_index_in_group / 64] & (1 << (ino_inode_index_in_group % 64)));
    ((uint64_t *) block_buffer)[ino_inode_index_in_group / 64] &= ~(1 << (ino_inode_index_in_group % 64));

    // Write modified bitmap back
    if ((res = ext2_write_block(ext2,
                                info->block_group_descriptor_table[ino_block_group_number].inode_usage_bitmap_block,
                                block_buffer)) < 0) {
        return res;
    }

    // Increment free inode count in BGDT entry and write it back
    // TODO: this code is repetitive and maybe should be moved to
    //       ext2_bgdt_inode_inc/_dec()
    ++info->block_group_descriptor_table[ino_block_group_number].free_inodes;
    for (size_t i = 0; i < info->block_group_descriptor_table_size_blocks; ++i) {
        void *blk_ptr = (void *) (((uintptr_t) info->block_group_descriptor_table) + i * info->block_size);

        if ((res = ext2_write_block(ext2, info->block_group_descriptor_table_block + i, blk_ptr)) < 0) {
            return res;
        }
    }

    // Update global inode count
    ++info->sb.free_inode_count;
    if ((res = ext2_write_superblock(ext2)) < 0) {
        return res;
    }

    kdebug("Freed inode #%u\n", ino);
    return 0;
}

int ext2_alloc_inode(struct fs *ext2, uint32_t *ino) {
    struct ext2_info *info = ext2->fs_private;
    char block_buffer[info->block_size];
    uint32_t res_ino = 0;
    uint32_t res_group_no = 0;
    uint32_t res_ino_number_in_group = 0;
    int res;

    // Look through BGDT to find any block groups with free inodes
    for (size_t i = 0; i < info->block_group_count; ++i) {
        if (info->block_group_descriptor_table[i].free_inodes > 0) {
            // Found a block group with free inodes
            kdebug("Allocating an inode inside block group #%u\n", i);

            // Read inode usage bitmap
            if ((res = ext2_read_block(ext2,
                                       info->block_group_descriptor_table[i].inode_usage_bitmap_block,
                                       block_buffer)) < 0) {
                return res;
            }

            // Find a free bit
            // Think this should be fine on amd64
            for (size_t j = 0; j < info->block_size / sizeof(uint64_t); ++j) {
                // Get bitmap qword
                uint64_t qw = ((uint64_t *) block_buffer)[j];
                // If not all bits are set in this qword, find exactly which one
                if (qw != ((uint64_t) -1)) {
                    for (size_t k = 0; k < 64; ++k) {
                        if (!(qw & (1 << k))) {
                            res_ino_number_in_group = k + j * 64;
                            res_group_no = i;
                            res_ino = res_ino_number_in_group + i * info->sb.block_group_size_inodes + 1;
                            break;
                        }
                    }

                    if (res_ino) {
                        break;
                    }
                }

                if (res_ino) {
                    break;
                }
            }
        }

        if (res_ino) {
            break;
        }
    }
    if (res_ino == 0) {
        return -ENOSPC;
    }

    // Write updated bitmap
    ((uint64_t *) block_buffer)[res_ino_number_in_group / 64] |= (1 << (res_ino_number_in_group % 64));
    if ((res = ext2_write_block(ext2,
                                info->block_group_descriptor_table[res_group_no].inode_usage_bitmap_block,
                                block_buffer)) < 0) {
        return res;
    }

    // Write updated BGDT
    --info->block_group_descriptor_table[res_group_no].free_inodes;
    for (size_t i = 0; i < info->block_group_descriptor_table_size_blocks; ++i) {
        void *blk_ptr = (void *) (((uintptr_t) info->block_group_descriptor_table) + i * info->block_size);

        if ((res = ext2_write_block(ext2, info->block_group_descriptor_table_block + i, blk_ptr)) < 0) {
            return res;
        }
    }

    // Update global inode count and flush superblock
    --info->sb.free_inode_count;
    if ((res = ext2_write_superblock(ext2)) < 0) {
        return res;
    }

    *ino = res_ino;

    return 0;
}

