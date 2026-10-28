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
	"CANNON_SUSFS_BUILTIN_PARTIAL\n";

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

int ksu_susfs_handle_supercall(unsigned int cmd, void __user **arg)
{
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
	default:
		/* 未实现的命令不写 err，userspace 会保持 ERR_CMD_NOT_SUPPORTED(126) */
		return 0;
	}
}
