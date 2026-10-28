// SPDX-License-Identifier: GPL-2.0
/*
 * cannon: SUSFS 新协议超调用入口（第一阶段：只回 SHOW_* 三个查询命令）。
 *
 * 上游 KernelSU-Next v3.4.2 把 SUSFS 做成了独立的 kernel/susfs/ 模块
 * （~15k 行，依赖 kprobe、patch_memory、symbol_resolver），这套东西在
 * 4.14 上跑不起来；而本地 next-susfs 分支里的 4.14 移植用的是旧的
 * prctl 协议，v3.4.2 的 ksud 已经不认了。
 *
 * 所以这里先只把「管理器首页那行 SuSFS 版本」需要的最小 ABI 补上，
 * 隐藏功能（sus_path / sus_mount / ...）留到第二阶段。
 *
 * 注意：features 里刻意不写 CONFIG_KSU_SUSFS_* 名字，避免让管理器
 * 以为隐藏功能已经可用；用一个非 CONFIG 的标记行表明这是本地裁剪版。
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "susfs_abi.h"

#define SUSFS_VERSION_STR	"v2.3.0"
#define SUSFS_VARIANT_STR	"Builtin-4.14"

static const char susfs_feature_list[] =
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	"CONFIG_KSU_SUSFS_SUS_PATH\n"
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	"CONFIG_KSU_SUSFS_SUS_KSTAT\n"
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	"CONFIG_KSU_SUSFS_SUS_MOUNT\n"
#endif
	;

static void susfs_show_version(void __user **arg)
{
	struct st_susfs_version info = { 0 };

	if (copy_from_user(&info, (void __user *)*arg, sizeof(info))) {
		info.err = -EFAULT;
		goto out;
	}
	strscpy(info.susfs_version, SUSFS_VERSION_STR,
		SUSFS_MAX_VERSION_BUFSIZE);
	info.err = 0;
out:
	if (copy_to_user((void __user *)*arg, &info, sizeof(info)))
		pr_warn("susfs: show_version copy_to_user failed\n");
}

static void susfs_show_variant(void __user **arg)
{
	struct st_susfs_variant info = { 0 };

	if (copy_from_user(&info, (void __user *)*arg, sizeof(info))) {
		info.err = -EFAULT;
		goto out;
	}
	strscpy(info.susfs_variant, SUSFS_VARIANT_STR,
		SUSFS_MAX_VARIANT_BUFSIZE);
	info.err = 0;
out:
	if (copy_to_user((void __user *)*arg, &info, sizeof(info)))
		pr_warn("susfs: show_variant copy_to_user failed\n");
}

static void susfs_show_features(void __user **arg)
{
	struct st_susfs_enabled_features *info;
	size_t len = sizeof(susfs_feature_list) - 1;

	/* 8192 字节的结构体不能放栈上（4.14 栈只有 16K） */
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_warn("susfs: show_features kzalloc failed\n");
		return;
	}

	if (copy_from_user(info, (void __user *)*arg, sizeof(*info))) {
		info->err = -EFAULT;
		goto out;
	}
	if (len >= SUSFS_ENABLED_FEATURES_SIZE) {
		info->err = -EINVAL;
		goto out;
	}
	memcpy(info->enabled_features, susfs_feature_list, len);
	info->enabled_features[len] = '\0';
	info->err = 0;
out:
	if (copy_to_user((void __user *)*arg, info, sizeof(*info)))
		pr_warn("susfs: show_features copy_to_user failed\n");
	kfree(info);
}

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
/*
 * fs/susfs.c 里的入口。原型在 include/linux/susfs.h，但那个头里也定义了
 * struct st_susfs_sus_path，和本目录 susfs_abi.h 的同名结构会撞车；
 * 两处布局一致（char[256] + int err），这里用 void __user * 声明绕开。
 */
extern int susfs_add_sus_path(void __user *user_info);
#endif

#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
extern int susfs_add_sus_kstat(void __user *user_info, int is_statically);
#endif

int ksu_susfs_handle_supercall(unsigned int cmd, void __user **arg)
{
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	/* 布局和 userspace 的 #[repr(C)] 对不上就直接编译失败，别等到运行时 */
	BUILD_BUG_ON(sizeof(struct st_susfs_sus_kstat) != 376);
	BUILD_BUG_ON(offsetof(struct st_susfs_sus_kstat, err) != 372);
#endif
	switch (cmd) {
	case CMD_SUSFS_SHOW_VERSION:
		susfs_show_version(arg);
		return 0;
	case CMD_SUSFS_SHOW_VARIANT:
		susfs_show_variant(arg);
		return 0;
	case CMD_SUSFS_SHOW_ENABLED_FEATURES:
		susfs_show_features(arg);
		return 0;
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	case CMD_SUSFS_ADD_SUS_PATH:
	case CMD_SUSFS_ADD_SUS_PATH_LOOP:
		/* err 由 fs/susfs.c 直接写回 userspace */
		susfs_add_sus_path((void __user *)*arg);
		return 0;
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	case CMD_SUSFS_ADD_SUS_KSTAT:
		susfs_add_sus_kstat((void __user *)*arg, 0);
		return 0;
	case CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY:
		susfs_add_sus_kstat((void __user *)*arg, 1);
		return 0;
	case CMD_SUSFS_UPDATE_SUS_KSTAT:
		/* 我们的实现每次都按路径重解析 ino，UPDATE 与 ADD 等价 */
		susfs_add_sus_kstat((void __user *)*arg, 0);
		return 0;
#endif
	default:
		/* 未实现的命令不写 err，userspace 会保持 ERR_CMD_NOT_SUPPORTED(126) */
		return 0;
	}
}
