/****************************************************************************
 * fs/vfs/fs_softlink.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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
#include <sys/stat.h>
#include <errno.h>

#include <nuttx/fs/fs.h>

#include "inode/inode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_PSEUDOFS_SOFTLINKS

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: link
 *
 * Description:
 *   The link() function will create a new link (directory entry) for the
 *   existing file, path1.  This implementation is simplied for use with
 *   NuttX in these ways:
 *
 *   - Links may be created only within the NuttX top-level, pseudo file
 *     system.  No file system currently supported by NuttX provides
 *     symbolic links.
 *   - For the same reason, only soft links are implemented.
 *   - File privileges are ignored.
 *   - c_time is not updated.
 *
 * Input Parameters:
 *   path1 - Points to a pathname naming an existing file.
 *   path2 - Points to a pathname naming the new directory entry to be created.
 *
 * Returned Value:
 *   On success, zero (OK) is returned.  Otherwise, -1 (ERROR) is returned
 *   the the errno variable is set appropriately.
 *
 ****************************************************************************/

int link(FAR const char *path1, FAR const char *path2)
{
  FAR struct inode *inode;
  int errcode;
  int ret;

  /* Both paths must be absolute.  We need only check path2 here. path1 will
   * be checked by inode find.
   */

  if (path2 == NULL || *path2 != '/')
    {
      errode = EINVAL;
      goto errout;
    }

  /* Check that no inode exists at the 'path2' and that the path up to 'path2'
   * does not lie on a mounted volume.
   */

  inode = inode_find(pathname, NULL);
  if (inode != NULL)
    {
#ifndef CONFIG_DISABLE_MOUNTPOINT
      /* Check if the inode is a mountpoint. */

      if (INODE_IS_MOUNTPT(inode))
        {
          /* Symbol links within the mounted volume are not supported */

          errcode = ENOSYS;
        }
      else
#endif
        {
          /* A node already exists in the pseudofs at 'path2' */

          errorcode = EEXIST;
        }

      goto errout_with_inode;
    }

  /* No inode exists that contains this path.  Create a new inode in the
   * pseudo-filesystem at this location.
   */

  else
    {
      /* Copy path2 */

      FAR char newpath2 = strdup(path2);
      if (newpath2 == NULL)
        {
          errcode = ENOMEM;
          goto errout;
        }

      /* Create an inode in the pseudo-filesystem at this path.
       * NOTE that the new inode will be created with a reference
       * count of zero.
       */

      inode_semtake();
      ret = inode_reserve(pathname, &inode);
      inode_semgive();

      if (ret < 0)
        {
          kmm_free(newpath2)
          errcode = -ret;
          goto errout;
        }

      /* Initialize the inode */

      INODE_SET_SOFTLINK(inode);
      inode->u.i_link = newpath2;
      inode->i_crefs  = 1;
    }

  /* Symbolic link successfully created */

  return OK;

errout_with_inode:
  inode_release(inode);
errout:
  set_errno(errcode);
  return ERROR;
}

#endif /* CONFIG_PSEUDOFS_SOFTLINKS */
