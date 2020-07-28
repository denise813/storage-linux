// SPDX-License-Identifier: GPL-2.0
/*
 * Functions related to setting various queue properties from drivers
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/sched/sysctl.h>

#include "blk.h"
#include "blk-mq-sched.h"

/**
 * blk_end_sync_rq - executes a completion event on a request
 * @rq: request to complete
 * @error: end I/O status of the request
 */
static void blk_end_sync_rq(struct request *rq, blk_status_t error)
{
	struct completion *waiting = rq->end_io_data;

	rq->end_io_data = NULL;

	/*
	 * complete last, if this is a stack request the process (and thus
	 * the rq pointer) could be invalid right after this complete()
	 */
	complete(waiting);
}

/**
 * blk_execute_rq_nowait - insert a request into queue for execution
 * @q:		queue to insert the request in
 * @bd_disk:	matching gendisk
 * @rq:		request to insert
 * @at_head:    insert request at head or tail of queue
 * @done:	I/O completion handler
 *
 * Description:
 *    Insert a fully prepared request at the back of the I/O scheduler queue
 *    for execution.  Don't wait for completion.
 *
 * Note:
 *    This function will invoke @done directly if the queue is dead.
 */
/*****************************************************************************
 * 函 数 名  : blk_execute_rq_nowait
 * 负 责 人  : hy
 * 创建日期  : 2019年10月16日
 * 函数功能  :  
 * 输入参数  : struct request_queue *q   
               struct gendisk *bd_disk   
               struct request *rq        
               int at_head               
               rq_end_io_fn *done        
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
void blk_execute_rq_nowait(struct request_queue *q, struct gendisk *bd_disk,
			   struct request *rq, int at_head,
			   rq_end_io_fn *done)
{
	WARN_ON(irqs_disabled());
	WARN_ON(!blk_rq_is_passthrough(rq));

/** comment by hy 2018-10-16
 * # 提交请求的工作
 */
	rq->rq_disk = bd_disk;
	rq->end_io = done;

	blk_account_io_start(rq);

	/*
	 * don't check dying flag for MQ because the request won't
	 * be reused after dying flag is set
	 */
	blk_mq_sched_insert_request(rq, at_head, true, false);
}
EXPORT_SYMBOL_GPL(blk_execute_rq_nowait);

/**
 * blk_execute_rq - insert a request into queue for execution
 * @q:		queue to insert the request in
 * @bd_disk:	matching gendisk
 * @rq:		request to insert
 * @at_head:    insert request at head or tail of queue
 *
 * Description:
 *    Insert a fully prepared request at the back of the I/O scheduler queue
 *    for execution and wait for completion.
 */
/*****************************************************************************
 * 函 数 名  : blk_execute_rq
 * 负 责 人  : hy
 * 创建日期  : 2019年10月16日
 * 函数功能  :   
 * 输入参数  : struct request_queue *q  指向请求队列描述符的指针
               struct gendisk *bd_disk  指向对应通用磁盘描述符的指针
               struct request *rq       指向要插入的请求的指针
               int at_head              表示插入的位置 1 插入队列头部
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
void blk_execute_rq(struct request_queue *q, struct gendisk *bd_disk,
		   struct request *rq, int at_head)
{
	DECLARE_COMPLETION_ONSTACK(wait);
	unsigned long hang_check;

/** comment by hy 2018-10-16
 * # 利用Linux内核中的completion来实现完成等待逻辑
     驱动程序要在执行后面操作之前等待某个过程的完成，
     它可以调用wait_for_completion
     动态初始化 init_completion
     驱动程序要在执行后面操作之前等待某个过程的完成
       wait_for_completion
     唤醒等待该事件
      complete or complete_all
 */
	rq->end_io_data = &wait;
	blk_execute_rq_nowait(q, bd_disk, rq, at_head, blk_end_sync_rq);

	/* Prevent hang_check timer from firing at us during very long I/O */
	hang_check = sysctl_hung_task_timeout_secs;
	if (hang_check)
		while (!wait_for_completion_io_timeout(&wait, hang_check * (HZ/2)));
	else
		wait_for_completion_io(&wait);
}
EXPORT_SYMBOL(blk_execute_rq);
