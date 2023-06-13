#include "lfs.h"

int lfswrapper_lfs_mount();
int lfswrapper_dir_open(const char* path);
int lfswrapper_dir_read(int dir, struct lfs_info* info);
int lfswrapper_dir_seek(int dir, lfs_off_t off);
lfs_soff_t lfswrapper_dir_tell(int dir);
int lfswrapper_dir_rewind(int dir);
int lfswrapper_dir_close(int dir);
void lfswrapper_dump_dir(void);
