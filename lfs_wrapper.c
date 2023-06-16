#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "lfs_wrapper.h"

// Define the flash sizes
// This is setup to read a block of the flash from the end
// Same config as Micropython would should mean the littlefs filesytem
// will remain intact between MicroPython and PiccoloBASIC.

#define BLOCK_SIZE_BYTES (FLASH_SECTOR_SIZE)

#ifndef PICCOLOBASIC_HW_FLASH_STORAGE_BYTES
#define PICCOLOBASIC_HW_FLASH_STORAGE_BYTES (1408 * 1024)
#endif
static_assert(PICCOLOBASIC_HW_FLASH_STORAGE_BYTES % 4096 == 0, "Flash storage size must be a multiple of 4K");

#ifndef PICCOLOBASIC_HW_FLASH_STORAGE_BASE
#define PICCOLOBASIC_HW_FLASH_STORAGE_BASE (PICO_FLASH_SIZE_BYTES - PICCOLOBASIC_HW_FLASH_STORAGE_BYTES)
#endif

static_assert(PICCOLOBASIC_HW_FLASH_STORAGE_BYTES <= PICO_FLASH_SIZE_BYTES, "PICCOLOBASIC_HW_FLASH_STORAGE_BYTES too big");
static_assert(PICCOLOBASIC_HW_FLASH_STORAGE_BASE + PICCOLOBASIC_HW_FLASH_STORAGE_BYTES <= PICO_FLASH_SIZE_BYTES, "PICCOLOBASIC_HW_FLASH_STORAGE_BYTES too big");

// variables used by the filesystem
lfs_t lfs;
lfs_file_t current_lfs_file;

int pico_lfsflash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t fs_start = XIP_BASE + PICCOLOBASIC_HW_FLASH_STORAGE_BASE;
    uint32_t addr = fs_start + (block * c->block_size) + off;

    // printf("[FS] READ: %p, %d\n", addr, size);

    memcpy(buffer, (unsigned char *)addr, size);
    return 0;
}

int pico_lfsflash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t fs_start = PICCOLOBASIC_HW_FLASH_STORAGE_BASE;
    uint32_t addr = fs_start + (block * c->block_size) + off;

    // printf("[FS] WRITE: %p, %d\n", addr, size);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(addr, (const uint8_t *)buffer, size);
    restore_interrupts(ints);

    return 0;
}

int pico_lfsflash_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t fs_start = PICCOLOBASIC_HW_FLASH_STORAGE_BASE;
    uint32_t offset = fs_start + (block * c->block_size);

    // printf("[FS] ERASE: %p, %d\n", offset, block);

    uint32_t ints = save_and_disable_interrupts();   
    flash_range_erase(offset, c->block_size);  
    restore_interrupts(ints);

    return 0;
}

int pico_lfsflash_sync(const struct lfs_config *c)
{
    return 0;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config lfswrapper_cfg = {
    // block device operations
    .read  = &pico_lfsflash_read,
    .prog  = &pico_lfsflash_prog,
    .erase = &pico_lfsflash_erase,
    .sync  = &pico_lfsflash_sync,

    // block device configuration
    .read_size = FLASH_PAGE_SIZE,
    .prog_size = FLASH_PAGE_SIZE,

    .block_size = BLOCK_SIZE_BYTES,
    .block_count = PICCOLOBASIC_HW_FLASH_STORAGE_BYTES / BLOCK_SIZE_BYTES,
    .block_cycles = 500,

    .cache_size = FLASH_PAGE_SIZE,
    .lookahead_size = FLASH_PAGE_SIZE,
};

int lfswrapper_lfs_mount() {
	// mount the filesystem
	int err = lfs_mount(&lfs, &lfswrapper_cfg);

	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err) {
        	lfs_format(&lfs, &lfswrapper_cfg);
		lfs_mount(&lfs, &lfswrapper_cfg);
	}
}

int lfswrapper_dir_open(const char* path) {
	lfs_dir_t* dir = lfs_malloc(sizeof(lfs_dir_t));
	if (dir == NULL)
		return -1;
	if (lfs_dir_open(&lfs, dir, path) != LFS_ERR_OK) {
		lfs_free(dir);
		return -1;
	}
	return (int)dir;
}
int lfswrapper_dir_read(int dir, struct lfs_info* info) { return lfs_dir_read(&lfs, (lfs_dir_t*)dir, info); }

int lfswrapper_dir_seek(int dir, lfs_off_t off) { return lfs_dir_seek(&lfs, (lfs_dir_t*)dir, off); }

lfs_soff_t lfswrapper_dir_tell(int dir) { return lfs_dir_tell(&lfs, (lfs_dir_t*)dir); }

int lfswrapper_dir_rewind(int dir) { return lfs_dir_rewind(&lfs, (lfs_dir_t*)dir); }

int lfswrapper_dir_close(int dir) {
	return lfs_dir_close(&lfs, (lfs_dir_t*)dir);
	lfs_free((void*)dir);
}

int  lfswrapper_file_open(char *n, int flags) {
    return lfs_file_open(&lfs, &current_lfs_file, n, flags);
}

int lfswrapper_file_close() {
    return lfs_file_close(&lfs, &current_lfs_file);
}

int lfswrapper_file_write(const void *buffer, int sz) {
    return (int) lfs_file_write(&lfs, &current_lfs_file, buffer, (lfs_size_t) sz);
}

int lfswrapper_file_read(void *buffer, int sz) {
    return (int) lfs_file_read(&lfs, &current_lfs_file, buffer, sz);
}

int lfswrapper_get_file_size(char *path) {
    struct lfs_info info;
    int sts = lfs_stat(&lfs, path, &info);
    if(sts < 0) return -1;
    return info.size;
}

void lfswrapper_dump_dir(char *path) {
	// display each directory entry name
	printf("%s\n", path);
	int dir = lfswrapper_dir_open(path);
    	if (dir < 0)
        	return;

	struct lfs_info info;
	while (lfswrapper_dir_read(dir, &info) > 0)
        	printf("%s\n", info.name);
	lfswrapper_dir_close(dir);
}
