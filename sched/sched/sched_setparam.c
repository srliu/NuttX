/****************************************************************************
 * sched/sched/sched_setparam.c
 *
 *   Copyright (C) 2007, 2009, 2013, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sched.h>
#include <errno.h>

#include <nuttx/arch.h>

#include "clock/clock.h"
#include "sched/sched.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  sched_setparam
 *
 * Description:
 *   This function sets the priority of a specified task.
 *
 *   NOTE: Setting a task's priority to the same value has a similar effect
 *   to sched_yield() -- The task will be moved to  after all other tasks
 *   with the same priority.
 *
 * Inputs:
 *   pid - the task ID of the task to reprioritize.  If pid is zero, the
 *      priority of the calling task is changed.
 *   param - A structure whose member sched_priority is the integer priority.
 *      The range of valid priority numbers is from SCHED_PRIORITY_MIN
 *      through SCHED_PRIORITY_MAX.
 *
 * Return Value:
 *   On success, sched_setparam() returns 0 (OK). On error, -1 (ERROR) is
 *   returned, and errno is set appropriately.
 *
 *  EINVAL The parameter 'param' is invalid or does not make sense for the
 *         current scheduling policy.
 *  EPERM  The calling task does not have appropriate privileges.
 *  ESRCH  The task whose ID is pid could not be found.
 *
 * Assumptions:
 *
 ****************************************************************************/

int sched_setparam(pid_t pid, FAR const struct sched_param *param)
{
  FAR struct tcb_s *rtcb;
  FAR struct tcb_s *tcb;
  int errcode;
  int ret;

  /* Verify that the requested priority is in the valid range */

  if (!param)
    {
      errcode = EINVAL;
      goto errout_with_errcode;
    }

  /* Prohibit modifications to the head of the ready-to-run task
   * list while adjusting the priority
   */

  sched_lock();

  /* Check if the task to reprioritize is the calling task */

  rtcb = (FAR struct tcb_s*)g_readytorun.head;
  if (pid == 0 || pid == rtcb->pid)
    {
      tcb = rtcb;
    }

  /* The PID is not the calling task, we will have to search for it */

  else
    {
      tcb = sched_gettcb(pid);
      if (!tcb)
        {
          /* No task with this PID was found */

          errcode = ESRCH;
          goto errout_with_lock;
        }
    }

#ifdef CONFIG_SCHED_SPORADIC
  /* Update parameters associated with SCHED_SPORADIC */

  if ((rtcb->flags & TCB_FLAG_POLICY_MASK) == TCB_FLAG_SCHED_SPORADIC)
    {
      int repl_ticks;
      int budget_ticks;

      DEBUGASSERT(param->sched_ss_max_repl <= UINT8_MAX);

      /* Convert timespec values to system clock ticks */

      (void)clock_time2ticks(&param->sched_ss_repl_period, &repl_ticks);
      (void)clock_time2ticks(&param->sched_ss_init_budget, &budget_ticks);

      /* The replenishment period must be greater than or equal to the
       * budget period.
       */

      if (repl_ticks < budget_ticks)
        {
          errcode = EINVAL;
          goto errout_with_lock;
        }

      /* Save the sporadic scheduling parameters */

      tcb->flags       |= TCB_FLAG_SCHED_SPORADIC;
      tcb->timeslice    = MSEC2TICK(CONFIG_RR_INTERVAL);
      tcb->low_priority = param->sched_ss_low_priority;
      tcb->max_repl     = param->sched_ss_max_repl;
      tcb->repl_period  = repl_ticks;
      tcb->budget       = budget_ticks;
    }
  else
    {
      tcb->low_priority = 0;
      tcb->max_repl     = 0;
      tcb->repl_period  = 0;
      tcb->budget       = 0;
    }
#endif

  /* Then perform the reprioritization */

  ret = sched_reprioritize(tcb, param->sched_priority);
  sched_unlock();
  return ret;

errout_with_lock:
  set_errno(errcode);
  sched_unlock();
  return ERROR;

errout_with_errcode:
  set_errno(errcode);
  return ERROR;
}
