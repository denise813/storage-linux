/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PATH_H
#define _LINUX_PATH_H

struct dentry;
struct vfsmount;

struct path {
	struct vfsmount *mnt; /*指向文件系统装载点的指针*/
	struct dentry *dentry;  /*指向目录项对应的指针*/
} __randomize_layout;

extern void path_get(const struct path *);
extern void path_put(const struct path *);

static inline int path_equal(const struct path *path1, const struct path *path2)
{
	return path1->mnt == path2->mnt && path1->dentry == path2->dentry;
}

static inline void path_put_init(struct path *path)
{
	path_put(path);
	*path = (struct path) { };
}

#endif  /* _LINUX_PATH_H */
