/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cannon: SUSFS 新协议 ABI（reboot 超调用）的最小实现。
 * 与 KernelSU-Next v3.4.2 的 kernel/susfs/susfs_abi.h 保持逐字节一致。
 * 协议：syscall(SYS_reboot, 0xDEADBEEF, 0xFAFAFAFA, cmd, &payload)
 * 内核只写回 payload.err（0 成功，其余 errno 风格）。
 */
#ifndef __CANNON_SUSFS_ABI_H
#define __CANNON_SUSFS_ABI_H

#define KSU_INSTALL_MAGIC1_VAL	0xDEADBEEF
#define SUSFS_MAGIC		0xFAFAFAFA

#define CMD_SUSFS_ADD_SUS_PATH			0x55550
#define CMD_SUSFS_ADD_SUS_PATH_LOOP		0x55553
#define CMD_SUSFS_ADD_SUS_KSTAT			0x55570
#define CMD_SUSFS_UPDATE_SUS_KSTAT		0x55571
#define CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY	0x55572
#define CMD_SUSFS_SHOW_VERSION			0x555e1
#define CMD_SUSFS_SHOW_ENABLED_FEATURES		0x555e2
#define CMD_SUSFS_SHOW_VARIANT			0x555e3

#define SUSFS_MAX_LEN_PATHNAME			256
#define SUSFS_MAX_VERSION_BUFSIZE		16
#define SUSFS_MAX_VARIANT_BUFSIZE		16
#define SUSFS_ENABLED_FEATURES_SIZE		8192

/* 与 fs/susfs.c 的 include/linux/susfs.h 同名结构布局必须一致 */
struct st_susfs_sus_path {
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int err;
};

/* 见 include/linux/susfs.h 里同名的 KSTAT_SPOOF_* 定义 */
#define KSTAT_SPOOF_INO           (1 << 0)
#define KSTAT_SPOOF_DEV           (1 << 1)
#define KSTAT_SPOOF_NLINK         (1 << 2)
#define KSTAT_SPOOF_SIZE          (1 << 3)
#define KSTAT_SPOOF_ATIME_TV_SEC  (1 << 4)
#define KSTAT_SPOOF_ATIME_TV_NSEC (1 << 5)
#define KSTAT_SPOOF_MTIME_TV_SEC  (1 << 6)
#define KSTAT_SPOOF_MTIME_TV_NSEC (1 << 7)
#define KSTAT_SPOOF_CTIME_TV_SEC  (1 << 8)
#define KSTAT_SPOOF_CTIME_TV_NSEC (1 << 9)
#define KSTAT_SPOOF_BLOCKS        (1 << 10)
#define KSTAT_SPOOF_BLKSIZE       (1 << 11)
#define KSTAT_SPOOF_ALL           0xfff

struct st_susfs_sus_kstat {
	int is_statically;
	unsigned long target_ino;
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long spoofed_ino;
	unsigned long spoofed_dev;
	unsigned int spoofed_nlink;
	long long spoofed_size;
	long spoofed_atime_tv_sec;
	unsigned long spoofed_atime_tv_nsec;
	long spoofed_mtime_tv_sec;
	unsigned long spoofed_mtime_tv_nsec;
	long spoofed_ctime_tv_sec;
	unsigned long spoofed_ctime_tv_nsec;
	long long spoofed_blocks;
	long spoofed_blksize;
	int flags;
	int err;
};

struct st_susfs_version {
	char susfs_version[SUSFS_MAX_VERSION_BUFSIZE];
	int err;
};

struct st_susfs_variant {
	char susfs_variant[SUSFS_MAX_VARIANT_BUFSIZE];
	int err;
};

struct st_susfs_enabled_features {
	char enabled_features[SUSFS_ENABLED_FEATURES_SIZE];
	int err;
};

#endif /* __CANNON_SUSFS_ABI_H */
