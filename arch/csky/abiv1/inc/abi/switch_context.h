/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ABI_CSKY_PTRACE_H
#define __ABI_CSKY_PTRACE_H

struct switch_stack {
	unsigned long r8;   //线程参数
	unsigned long r9;   //sp
	unsigned long r10;
	unsigned long r11;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;  //pc
};
#endif /* __ABI_CSKY_PTRACE_H */
