/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sd/sd_spec.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>

#ifdef CONFIG_FILE_SYSTEM_SHELL_MOUNT_COMMAND
/* FAT */
#ifdef CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#define FATFS_MNTP "/RAM:"
/* FatFs work area */
FATFS fat_fs;
/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};
#endif
/* LITTLEFS */
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <zephyr/fs/littlefs.h>

/* TODO: Implement dynamic storage dev selection */
#ifdef CONFIG_FS_LITTLEFS_BLK_DEV

#if defined(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME "SD"
#elif defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME "SD2"
#else
#error "No disk device defined, is your board supported?"
#endif

FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(
	lfs_data,
	CONFIG_SDHC_BUFFER_ALIGNMENT,
	SDMMC_DEFAULT_BLOCK_SIZE,
	SDMMC_DEFAULT_BLOCK_SIZE,
	SDMMC_DEFAULT_BLOCK_SIZE,
	2 * SDMMC_DEFAULT_BLOCK_SIZE);

static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &lfs_data,
	.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
	.storage_dev = DISK_NAME,
};
#else
#include <zephyr/storage/flash_map.h>

#define STORAGE_PARTIION_NODE_ID DT_PHANDLE(DT_INST(0, zephyr_fstab_littlefs), partition)

#if DT_FIXED_PARTITION_EXISTS(STORAGE_PARTIION_NODE_ID)
#define STORAGE_PARTITION_ID DT_FIXED_PARTITION_ID(STORAGE_PARTIION_NODE_ID)
#else
#define STORAGE_PARTITION    storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION)
#endif

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_data);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &lfs_data,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
};
#endif
#endif
#endif

#define BUF_CNT 64

#define MAX_PATH_LEN     128
#define MAX_FILENAME_LEN 128
#define MAX_INPUT_LEN    20

#define SHELL_FS "fs"

/* Maintenance guarantees this begins with '/' and is NUL-terminated. */
static char cwd[MAX_PATH_LEN] = "/";

static void create_abs_path(const char *name, char *path, size_t len)
{
	if (name[0] == '/') {
		strncpy(path, name, len);
		path[len - 1] = '\0';
	} else {
		if (cwd[1] == '\0') {
			__ASSERT_NO_MSG(len >= 2);
			*path++ = '/';
			--len;

			strncpy(path, name, len);
			path[len - 1] = '\0';
		} else {
			strncpy(path, cwd, len);
			path[len - 1] = '\0';

			size_t plen = strlen(path);

			if (plen < len) {
				path += plen;
				*path++ = '/';
				len -= plen + 1U;
				strncpy(path, name, len);
				path[len - 1] = '\0';
			}
		}
	}
}

static int cmd_cd(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dirent entry;
	int err;

	if (argc < 2) {
		strcpy(cwd, "/");
		return 0;
	}

	if (strcmp(argv[1], "..") == 0) {
		char *prev = strrchr(cwd, '/');

		if (!prev || prev == cwd) {
			strcpy(cwd, "/");
		} else {
			*prev = '\0';
		}

		/* No need to test that a parent exists */
		return 0;
	}

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_stat(path, &entry);
	if (err != 0) {
		shell_error(sh, "%s doesn't exist", path);
		return -ENOENT;
	}

	if (entry.type != FS_DIR_ENTRY_DIR) {
		shell_error(sh, "%s is not a directory", path);
		return -ENOTDIR;
	}

	strncpy(cwd, path, sizeof(cwd));
	cwd[sizeof(cwd) - 1] = '\0';

	return 0;
}

static int cmd_ls(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dir_t dir;
	int err;

	if (argc < 2) {
		strncpy(path, cwd, sizeof(path));
		path[sizeof(path) - 1] = '\0';
	} else {
		create_abs_path(argv[1], path, sizeof(path));
	}

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, path);
	if (err != 0) {
		shell_error(sh, "Unable to open %s (err %d)", path, err);
		return -EIO;
	}

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err != 0) {
			shell_error(sh, "Unable to read directory");
			break;
		}

		/* Check for end of directory listing */
		if (entry.name[0] == '\0') {
			break;
		}

		shell_print(sh, "%s%s", entry.name, (entry.type == FS_DIR_ENTRY_DIR) ? "/" : "");
	}

	fs_closedir(&dir);

	return 0;
}

static int cmd_pwd(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "%s", cwd);

	return 0;
}

static int cmd_trunc(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_file_t file;
	int length;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (argc > 2) {
		length = strtol(argv[2], NULL, 0);
	} else {
		length = 0;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_WRITE);
	if (err != 0) {
		shell_error(sh, "Failed to open %s (%d)", path, err);
		return -EIO;
	}

	err = fs_truncate(&file, length);
	if (err != 0) {
		shell_error(sh, "Failed to truncate %s (%d)", path, err);
		err = -EIO;
	}

	fs_close(&file);

	return err;
}

static int cmd_mkdir(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_mkdir(path);
	if (err != 0) {
		shell_error(sh, "Error creating dir[%d]", err);
		err = -EIO;
	}

	return err;
}

static int cmd_rm(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_unlink(path);
	if (err != 0) {
		shell_error(sh, "Failed to remove %s (%d)", path, err);
		err = -EIO;
	}

	return err;
}

static int cmd_cp(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int close_err;
	char path_src[MAX_PATH_LEN];
	char path_dst[MAX_PATH_LEN];
	struct fs_file_t file_src;
	struct fs_file_t file_dst;
	uint8_t buf[BUF_CNT];
	ssize_t buf_len;
	ssize_t num_written;

	create_abs_path(argv[1], path_src, sizeof(path_src));
	create_abs_path(argv[2], path_dst, sizeof(path_dst));

	fs_file_t_init(&file_src);
	fs_file_t_init(&file_dst);

	err = fs_open(&file_src, path_src, FS_O_READ);
	if (err != 0) {
		shell_error(sh, "Failed to open %s (%d)", path_src, err);
		err = -EIO;
		goto exit;
	}

	err = fs_open(&file_dst, path_dst, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
	if (err != 0) {
		shell_error(sh, "Failed to open %s (%d)", path_dst, err);
		err = -EIO;
		goto close_src;
	}

	while (true) {
		buf_len = fs_read(&file_src, buf, BUF_CNT);
		if (buf_len < 0) {
			shell_error(sh, "Failed to read %s (%d)", path_src, (int)buf_len);
			err = -EIO;
			goto close;
		}
		if (buf_len == 0) {
			break;
		}

		num_written = fs_write(&file_dst, buf, buf_len);
		if (num_written < 0) {
			shell_error(sh, "Failed to write %s (%d)", path_dst, (int)num_written);
			err = -EIO;
			goto close;
		}
		if (num_written != buf_len) {
			shell_error(sh, "Failed to write %s", path_dst);
			err = -EIO;
			goto close;
		}
	}

close:
	close_err = fs_close(&file_dst);
	if (close_err != 0) {
		shell_error(sh, "Failed to close %s", path_dst);
		err = -EIO;
	}

close_src:
	close_err = fs_close(&file_src);
	if (close_err != 0) {
		shell_error(sh, "Failed to close %s", path_src);
		err = -EIO;
	}

exit:
	return err;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dirent dirent;
	struct fs_file_t file;
	int count;
	off_t offset;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (argc > 2) {
		count = strtol(argv[2], NULL, 0);
		if (count <= 0) {
			count = INT_MAX;
		}
	} else {
		count = INT_MAX;
	}

	if (argc > 3) {
		offset = strtol(argv[3], NULL, 0);
	} else {
		offset = 0;
	}

	err = fs_stat(path, &dirent);
	if (err != 0) {
		shell_error(sh, "Failed to obtain file %s (err: %d)", path, err);
		return -EIO;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		shell_error(sh, "Not a file %s", path);
		return -EIO;
	}

	shell_print(sh, "File size: %zd", dirent.size);

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_READ);
	if (err != 0) {
		shell_error(sh, "Failed to open %s (%d)", path, err);
		return -EIO;
	}

	if (offset > 0) {
		err = fs_seek(&file, offset, FS_SEEK_SET);
		if (err != 0) {
			shell_error(sh, "Failed to seek %s (%d)", path, err);
			fs_close(&file);
			return -EIO;
		}
	}

	ssize_t read = 0;
	while (count > 0) {
		uint8_t buf[16];
		int i;

		read = fs_read(&file, buf, MIN(count, sizeof(buf)));
		if (read <= 0) {
			break;
		}

		shell_fprintf(sh, SHELL_NORMAL, "%08X  ", (uint32_t)offset);

		for (i = 0; i < read; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%02X ", buf[i]);
		}
		for (; i < sizeof(buf); i++) {
			shell_fprintf(sh, SHELL_NORMAL, "   ");
		}
		i = sizeof(buf) - i;
		shell_fprintf(sh, SHELL_NORMAL, "%*c", i * 3, ' ');

		for (i = 0; i < read; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%c",
				      buf[i] < 32 || buf[i] > 127 ? '.' : buf[i]);
		}

		shell_print(sh, "");

		offset += read;
		count -= read;
	}

	if (read < 0) {
		shell_error(sh, "Failed to read from file %s (err: %zd)", path, read);
	}

	fs_close(&file);

	return 0;
}

static int cmd_cat(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	uint8_t buf[BUF_CNT];
	struct fs_dirent dirent;
	struct fs_file_t file;
	int err;
	ssize_t read;

	fs_file_t_init(&file);

	for (size_t i = 1; i < argc; ++i) {
		create_abs_path(argv[i], path, sizeof(path));

		err = fs_stat(path, &dirent);
		if (err < 0) {
			shell_error(sh, "Failed to obtain file %s (err: %d)", path, err);
			continue;
		}

		if (dirent.type != FS_DIR_ENTRY_FILE) {
			shell_error(sh, "Not a file %s", path);
			continue;
		}

		err = fs_open(&file, path, FS_O_READ);
		if (err < 0) {
			shell_error(sh, "Failed to open %s (%d)", path, err);
			continue;
		}

		while (true) {
			read = fs_read(&file, buf, sizeof(buf));
			if (read <= 0) {
				break;
			}

			for (int j = 0; j < read; j++) {
				shell_fprintf(sh, SHELL_NORMAL, "%c", buf[j]);
			}
		}

		if (read < 0) {
			shell_error(sh, "Failed to read from file %s (err: %zd)", path, read);
		}

		fs_close(&file);
	}

	return 0;
}

static int cmd_statvfs(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	char path[MAX_PATH_LEN];
	struct fs_statvfs stat;

	create_abs_path(argv[1], path, sizeof(path));

	err = fs_statvfs(path, &stat);
	if (err < 0) {
		shell_error(sh, "Failed to statvfs %s (%d)", path, err);
		return -EIO;
	}

	shell_fprintf(sh, SHELL_NORMAL, "bsize %lu, frsize %lu, blocks %lu, bfree %lu\n",
		      stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);

	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	uint8_t buf[BUF_CNT];
	uint8_t buf_len;
	int arg_offset;
	struct fs_file_t file;
	off_t offset = -1;
	int err;

	create_abs_path(argv[1], path, sizeof(path));

	if (!strcmp(argv[2], "-o")) {
		if (argc < 4) {
			shell_error(sh, "Missing argument");
			return -EINVAL;
		}

		offset = strtol(argv[3], NULL, 0);

		arg_offset = 4;
	} else {
		arg_offset = 2;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (err != 0) {
		shell_error(sh, "Failed to open %s (%d)", path, err);
		return -EIO;
	}

	if (offset < 0) {
		err = fs_seek(&file, 0, FS_SEEK_END);
	} else {
		err = fs_seek(&file, offset, FS_SEEK_SET);
	}
	if (err != 0) {
		shell_error(sh, "Failed to seek %s (%d)", path, err);
		fs_close(&file);
		return -EIO;
	}

	buf_len = 0U;
	while (arg_offset < argc) {
		buf[buf_len++] = strtol(argv[arg_offset++], NULL, 16);

		if ((buf_len == BUF_CNT) || (arg_offset == argc)) {
			err = fs_write(&file, buf, buf_len);
			if (err < 0) {
				shell_error(sh, "Failed to write %s (%d)", path, err);
				fs_close(&file);
				return -EIO;
			}

			buf_len = 0U;
		}
	}

	fs_close(&file);

	return 0;
}

#ifdef CONFIG_FILE_SYSTEM_SHELL_TEST_COMMANDS
const static uint8_t speed_types[][4] = {"B", "KiB", "MiB", "GiB"};
const static uint32_t speed_divisor = 1024;

static void file_size_output(const struct shell *sh, double size)
{
	uint8_t speed_index = 0;

	while (size >= (double)speed_divisor && speed_index < ARRAY_SIZE(speed_types)) {
		size /= (double)speed_divisor;
		++speed_index;
	}

	shell_print(sh, "File size: %.1f%s", size, speed_types[speed_index]);
}

static void speed_output(const struct shell *sh, uint64_t total_time, double loops, double size)
{
	double time_per_loop = (double)total_time / loops;
	double throughput = size;
	uint8_t speed_index = 0;

	if (time_per_loop > 0) {
		throughput /= (time_per_loop / 1000.0);
	}

	while (throughput >= (double)speed_divisor && speed_index < ARRAY_SIZE(speed_types)) {
		throughput /= (double)speed_divisor;
		++speed_index;
	}

	shell_print(sh, "Total: %llums, Per loop: ~%.0fms, Speed: ~%.1f%sps", total_time,
		    time_per_loop, throughput, speed_types[speed_index]);
}

static int cmd_read_test(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_dirent dirent;
	struct fs_file_t file;
	int err;
	uint32_t repeat;
	uint8_t random_data[CONFIG_FILE_SYSTEM_SHELL_BUFFER_SIZE];
	uint32_t i;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;
	uint32_t size;

	if (argc < 3) {
		shell_error(sh, "Missing parameters: read_test <path> <repeat>");
		return -EINVAL;
	}

	create_abs_path(argv[1], path, sizeof(path));
	repeat = strtol(argv[2], NULL, 0);

	if (repeat == 0 || repeat > 10) {
		shell_error(sh, "<repeat> must be between 1 and 10.");
		return -EINVAL;
	}

	err = fs_stat(path, &dirent);

	if (err != 0) {
		shell_error(sh, "File status failed: %d", err);
		return err;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		shell_error(sh, "Provided path is not a file");
		return -ENOENT;
	}

	/* Store size of file so we can ensure it was fully read */
	size = dirent.size;
	file_size_output(sh, (double)size);

	while (loops < repeat) {
		start_time = k_uptime_get();

		fs_file_t_init(&file);
		err = fs_open(&file, path, FS_O_READ);
		if (err != 0) {
			shell_error(sh, "Failed to open %s (%d)", path, err);
			return -EIO;
		}

		/* Read data in chunk by chunk until the full size has been read */
		i = 0;
		while (1) {
			err = fs_read(&file, random_data, sizeof(random_data));
			if (err < 0) {
				shell_error(sh, "Failed to read %s (%d)", path, err);
				fs_close(&file);
				return -EIO;
			}

			i += err;

			if (err == 0) {
				/* Read finished */
				break;
			}
		}

		/* Ensure file contents is fully read then close file */
		fs_close(&file);

		if (i != size) {
			shell_error(sh, "File read error, expected %d bytes but only read %d", size,
				    i);
			return -EIO;
		}

		++loops;
		loop_time = k_uptime_delta(&start_time);
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llums.", loops, loop_time);
	}

	speed_output(sh, total_time, (double)loops, (double)size);

	return 0;
}

static int cmd_erase_write_test(const struct shell *sh, size_t argc, char **argv)
{
	char path[MAX_PATH_LEN];
	struct fs_file_t file;
	int err;
	uint32_t size;
	uint32_t repeat;
	uint8_t random_data[CONFIG_FILE_SYSTEM_SHELL_BUFFER_SIZE];
	uint32_t i;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;

	if (argc < 4) {
		shell_error(sh, "Missing parameters: erase_write_test <path> <size> <repeat>");
		return -EINVAL;
	}

	create_abs_path(argv[1], path, sizeof(path));
	size = strtol(argv[2], NULL, 0);
	repeat = strtol(argv[3], NULL, 0);

	if (size == 0) {
		shell_error(sh, "<size> must be at least 1.");
		return -EINVAL;
	}

	if (repeat == 0 || repeat > 10) {
		shell_error(sh, "<repeat> must be between 1 and 10.");
		return -EINVAL;
	}

	/* Generate random data, the contents is not important */
	i = 0;
	while (i < sizeof(random_data)) {
		random_data[i] = (uint8_t)(i % 255);
		++i;
	}

	while (loops < repeat) {
		start_time = k_uptime_get();

		fs_file_t_init(&file);
		err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
		if (err != 0) {
			shell_error(sh, "Failed to open %s (%d)", path, err);
			return -EIO;
		}

		/* Truncate the file size to 0 (if supported, erase if not) */
		err = fs_truncate(&file, 0);

		if (err == -ENOTSUP) {
			fs_close(&file);

			err = fs_unlink(path);
			if (err != 0) {
				shell_error(sh, "Failed to delete %s (%d)", path, err);
				return -EIO;
			}

			err = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
			if (err != 0) {
				shell_error(sh, "Failed to open %s (%d)", path, err);
				return -EIO;
			}
		} else if (err != 0) {
			shell_error(sh, "Failed to truncate %s (%d)", path, err);
			fs_close(&file);
			return -EIO;
		}

		/* Write data out chunk by chunk until the full size has been written */
		i = 0;
		while (i < size) {
			uint32_t write_size = size - i;

			if (write_size > sizeof(random_data)) {
				write_size = sizeof(random_data);
			}

			err = fs_write(&file, random_data, write_size);
			if (err < 0) {
				shell_error(sh, "Failed to write %s (%d)", path, err);
				fs_close(&file);
				return -EIO;
			}

			i += write_size;
		}

		/* Ensure file contents is fully written then close file */
		fs_sync(&file);
		fs_close(&file);

		++loops;
		loop_time = k_uptime_delta(&start_time);
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llums.", loops, loop_time);
	}

	speed_output(sh, total_time, (double)loops, (double)size);

	return 0;
}
#endif

#ifdef CONFIG_FILE_SYSTEM_SHELL_MOUNT_COMMAND

static char *mntpt_prepare(char *mntpt)
{
	char *cpy_mntpt;

	cpy_mntpt = k_malloc(strlen(mntpt) + 1);
	if (cpy_mntpt != NULL) {
		strcpy(cpy_mntpt, mntpt);
	}
	return cpy_mntpt;
}

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static int cmd_mount_fat(const struct shell *sh, size_t argc, char **argv)
{
	char *mntpt;
	int res;

	mntpt = mntpt_prepare(argv[1]);
	if (mntpt == NULL) {
		shell_error(sh, "Failed to allocate buffer for mount point");
		return -EIO;
	}

	fatfs_mnt.mnt_point = (const char *)mntpt;
	res = fs_mount(&fatfs_mnt);
	if (res != 0) {
		shell_error(sh, "Error mounting FAT fs. Error Code [%d]", res);
		k_free((void *)fatfs_mnt.mnt_point);
		fatfs_mnt.mnt_point = NULL;
		return -EIO;
	}

	shell_print(sh, "Successfully mounted fat fs:%s", fatfs_mnt.mnt_point);

	return 0;
}
#endif

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
static int cmd_mount_littlefs(const struct shell *sh, size_t argc, char **argv)
{
	if (littlefs_mnt.mnt_point != NULL) {
		return -EBUSY;
	}

	char *mntpt = mntpt_prepare(argv[1]);

	if (mntpt == NULL) {
		shell_error(sh, "Failed to allocate mount point");
		return -EIO;
	}

	littlefs_mnt.mnt_point = mntpt;

	int rc = fs_mount(&littlefs_mnt);

	if (rc != 0) {
		shell_error(sh, "Error mounting as littlefs: %d", rc);
		k_free((void *)littlefs_mnt.mnt_point);
		littlefs_mnt.mnt_point = NULL;
		return -EIO;
	}

	return rc;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fs_mount,
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	SHELL_CMD_ARG(fat, NULL,
		      "Mount fatfs. fs mount fat <mount-point>",
		      cmd_mount_fat, 2, 0),
#endif

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
	SHELL_CMD_ARG(littlefs, NULL,
		      "Mount littlefs. fs mount littlefs <mount-point>",
		      cmd_mount_littlefs, 2, 0),
#endif

	SHELL_SUBCMD_SET_END
);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fs,
	SHELL_CMD(cd, NULL, "Change working directory", cmd_cd),
	SHELL_CMD(ls, NULL, "List files in current directory", cmd_ls),
	SHELL_CMD_ARG(mkdir, NULL, "Create directory", cmd_mkdir, 2, 0),
#ifdef CONFIG_FILE_SYSTEM_SHELL_MOUNT_COMMAND
	SHELL_CMD(mount, &sub_fs_mount,
		  "<Mount fs, syntax:- fs mount <fs type> <mount-point>", NULL),
#endif
	SHELL_CMD(pwd, NULL, "Print current working directory", cmd_pwd),
	SHELL_CMD_ARG(read, NULL, "Read from file", cmd_read, 2, 255),
	SHELL_CMD_ARG(cat, NULL,
		"Concatenate files and print on the standard output",
		cmd_cat, 2, 255),
	SHELL_CMD_ARG(rm, NULL, "Remove file", cmd_rm, 2, 0),
	SHELL_CMD_ARG(cp, NULL, "Copy file", cmd_cp, 3, 0),
	SHELL_CMD_ARG(statvfs, NULL, "Show file system state", cmd_statvfs, 2, 0),
	SHELL_CMD_ARG(trunc, NULL, "Truncate file", cmd_trunc, 2, 255),
	SHELL_CMD_ARG(write, NULL, "Write file", cmd_write, 3, 255),
#ifdef CONFIG_FILE_SYSTEM_SHELL_TEST_COMMANDS
	SHELL_CMD_ARG(read_test, NULL, "Read file test", cmd_read_test, 2, 2),
	SHELL_CMD_ARG(erase_write_test, NULL, "Erase/write file test", cmd_erase_write_test, 3, 3),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(fs, &sub_fs, "File system commands", NULL);
