/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __TARGET_CORE_USER_H
#define __TARGET_CORE_USER_H

/* This header will be used by application too */

#include <linux/types.h>
#include <linux/uio.h>

#define TCMU_VERSION "2.0"

/**
 * DOC: Ring Design
 * Ring Design
 * -----------
 *
 * The mmaped area is divided into three parts:
 * 1) The mailbox (struct tcmu_mailbox, below);
 * 2) The command ring;
 * 3) Everything beyond the command ring (data).
 *
 * The mailbox tells userspace the offset of the command ring from the
 * start of the shared memory region, and how big the command ring is.
 *
 * The kernel passes SCSI commands to userspace by putting a struct
 * tcmu_cmd_entry in the ring, updating mailbox->cmd_head, and poking
 * userspace via UIO's interrupt mechanism.
 *
 * tcmu_cmd_entry contains a header. If the header type is PAD,
 * userspace should skip hdr->length bytes (mod cmdr_size) to find the
 * next cmd_entry.
 *
 * Otherwise, the entry will contain offsets into the mmaped area that
 * contain the cdb and data buffers -- the latter accessible via the
 * iov array. iov addresses are also offsets into the shared area.
 *
 * When userspace is completed handling the command, set
 * entry->rsp.scsi_status, fill in rsp.sense_buffer if appropriate,
 * and also set mailbox->cmd_tail equal to the old cmd_tail plus
 * hdr->length, mod cmdr_size. If cmd_tail doesn't equal cmd_head, it
 * should process the next packet the same way, and so on.
 */

#define TCMU_MAILBOX_VERSION 2
#define ALIGN_SIZE 64 /* Should be enough for most CPUs */
#define TCMU_MAILBOX_FLAG_CAP_OOOC (1 << 0) /* Out-of-order completions */
#define TCMU_MAILBOX_FLAG_CAP_READ_LEN (1 << 1) /* Read data length */

struct tcmu_mailbox {
	__u16 version;  /*2 如果是别的值，用户空间应该废弃*/
	__u16 flags;  /*标志支持out-of-order completion*/
	__u32 cmdr_off; /*command ring在内存区域的起始位置的偏移量*/
	__u32 cmdr_size;  /*command ring区域的大小。这不需要2的幂来表示*/

	__u32 cmd_head; /*由内核修改，表示一个command已经放置到ring中*/

	/* Updated by user. On its own cacheline */
/** comment by hy 2018-11-06
 * # 由用户空间修改，表示一个command已经处理完成
	   用户空间根据entry.hdr.length(mod cmdr_size)增加mailbox.cmd_tail
	   并且通过UIO方法通知内核，4字节写到文件描述符
 */
	__u32 cmd_tail __attribute__((__aligned__(ALIGN_SIZE)));

} __packed;

enum tcmu_opcode {
	TCMU_OP_PAD = 0,
	TCMU_OP_CMD,
};

/*
 * Only a few opcodes, and length is 8-byte aligned, so use low bits for opcode.
 */
struct tcmu_cmd_entry_hdr {
	__u32 len_op;
	__u16 cmd_id;
	__u8 kflags;
#define TCMU_UFLAG_UNKNOWN_OP 0x1
#define TCMU_UFLAG_READ_LEN   0x2
	__u8 uflags;

} __packed;

#define TCMU_OP_MASK 0x7

static inline enum tcmu_opcode tcmu_hdr_get_op(__u32 len_op)
{
	return len_op & TCMU_OP_MASK;
}

static inline void tcmu_hdr_set_op(__u32 *len_op, enum tcmu_opcode op)
{
	*len_op &= ~TCMU_OP_MASK;
	*len_op |= (op & TCMU_OP_MASK);
}

static inline __u32 tcmu_hdr_get_len(__u32 len_op)
{
	return len_op & ~TCMU_OP_MASK;
}

static inline void tcmu_hdr_set_len(__u32 *len_op, __u32 len)
{
	*len_op &= TCMU_OP_MASK;
	*len_op |= len;
}

/* Currently the same as SCSI_SENSE_BUFFERSIZE */
#define TCMU_SENSE_BUFFERSIZE 96

struct tcmu_cmd_entry {
	struct tcmu_cmd_entry_hdr hdr;

	union {
		struct {
			__u32 iov_cnt;  /*指定多少iovec覆盖了Data-Out区域*/
			__u32 iov_bidi_cnt; /*指定了多少iovec覆盖了Data-In区域*/
			__u32 iov_dif_cnt;
			__u64 cdb_off;  /*SCSI CDB 是在整个共享内存区域起始位置的偏移量*/
			__u64 __pad1;
			__u64 __pad2;
			struct iovec iov[0];
		} req;
		struct {
			__u8 scsi_status;
			__u8 __pad1;
			__u16 __pad2;
			__u32 read_len;
			char sense_buffer[TCMU_SENSE_BUFFERSIZE];
		} rsp;
	};

} __packed;

#define TCMU_OP_ALIGN_SIZE sizeof(__u64)

enum tcmu_genl_cmd {
	TCMU_CMD_UNSPEC,
	TCMU_CMD_ADDED_DEVICE,
	TCMU_CMD_REMOVED_DEVICE,
	TCMU_CMD_RECONFIG_DEVICE,
	TCMU_CMD_ADDED_DEVICE_DONE,
	TCMU_CMD_REMOVED_DEVICE_DONE,
	TCMU_CMD_RECONFIG_DEVICE_DONE,
	TCMU_CMD_SET_FEATURES,
	__TCMU_CMD_MAX,
};
#define TCMU_CMD_MAX (__TCMU_CMD_MAX - 1)

enum tcmu_genl_attr {
	TCMU_ATTR_UNSPEC,
	TCMU_ATTR_DEVICE,
	TCMU_ATTR_MINOR,
	TCMU_ATTR_PAD,
	TCMU_ATTR_DEV_CFG,
	TCMU_ATTR_DEV_SIZE,
	TCMU_ATTR_WRITECACHE,
	TCMU_ATTR_CMD_STATUS,
	TCMU_ATTR_DEVICE_ID,
	TCMU_ATTR_SUPP_KERN_CMD_REPLY,
	__TCMU_ATTR_MAX,
};
#define TCMU_ATTR_MAX (__TCMU_ATTR_MAX - 1)

#endif
