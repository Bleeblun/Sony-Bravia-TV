#ifndef _GDTH_PROC_H
#define _GDTH_PROC_H

/* gdth_proc.h 
 * $Id: //DTV/MP_BR/DTV_X_IDTV1501_002288_8_001_800_002/apollo/linux_core/kernel/linux-3.10/drivers/scsi/gdth_proc.h#1 $
 */

int gdth_execute(struct Scsi_Host *shost, gdth_cmd_str *gdtcmd, char *cmnd,
                 int timeout, u32 *info);

static int gdth_set_asc_info(struct Scsi_Host *host, char *buffer,
                             int length, gdth_ha_str *ha);

static char *gdth_ioctl_alloc(gdth_ha_str *ha, int size, int scratch,
                              u64 *paddr);
static void gdth_ioctl_free(gdth_ha_str *ha, int size, char *buf, u64 paddr);
static void gdth_wait_completion(gdth_ha_str *ha, int busnum, int id);

#endif

