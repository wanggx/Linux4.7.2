#ifndef _LINUX_PID_NS_H
#define _LINUX_PID_NS_H

#include <linux/sched.h>
#include <linux/bug.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <linux/threads.h>
#include <linux/nsproxy.h>
#include <linux/kref.h>
#include <linux/ns_common.h>

/* 和2.6.11结构相同 */
struct pidmap {
       atomic_t nr_free;
       void *page;
};

#define BITS_PER_PAGE		(PAGE_SIZE * 8)
#define BITS_PER_PAGE_MASK	(BITS_PER_PAGE-1)
#define PIDMAP_ENTRIES		((PID_MAX_LIMIT+BITS_PER_PAGE-1)/BITS_PER_PAGE)

struct fs_pin;

/* pid名称空间 */
struct pid_namespace {
	struct kref kref;
	struct pidmap pidmap[PIDMAP_ENTRIES];
        /* 形成名称空间的RCU链表 */
	struct rcu_head rcu;
	int last_pid;		/* 空间当中最后一个分配的pid */
	/* 名称空间当中添加到hash链表当中upid的个数 */
	unsigned int nr_hashed;
	/* 该进程的作用是子进程结束时为其收尸，
	  * 由于目前只支持global namespace，所以指向init_task 
	  */
	struct task_struct *child_reaper;
	/* 分配struct pid的内存缓冲， */
	struct kmem_cache *pid_cachep;		
	unsigned int level;		/* 表示名称空间处在第几层 */
	struct pid_namespace *parent;			/* 父名称空间 */
#ifdef CONFIG_PROC_FS
	struct vfsmount *proc_mnt;
	struct dentry *proc_self;
	struct dentry *proc_thread_self;
#endif
#ifdef CONFIG_BSD_PROCESS_ACCT
	struct fs_pin *bacct;
#endif
	struct user_namespace *user_ns;		/* 用户名称空间 */
	struct work_struct proc_work;
	kgid_t pid_gid;
	int hide_pid;
	int reboot;	/* group exit code if this pidns was rebooted */
	struct ns_common ns;
};

extern struct pid_namespace init_pid_ns;

#define PIDNS_HASH_ADDING (1U << 31)

#ifdef CONFIG_PID_NS
static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	if (ns != &init_pid_ns)
		kref_get(&ns->kref);
	return ns;
}

extern struct pid_namespace *copy_pid_ns(unsigned long flags,
	struct user_namespace *user_ns, struct pid_namespace *ns);
extern void zap_pid_ns_processes(struct pid_namespace *pid_ns);
extern int reboot_pid_ns(struct pid_namespace *pid_ns, int cmd);
extern void put_pid_ns(struct pid_namespace *ns);

#else /* !CONFIG_PID_NS */
#include <linux/err.h>

static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	return ns;
}

static inline struct pid_namespace *copy_pid_ns(unsigned long flags,
	struct user_namespace *user_ns, struct pid_namespace *ns)
{
	if (flags & CLONE_NEWPID)
		ns = ERR_PTR(-EINVAL);
	return ns;
}

static inline void put_pid_ns(struct pid_namespace *ns)
{
}

static inline void zap_pid_ns_processes(struct pid_namespace *ns)
{
	BUG();
}

static inline int reboot_pid_ns(struct pid_namespace *pid_ns, int cmd)
{
	return 0;
}
#endif /* CONFIG_PID_NS */

extern struct pid_namespace *task_active_pid_ns(struct task_struct *tsk);
void pidhash_init(void);
void pidmap_init(void);

#endif /* _LINUX_PID_NS_H */
