/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SCSI_SCSI_HOST_H
#define _SCSI_SCSI_HOST_H

#include <linux/device.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/blk-mq.h>
#include <scsi/scsi.h>

struct block_device;
struct completion;
struct module;
struct scsi_cmnd;
struct scsi_device;
struct scsi_host_cmd_pool;
struct scsi_target;
struct Scsi_Host;
struct scsi_host_cmd_pool;
struct scsi_transport_template;


#define SG_ALL	SG_CHUNK_SIZE

#define MODE_UNKNOWN 0x00
#define MODE_INITIATOR 0x01
#define MODE_TARGET 0x02

/** comment by hy 2019-10-13
 * # SCSI 主机适配器模板
 */
struct scsi_host_template {
	struct module *module;  /*指向实现了这个 host 模板的低层驱动模块的指针*/
	const char *name; //scsi hba驱动的名字
	/*
	 * The info function will return whatever useful information the
	 * developer sees fit.  If not provided, then the name field will
	 * be used instead.
	 *
	 * Status: OPTIONAL
	 */
	const char *(* info)(struct Scsi_Host *); /*返回开发人员觉得合适的任何有用信息
	      如果未给定.则使用名字域状态,可选*/

	/*
	 * Ioctl interface
	 *
	 * Status: OPTIONAL
	 */
	int (*ioctl)(struct scsi_device *dev, unsigned int cmd,
		     void __user *arg); /*ioctl接口*/


#ifdef CONFIG_COMPAT
	/* 
	 * Compat handler. Handle 32bit ABI.
	 * When unknown ioctl is passed return -ENOIOCTLCMD.
	 *
	 * Status: OPTIONAL
	 */
	int (*compat_ioctl)(struct scsi_device *dev, unsigned int cmd,
			    void __user *arg);
#endif

	int (*init_cmd_priv)(struct Scsi_Host *shost, struct scsi_cmnd *cmd);
	int (*exit_cmd_priv)(struct Scsi_Host *shost, struct scsi_cmnd *cmd);

	/*
	 * The queuecommand function is used to queue up a scsi
	 * command block to the LLDD.  When the driver finished
	 * processing the command the done callback is invoked.
	 *
	 * If queuecommand returns 0, then the driver has accepted the
	 * command.  It must also push it to the HBA if the scsi_cmnd
	 * flag SCMD_LAST is set, or if the driver does not implement
	 * commit_rqs.  The done() function must be called on the command
	 * when the driver has finished with it. (you may call done on the
	 * command before queuecommand returns, but in this case you
	 * *must* return 0 from queuecommand).
	 *
	 * Queuecommand may also reject the command, in which case it may
	 * not touch the command and must not call done() for it.
	 *
	 * There are two possible rejection returns:
	 *
	 *   SCSI_MLQUEUE_DEVICE_BUSY: Block this device temporarily, but
	 *   allow commands to other devices serviced by this host.
	 *
	 *   SCSI_MLQUEUE_HOST_BUSY: Block all devices served by this
	 *   host temporarily.
	 *
         * For compatibility, any other non-zero return is treated the
         * same as SCSI_MLQUEUE_HOST_BUSY.
	 *
	 * NOTE: "temporarily" means either until the next command for#
	 * this device/host completes, or a period of time determined by
	 * I/O pressure in the system if there are no other outstanding
	 * commands.
	 *
	 * STATUS: REQUIRED
	 */
/** comment by hy 2019-10-09
 * # 将SCSI命令排入底层驱动的队列,SCSI 中间层调用该回调函数向HBA发送SCSI命令
 */
	int (* queuecommand)(struct Scsi_Host *, struct scsi_cmnd *);

	/*
	 * The commit_rqs function is used to trigger a hardware
	 * doorbell after some requests have been queued with
	 * queuecommand, when an error is encountered before sending
	 * the request with SCMD_LAST set.
	 *
	 * STATUS: OPTIONAL
	 */
	void (*commit_rqs)(struct Scsi_Host *, u16);

	/*
	 * This is an error handling strategy routine.  You don't need to
	 * define one of these if you don't want to - there is a default
	 * routine that is present that should work in most cases.  For those
	 * driver authors that have the inclination and ability to write their
	 * own strategy routine, this is where it is specified.  Note - the
	 * strategy routine is *ALWAYS* run in the context of the kernel eh
	 * thread.  Thus you are guaranteed to *NOT* be in an interrupt
	 * handler when you execute this, and you are also guaranteed to
	 * *NOT* have any other commands being queued while you are in the
	 * strategy routine. When you return from this function, operations
	 * return to normal.
	 *
	 * See scsi_error.c scsi_unjam_host for additional comments about
	 * what this function should and should not be attempting to do.
	 *
	 * Status: REQUIRED	(at least one of them)
	 */
/** comment by hy 2019-10-09
 * # 错误恢复动作:放弃给定的命令
 */
	int (* eh_abort_handler)(struct scsi_cmnd *);
/** comment by hy 2019-10-13
 * # 错误恢复动作 SCSI 设备复位
 */
	int (* eh_device_reset_handler)(struct scsi_cmnd *);  /* scsi设备复位 */
/** comment by hy 2019-10-13
 * # 错误恢复动作：目标节点复位
 */
	int (* eh_target_reset_handler)(struct scsi_cmnd *);  /* 目标节点复位 */
/** comment by hy 2019-10-13
 * # 错误恢复动作 SCSI 总线复位
 */
	int (* eh_bus_reset_handler)(struct scsi_cmnd *);     /* scsi总线复位 */
/** comment by hy 2019-10-13
 * # 错误恢复动作: 主机适配器复位
 */
	int (* eh_host_reset_handler)(struct scsi_cmnd *);    /* 主机适配器复位 */

	/*
	 * Before the mid layer attempts to scan for a new device where none
	 * currently exists, it will call this entry in your driver.  Should
	 * your driver need to allocate any structs or perform any other init
	 * items in order to send commands to a currently unused target/lun
	 * combo, then this is where you can perform those allocations.  This
	 * is specifically so that drivers won't have to perform any kind of
	 * "is this a new device" checks in their queuecommand routine,
	 * thereby making the hot path a bit quicker.
	 *
	 * Return values: 0 on success, non-0 on failure
	 *
	 * Deallocation:  If we didn't find any devices at this ID, you will
	 * get an immediate call to slave_destroy().  If we find something
	 * here then you will get a call to slave_configure(), then the
	 * device will be used for however long it is kept around, then when
	 * the device is removed from the system (or * possibly at reboot
	 * time), you will then get a call to slave_destroy().  This is
	 * assuming you implement slave_configure and slave_destroy.
	 * However, if you allocate memory and hang it off the device struct,
	 * then you must implement the slave_destroy() routine at a minimum
	 * in order to avoid leaking memory
	 * each time a device is tore down.
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 在扫描到一个新的 SCSI 设备后调用，用户可以在这个函数中为其分配私有 device 结构
 */
	int (* slave_alloc)(struct scsi_device *);  /* 在扫描到一个新的SCSI设备后调用 */

	/*
	 * Once the device has responded to an INQUIRY and we know the
	 * device is online, we call into the low level driver with the
	 * struct scsi_device *.  If the low level device driver implements
	 * this function, it *must* perform the task of setting the queue
	 * depth on the device.  All other tasks are optional and depend
	 * on what the driver supports and various implementation details.
	 * 
	 * Things currently recommended to be handled at this time include:
	 *
	 * 1.  Setting the device queue depth.  Proper setting of this is
	 *     described in the comments for scsi_change_queue_depth.
	 * 2.  Determining if the device supports the various synchronous
	 *     negotiation protocols.  The device struct will already have
	 *     responded to INQUIRY and the results of the standard items
	 *     will have been shoved into the various device flag bits, eg.
	 *     device->sdtr will be true if the device supports SDTR messages.
	 * 3.  Allocating command structs that the device will need.
	 * 4.  Setting the default timeout on this device (if needed).
	 * 5.  Anything else the low level driver might want to do on a device
	 *     specific setup basis...
	 * 6.  Return 0 on success, non-0 on error.  The device will be marked
	 *     as offline on error so that no access will occur.  If you return
	 *     non-0, your slave_destroy routine will never get called for this
	 *     device, so don't leave any loose memory hanging around, clean
	 *     up after yourself before returning non-0
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 在接收到 SCSI 设备的 INQUIRY 响应之后调用，用户可以在这个函数逬行特定的设置
 */
	int (* slave_configure)(struct scsi_device *); /* 在接收到SCSI设备的inquiry响应之后调用 */

	/*
	 * Immediately prior to deallocating the device and after all activity
	 * has ceased the mid layer calls this point so that the low level
	 * driver may completely detach itself from the scsi device and vice
	 * versa.  The low level driver is responsible for freeing any memory
	 * it allocated in the slave_alloc or slave_configure calls. 
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 在销毁 SCSI 设备之前被调用,SCSi 子系统调用这个函数,
     释放关联的私有 device 结构
 */
	void (* slave_destroy)(struct scsi_device *); /* 在销毁scsi设备之前调用 */

	/*
	 * Before the mid layer attempts to scan for a new device attached
	 * to a target where no target currently exists, it will call this
	 * entry in your driver.  Should your driver need to allocate any
	 * structs or perform any other init items in order to send commands
	 * to a currently unused target, then this is where you can perform
	 * those allocations.
	 *
	 * Return values: 0 on success, non-0 on failure
	 *
	 * Status: OPTIONAL
	 */
  /** comment by hy 2019-10-13
   * # 在发现一个新的 SCSI 目标器后调用， 用户可以在这个函数中分配私有结构
   */
	int (* target_alloc)(struct scsi_target *); /* 在发现一个新的scsi目标器后调用 */

	/*
	 * Immediately prior to deallocating the target structure, and
	 * after all activity to attached scsi devices has ceased, the
	 * midlayer calls this point so that the driver may deallocate
	 * and terminate any references to the target.
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 在销毁 SCSI 目标器之前被调用 SCSI 子系统调用这个函数，
     释放管理的私有 target 结构
 */
	void (* target_destroy)(struct scsi_target *);  /* 在销毁scsi目标器后调用 */

	/*
	 * If a host has the ability to discover targets on its own instead
	 * of scanning the entire bus, it can fill in this function and
	 * call scsi_scan_host().  This function will be called periodically
	 * until it returns 1 with the scsi_host and the elapsed time of
	 * the scan in jiffies.
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 如果主机适配器定义了自己的妇描逻辑， 则需要实现达个回调函数
 */
	int (* scan_finished)(struct Scsi_Host *, unsigned long);

	/*
	 * If the host wants to be called before the scan starts, but
	 * after the midlayer has set up ready for the scan, it can fill
	 * in this function.
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 如果主机适配器定义了自己的扫描邂辑,则需要实现这个回调
 */
	void (* scan_start)(struct Scsi_Host *);

	/*
	 * Fill in this function to allow the queue depth of this host
	 * to be changeable (on a per device basis).  Returns either
	 * the current queue depth setting (may be different from what
	 * was passed in) or an error.  An error should only be
	 * returned if the requested depth is legal but the driver was
	 * unable to set it.  If the requested depth is illegal, the
	 * driver should set and return the closest legal queue depth.
	 *
	 * Status: OPTIONAL
	 */
	int (* change_queue_depth)(struct scsi_device *, int);  /* 改变主机适配器队列深度的回调函数 */

	/*
	 * This functions lets the driver expose the queue mapping
	 * to the block layer.
	 *
	 * Status: OPTIONAL
	 */
	int (* map_queues)(struct Scsi_Host *shost);

	/*
	 * Check if scatterlists need to be padded for DMA draining.
	 *
	 * Status: OPTIONAL
	 */
	bool (* dma_need_drain)(struct request *rq);

	/*
	 * This function determines the BIOS parameters for a given
	 * harddisk.  These tend to be numbers that are made up by
	 * the host adapter.  Parameters:
	 * size, device, list (heads, sectors, cylinders)
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 返回磁盘的 BIOS 参数: 柱面数、 磁遒数和廟区数等
 */
	int (* bios_param)(struct scsi_device *, struct block_device *,
			sector_t, int []);

	/*
	 * This function is called when one or more partitions on the
	 * device reach beyond the end of the device.
	 *
	 * Status: OPTIONAL
	 */
	void (*unlock_native_capacity)(struct scsi_device *);

	/*
	 * Can be used to export driver statistics and other infos to the
	 * world outside the kernel ie. userspace and it also provides an
	 * interface to feed the driver with information.
	 *
	 * Status: OBSOLETE
	 */
	int (*show_info)(struct seq_file *, struct Scsi_Host *);
	int (*write_info)(struct Scsi_Host *, char *, int);

	/*
	 * This is an optional routine that allows the transport to become
	 * involved when a scsi io timer fires. The return value tells the
	 * timer routine how to finish the io timeout handling.
	 *
	 * Status: OPTIONAL
	 */
/** comment by hy 2019-10-13
 * # 在中间层发现 scsi 命令超时,低层驱动调用这个回调,
     使低层有机会修正错误， 正常究成命令  或
     延长超时时间.重新计时 或进入错误恢复
 */
	enum blk_eh_timer_return (*eh_timed_out)(struct scsi_cmnd *);

	/* This is an optional routine that allows transport to initiate
	 * LLD adapter or firmware reset using sysfs attribute.
	 *
	 * Return values: 0 on success, -ve value on failure.
	 *
	 * Status: OPTIONAL
	 */

	int (*host_reset)(struct Scsi_Host *shost, int reset_type);
#define SCSI_ADAPTER_RESET	1
#define SCSI_FIRMWARE_RESET	2


	/*
	 * Name of proc directory
	 */
	const char *proc_name;  /*proc 目录名*/

	/*
	 * Used to store the procfs directory if a driver implements the
	 * show_info method.
	 */
	struct proc_dir_entry *proc_dir;  /*如果驱动实现了 procjnfo 方法 可以用来保存 proefs 目录*/

	/*
	 * This determines if we will use a non-interrupt driven
	 * or an interrupt driven scheme.  It is set to the maximum number
	 * of simultaneous commands a single hw queue in HBA will accept.
	 */
	int can_queue;  /*主机适配器可以同时接受的命令数， 必须大于零*/

	/*
	 * In many instances, especially where disconnect / reconnect are
	 * supported, our host also has an ID on the SCSI bus.  If this is
	 * the case, then it must be reserved.  Please set this_id to -1 if
	 * your setup is in single initiator mode, and the host lacks an
	 * ID.
	 */
/** comment by hy 2019-10-13
 * # 在很多情况下， 尤其是支持断开/重连时， 主机适器在 SCSI
     总线上占用一个ID 这是必须预留该 ID 値.如果只有一个启动器，
     并且hostK缺少 ©, 可将设为-1
 */
	int this_id;

	/*
	 * This determines the degree to which the host adapter is capable
	 * of scatter-gather.
	 */
/** comment by hy 2019-10-13
 * # 主机适配器支持聚散列表的能力
 */
	unsigned short sg_tablesize;
	unsigned short sg_prot_tablesize;

	/*
	 * Set this if the host adapter has limitations beside segment count.
	 */
/** comment by hy 2019-10-13
 * # 主机适K器单个 SCSI 命令能访问扇K的最大数
 */
	unsigned int max_sectors;

	/*
	 * Maximum size in bytes of a single segment.
	 */
	unsigned int max_segment_size;

	/*
	 * DMA scatter gather segment boundary limit. A segment crossing this
	 * boundary will be split in two.
	 */
/** comment by hy 2019-10-13
 * # DMA 聚散段 ( Scatty Gather segment ) 边界限制。 
     跨越这个边界的段将被分割为两个
 */
	unsigned long dma_boundary;

	unsigned long virt_boundary_mask;

	/*
	 * This specifies "machine infinity" for host templates which don't
	 * limit the transfer size.  Note this limit represents an absolute
	 * maximum, and may be over the transfer limits allowed for
	 * individual devices (e.g. 256 for SCSI-1).
	 */
#define SCSI_DEFAULT_MAX_SECTORS	1024

	/*
	 * True if this host adapter can make good use of linked commands.
	 * This will allow more than one command to be queued to a given
	 * unit on a given host.  Set this to the maximum number of command
	 * blocks to be provided for each device.  Set this to 1 for one
	 * command block per lun, 2 for two, etc.  Do not set this to 0.
	 * You should make sure that the host adapter will do the right thing
	 * before you try setting this above 1.
	 */
/** comment by hy 2019-10-13
 * # 允许排入连接到这个主机适紀器的 SCSI 设备的最大命令数 
     即队列深度，被 scsi_adjust_queue_depth 覆写
 */
	short cmd_per_lun;

	/*
	 * present contains counter indicating how many boards of this
	 * type were found when we did the scan.
	 */
/** comment by hy 2019-10-13
 * # 计数器， 给出了在扫描过程中发现了多少个这种类型的主机适配器
 */
	unsigned char present;

	/* If use block layer to manage tags, this is tag allocation policy */
	int tag_alloc_policy;

	/*
	 * Track QUEUE_FULL events and reduce queue depth on demand.
	 */
	unsigned track_queue_depth:1;

	/*
	 * This specifies the mode that a LLD supports.
	 */
	unsigned supported_mode:2;  /*低层驱动支持的模式（启动器或 目标器)*/

	/*
	 * True if this host adapter uses unchecked DMA onto an ISA bus.
	 */
/** comment by hy 2019-10-13
 * # 为 1 表示只能使用 RAM 的低位 作为 DMA 地址空间
         早期的ISA 设备24位的地址限制）,
     为 0 表示可以使用全部 32 倥
 */
	unsigned unchecked_isa_dma:1;

	/*
	 * True for emulated SCSI host adapters (e.g. ATAPI).
	 */
/** comment by hy 2019-10-13
 * # 若是仿真的 SCSI 主机适配器（如 AJAPI), 则为 TRUE
 */
	unsigned emulated:1;

	/*
	 * True if the low-level driver performs its own reset-settle delays.
	 */
/** comment by hy 2019-10-13
 * # 如果为 1， 在主机适配器复位和总线复位后
  ， 低层驱动自行执行reset-settle 延返
 */
	unsigned skip_settle_delay:1;

	/* True if the controller does not support WRITE SAME */
	unsigned no_write_same:1;

	/*
	 * Countdown for host blocking with no commands outstanding.
	 */
/** comment by hy 2019-10-13
 * # 如果主机适配器没有待处理的命令， 则暂时阻塞它
 ，   以便在派发队列中累计足够多针对它的命令•
     从这个阻塞汁算器开始， 毎次SCSI 策略例程处理请求时
     将计数器减一* 报齿主#LiS配器队列未准备好，
     直到计数器减到 0 才恢复常操作
     即开始派发命令到低层驱动
 */
	unsigned int max_host_blocked;

	/*
	 * Default value for the blocking.  If the queue is empty,
	 * host_blocked counts down in the request_fn until it restarts
	 * host operations as zero is reached.  
	 *
	 * FIXME: This should probably be a value in the template
	 */
#define SCSI_DEFAULT_HOST_BLOCKED	7

	/*
	 * Pointer to the sysfs class properties for this host, NULL terminated.
	 */
/** comment by hy 2019-10-13
 * # 该模板的主机适配器的公共属性及其操作方法， 以 NULL结尾
 */
	struct device_attribute **shost_attrs;

	/*
	 * Pointer to the SCSI device properties for this host, NULL terminated.
	 */
/** comment by hy 2019-10-13
 * # 连接到这个模板的主机适配器上的 SCSI 设备的公共属性及其操作方法
 */
	struct device_attribute **sdev_attrs;

	/*
	 * Pointer to the SCSI device attribute groups for this host,
	 * NULL terminated.
	 */
	const struct attribute_group **sdev_groups;

	/*
	 * Vendor Identifier associated with the host
	 *
	 * Note: When specifying vendor_id, be sure to read the
	 *   Vendor Type and ID formatting requirements specified in
	 *   scsi_netlink.h
	 */
/** comment by hy 2019-10-13
 * # 和主机适配器管理的厂商标识符。 若指定 vendor idp 
    阅读 scsi_detliiik.h 指定的厂商类型和 ID 格式需求
 */
	u64 vendor_id;

	/*
	 * Additional per-command data allocated for the driver.
	 */
	unsigned int cmd_size;
	struct scsi_host_cmd_pool *cmd_pool;

	/* Delay for runtime autosuspend */
	int rpm_autosuspend_delay;
};

/*
 * Temporary #define for host lock push down. Can be removed when all
 * drivers have been updated to take advantage of unlocked
 * queuecommand.
 *
 */
#define DEF_SCSI_QCMD(func_name) \
	int func_name(struct Scsi_Host *shost, struct scsi_cmnd *cmd)	\
	{								\
		unsigned long irq_flags;				\
		int rc;							\
		spin_lock_irqsave(shost->host_lock, irq_flags);		\
		rc = func_name##_lck (cmd, cmd->scsi_done);			\
		spin_unlock_irqrestore(shost->host_lock, irq_flags);	\
		return rc;						\
	}


/*
 * shost state: If you alter this, you also need to alter scsi_sysfs.c
 * (for the ascii descriptions) and the state model enforcer:
 * scsi_host_set_state()
 */
enum scsi_host_state {
	SHOST_CREATED = 1,
	SHOST_RUNNING,
	SHOST_CANCEL,
	SHOST_DEL,
	SHOST_RECOVERY,
	SHOST_CANCEL_RECOVERY,
	SHOST_DEL_RECOVERY,
};

struct Scsi_Host {
	/*
	 * __devices is protected by the host_lock, but you should
	 * usually use scsi_device_lookup / shost_for_each_device
	 * to access it and don't care about locking yourself.
	 * In the rare case of being in irq context you can use
	 * their __ prefixed variants with the lock held. NEVER
	 * access this list directly from a driver.
	 */
	struct list_head	__devices;  /* scsi适配器的scsi设备链表头 */
	struct list_head	__targets;  /* scsi适配器的scsi目标节点链表头 */
/** comment by hy 2019-10-13
 * # 由于主机适配器的处理能力， 发送到 SCSI 设备的请求队列中的请求
      可能无法被立即澉发，将 SCSI 设备链入主 机适配器的饥饿链表中等待合适时机再行理
      SCSI 设备链入此链表的连接件为 starved entry
 */
	struct list_head	starved_list;

/** comment by hy 2019-10-13
 * # 用于保护这个主机适配器的自旋锁
 */
	spinlock_t		default_lock;
/** comment by hy 2019-10-13
 * # 指向 default lock 的指针
 */
	spinlock_t		*host_lock;

/** comment by hy 2019-10-13
 * # 用于异步扫描的互斥锁
 */
	struct mutex		scan_mutex;/* serialize scanning activity */

/** comment by hy 2019-10-13
 * # 进入锘误恢复的 SCSI 命令链表的表头
 */
	struct list_head	eh_cmd_q; /* 进入错误恢复的scsi命令链表的表头 */
/** comment by hy 2019-10-13
 * # 错误恢复线程， 往分配本结构的同时创建
 */
	struct task_struct    * ehandler;  /* Error recovery thread. */
/** comment by hy 2019-10-13
 * # 在发送一个用于错误恢复的scsi命令后 需要等待其完成，才能继续
     该指针指向此同步目的的完成变量
 */
	struct completion     * eh_action; /* Wait for specific actions on the
					      host. */ /* 完成变量 */
/** comment by hy 2019-10-13
 * # SCSI 设备错误恢复等待队列 在 SCSI 设备错误恢复进程中，
     要操作SCSI 设备的进程将在此队列上等待直到恢复完成
 */
	wait_queue_head_t       host_wait;  /* 等待队列头 */
/** comment by hy 2019-10-13
 * # 指向用于创建这个主机适配器的模板的指针
 */
	struct scsi_host_template *hostt;
	struct scsi_transport_template *transportt; /* 指向传输模板 */

	/* Area to keep a shared tag map */
	struct blk_mq_tag_set	tag_set;
	atomic_t host_blocked;

	unsigned int host_failed;	   /* commands that failed.
					      protected by host_lock */ /* 失败的命令数 */
/** comment by hy 2019-10-13
 * #  错误恢复都是因出现错误命令后调度的。
      在没有错误命令情况下调度错误恢复的次数
 */
	unsigned int host_eh_scheduled;    /* EH scheduled without command */
/** comment by hy 2019-10-09
 * # host_no 主机编号
 */
	unsigned int host_no;  /* Used for IOCTL_GET_IDLUN, /proc/scsi et al. */

	/* next two fields are used to bound the time spent in error handling */
	int eh_deadline;
	unsigned long last_reset; /* 上次复位时间 以jiffle 为单位,必须确保超过上次复位时间 2 秒*/


	/*
	 * These three parameters can be used to allow for wide scsi,
	 * and for host adapters that support multiple busses
	 * The last two should be set to 1 more than the actual max id
	 * or lun (e.g. 8 for SCSI parallel systems).
	 */
	unsigned int max_channel;
	unsigned int max_id;  /* 目标节点最大编号 */
	u64 max_lun;  /* 逻辑设备lun最大编号 */

	/*
	 * This is a unique identifier that must be assigned so that we
	 * have some way of identifying each detected host adapter properly
	 * and uniquely.  For hosts that do not support more than one card
	 * in the system at one time, this does not need to be set.  It is
	 * initialized to 0 in scsi_register.
	 */
	unsigned int unique_id; /* 唯一标识符 */

	/*
	 * The maximum length of SCSI commands that this host can accept.
	 * Probably 12 for most host adapters, but could be 16 for others.
	 * or 260 if the driver supports variable length cdbs.
	 * For drivers that don't set this field, a value of 12 is
	 * assumed.
	 */
	unsigned short max_cmd_len; /* 可以接受的SCSI命令的最大长度 */

	int this_id;  /* 这个主机适配器的scsi  id */
	int can_queue;  /* 可以同时接受的命令数必须大于零 */
	short cmd_per_lun;  /*允许 SCSI设备的最大命令数,即队列深度*/
	short unsigned int sg_tablesize;  /*±机适配器支持聚敢列表的能力*/
	short unsigned int sg_prot_tablesize;
	unsigned int max_sectors; /*单个 SCSI 命令能访问扇区的最大数目*/
	unsigned int max_segment_size;
	unsigned long dma_boundary; /*DMA 聚散段 (Scatter Gather Segment) 边界限*/
	unsigned long virt_boundary_mask;
	/*
	 * In scsi-mq mode, the number of hardware queues supported by the LLD.
	 *
	 * Note: it is assumed that each hardware queue has a queue depth of
	 * can_queue. In other words, the total queue depth per host
	 * is nr_hw_queues * can_queue.
	 */
	unsigned nr_hw_queues;
	unsigned active_mode:2; /*当前激活的棋式启动器或目标器*/
/** comment by hy 2019-10-13
 * # 为 1 表示只能使用 RAM 的低16MB作为DMA地址空间
       早期的 ISA 设备使用专门有24位的地址限制
     为 0 表示可以使用全部32 位
 */
	unsigned unchecked_isa_dma:1; /**/

	/*
	 * Host has requested that no further requests come through for the
	 * time being.
	 */
/** comment by hy 2019-10-13
 * # 如果为 1, 表示低层驱动要求阻塞该主机适配器，
     即 SCSI 中间层不要醆续派发命令到其臥列
 */
	unsigned host_self_blocked:1;
    
	/*
	 * Host uses correct SCSI ordering not PC ordering. The bit is
	 * set for the minority of drivers whose authors actually read
	 * the spec ;).
	 */
/** comment by hy 2019-10-13
 * # 如果为 h 按逆序 C从 id-1 到 0) 扫 描 SCSI 总 线
 */
	unsigned reverse_ordering:1;

	/* Task mgmt function in progress */
/** comment by hy 2019-10-13
 * # 如果为 1, 表示当前正在执行任务管理功能
 */
	unsigned tmf_in_progress:1;

	/* Asynchronous scan in progress */
/** comment by hy 2019-10-13
 * # 为 I 表示这个主机适配器在执行异步扫描
 */
	unsigned async_scan:1;

	/* Don't resume host in EH */
	unsigned eh_noresume:1;

	/* The controller does not support WRITE SAME */
	unsigned no_write_same:1;

	/* Host responded with short (<36 bytes) INQUIRY result */
	unsigned short_inquiry:1;

	/* The transport requires the LUN bits NOT to be stored in CDB[1] */
	unsigned no_scsi2_lun_in_cdb:1;

	/*
	 * Optional work queue to be utilized by the transport
	 */
	char work_q_name[20]; /*工作队列名字*/
	struct workqueue_struct *work_q;  /*指向工作队列的指针*/

	/*
	 * Task management function work queue
	 */
	struct workqueue_struct *tmf_work_q;

	/*
	 * Value host_blocked counts down from
	 */
	unsigned int max_host_blocked;

	/* Protection Information */
/** comment by hy 2019-10-13
 * # 主机适配器的保护能力 是否支持 DIF 和域 DDO
 */
	unsigned int prot_capabilities;
/** comment by hy 2019-10-13
 * # 主机适E器的护卫标签值计算方式CRC16 或 IP 校验和
 */
	unsigned char prot_guard_type;

	/* legacy crap */
	unsigned long base; /*该 SCSI 主机适配器的 MMIO 基地址*/
	unsigned long io_port;  /*该 SCSI 主机适配器的 I/O 端口编号*/
	unsigned char n_io_port;  /*主机适配器使用的 I/O 空间字节数*/
	unsigned char dma_channel;  /*DMA 通道， 用于老式设备驱动， 新的设备駆动不便用*/
	unsigned int  irq;  /*主机适配器的 IRQ 号*/
	

	enum scsi_host_state shost_state;/*主机适配器的状态*/

	/* ldm bits */
/** comment by hy 2019-10-13
 * # shost_gendev 主机适配器的内嵌通用设备,
       SCSI 设备通过这个域链入 SCSI 总线类型
       (scsi_busjype)的设备链表
    shost_dev 主机适配器的内嵌类设备，
      SCSI 设备通过这个域链入 SCSI 主机适配器类
      (chort_class) 的 设 备 链 表
 */
	struct device		shost_gendev, shost_dev;

	/*
	 * Points to the transport data (if any) which is allocated
	 * separately
	 */
/** comment by hy 2019-10-13
 * # 裉向另外分配的传输层数据
 */
	void *shost_data;

	/*
	 * Points to the physical bus device we'd use to do DMA
	 * Needed just in case we have virtual hosts.
	 */
/** comment by hy 2019-10-13
 * # 指向用来进行 DMA 的物理总线设备的指针，
     仅为虚拟主机适配器所需
 */
	struct device *dma_dev;

	/*
	 * We should ensure that this is aligned, both for better performance
	 * and also because some compilers (m68k) don't automatically force
	 * alignment to a long boundary.
	 */
/** comment by hy 2019-10-13
 * # 用以倮存主机适配器的专有數据
 */
	unsigned long hostdata[]  /* Used for storage of host specific stuff */
		__attribute__ ((aligned (sizeof(unsigned long))));
};

#define		class_to_shost(d)	\
	container_of(d, struct Scsi_Host, shost_dev)

#define shost_printk(prefix, shost, fmt, a...)	\
	dev_printk(prefix, &(shost)->shost_gendev, fmt, ##a)

static inline void *shost_priv(struct Scsi_Host *shost)
{
	return (void *)shost->hostdata;
}

int scsi_is_host_device(const struct device *);

static inline struct Scsi_Host *dev_to_shost(struct device *dev)
{
	while (!scsi_is_host_device(dev)) {
		if (!dev->parent)
			return NULL;
		dev = dev->parent;
	}
	return container_of(dev, struct Scsi_Host, shost_gendev);
}

static inline int scsi_host_in_recovery(struct Scsi_Host *shost)
{
	return shost->shost_state == SHOST_RECOVERY ||
		shost->shost_state == SHOST_CANCEL_RECOVERY ||
		shost->shost_state == SHOST_DEL_RECOVERY ||
		shost->tmf_in_progress;
}

extern int scsi_queue_work(struct Scsi_Host *, struct work_struct *);
extern void scsi_flush_work(struct Scsi_Host *);

extern struct Scsi_Host *scsi_host_alloc(struct scsi_host_template *, int);
extern int __must_check scsi_add_host_with_dma(struct Scsi_Host *,
					       struct device *,
					       struct device *);
extern void scsi_scan_host(struct Scsi_Host *);
extern void scsi_rescan_device(struct device *);
extern void scsi_remove_host(struct Scsi_Host *);
extern struct Scsi_Host *scsi_host_get(struct Scsi_Host *);
extern int scsi_host_busy(struct Scsi_Host *shost);
extern void scsi_host_put(struct Scsi_Host *t);
extern struct Scsi_Host *scsi_host_lookup(unsigned short);
extern const char *scsi_host_state_name(enum scsi_host_state);
extern void scsi_host_complete_all_commands(struct Scsi_Host *shost,
					    int status);

/*****************************************************************************
 * 函 数 名  : scsi_add_host
 * 负 责 人  : hy
 * 创建日期  : 2019年10月14日
 * 函数功能  :  
 * 输入参数  : struct Scsi_Host *host   指向主机适配器描述符的指针
               struct device *dev       指向这个SCSI主机适配器在驱动模型中的父设备的device描述符
                      它决定了这个SCSI主机适配器在sysf文件系统中的位置
                      PCI-SCSI驱动 通常将它设置为对应PCI设备的内嵌驱动模型设备
                      例如某个SCSI主机适配器对应sysfs文件系统的/sys/devices/pci0000:00/0000:00:07.1/host0/目录
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static inline int __must_check scsi_add_host(struct Scsi_Host *host,
					     struct device *dev)
{
	return scsi_add_host_with_dma(host, dev, dev);
}

static inline struct device *scsi_get_device(struct Scsi_Host *shost)
{
        return shost->shost_gendev.parent;
}

/**
 * scsi_host_scan_allowed - Is scanning of this host allowed
 * @shost:	Pointer to Scsi_Host.
 **/
static inline int scsi_host_scan_allowed(struct Scsi_Host *shost)
{
	return shost->shost_state == SHOST_RUNNING ||
	       shost->shost_state == SHOST_RECOVERY;
}

extern void scsi_unblock_requests(struct Scsi_Host *);
extern void scsi_block_requests(struct Scsi_Host *);
extern int scsi_host_block(struct Scsi_Host *shost);
extern int scsi_host_unblock(struct Scsi_Host *shost, int new_state);

void scsi_host_busy_iter(struct Scsi_Host *,
			 bool (*fn)(struct scsi_cmnd *, void *, bool), void *priv);

struct class_container;

/*
 * These two functions are used to allocate and free a pseudo device
 * which will connect to the host adapter itself rather than any
 * physical device.  You must deallocate when you are done with the
 * thing.  This physical pseudo-device isn't real and won't be available
 * from any high-level drivers.
 */
extern void scsi_free_host_dev(struct scsi_device *);
extern struct scsi_device *scsi_get_host_dev(struct Scsi_Host *);

/*
 * DIF defines the exchange of protection information between
 * initiator and SBC block device.
 *
 * DIX defines the exchange of protection information between OS and
 * initiator.
 */
enum scsi_host_prot_capabilities {
	SHOST_DIF_TYPE1_PROTECTION = 1 << 0, /* T10 DIF Type 1 */
	SHOST_DIF_TYPE2_PROTECTION = 1 << 1, /* T10 DIF Type 2 */
	SHOST_DIF_TYPE3_PROTECTION = 1 << 2, /* T10 DIF Type 3 */

	SHOST_DIX_TYPE0_PROTECTION = 1 << 3, /* DIX between OS and HBA only */
	SHOST_DIX_TYPE1_PROTECTION = 1 << 4, /* DIX with DIF Type 1 */
	SHOST_DIX_TYPE2_PROTECTION = 1 << 5, /* DIX with DIF Type 2 */
	SHOST_DIX_TYPE3_PROTECTION = 1 << 6, /* DIX with DIF Type 3 */
};

/*
 * SCSI hosts which support the Data Integrity Extensions must
 * indicate their capabilities by setting the prot_capabilities using
 * this call.
 */
static inline void scsi_host_set_prot(struct Scsi_Host *shost, unsigned int mask)
{
	shost->prot_capabilities = mask;
}

static inline unsigned int scsi_host_get_prot(struct Scsi_Host *shost)
{
	return shost->prot_capabilities;
}

static inline int scsi_host_prot_dma(struct Scsi_Host *shost)
{
	return shost->prot_capabilities >= SHOST_DIX_TYPE0_PROTECTION;
}

static inline unsigned int scsi_host_dif_capable(struct Scsi_Host *shost, unsigned int target_type)
{
	static unsigned char cap[] = { 0,
				       SHOST_DIF_TYPE1_PROTECTION,
				       SHOST_DIF_TYPE2_PROTECTION,
				       SHOST_DIF_TYPE3_PROTECTION };

	if (target_type >= ARRAY_SIZE(cap))
		return 0;

	return shost->prot_capabilities & cap[target_type] ? target_type : 0;
}

static inline unsigned int scsi_host_dix_capable(struct Scsi_Host *shost, unsigned int target_type)
{
#if defined(CONFIG_BLK_DEV_INTEGRITY)
	static unsigned char cap[] = { SHOST_DIX_TYPE0_PROTECTION,
				       SHOST_DIX_TYPE1_PROTECTION,
				       SHOST_DIX_TYPE2_PROTECTION,
				       SHOST_DIX_TYPE3_PROTECTION };

	if (target_type >= ARRAY_SIZE(cap))
		return 0;

	return shost->prot_capabilities & cap[target_type];
#endif
	return 0;
}

/*
 * All DIX-capable initiators must support the T10-mandated CRC
 * checksum.  Controllers can optionally implement the IP checksum
 * scheme which has much lower impact on system performance.  Note
 * that the main rationale for the checksum is to match integrity
 * metadata with data.  Detecting bit errors are a job for ECC memory
 * and buses.
 */

enum scsi_host_guard_type {
	SHOST_DIX_GUARD_CRC = 1 << 0,
	SHOST_DIX_GUARD_IP  = 1 << 1,
};

static inline void scsi_host_set_guard(struct Scsi_Host *shost, unsigned char type)
{
	shost->prot_guard_type = type;
}

static inline unsigned char scsi_host_get_guard(struct Scsi_Host *shost)
{
	return shost->prot_guard_type;
}

extern int scsi_host_set_state(struct Scsi_Host *, enum scsi_host_state);

#endif /* _SCSI_SCSI_HOST_H */
