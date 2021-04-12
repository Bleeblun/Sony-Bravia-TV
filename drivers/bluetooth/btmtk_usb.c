/*
*  Copyright (c) 2014 MediaTek Inc.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include "btmtk_usb.h"
#include <linux/skbuff.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>  /*Used for kfifo*/
#include <linux/jiffies.h>
#include <linux/kallsyms.h>



/* Used for wake on Bluetooth */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>


/* Local Configuration ========================================================= */
#define VERSION "5.0.3.31"


/* ============================================================================= */



static struct usb_driver btmtk_usb_driver;

#if LOAD_PROFILE
void btmtk_usb_load_profile(struct hci_dev *hdev);
#endif

static char driver_version[64] = { 0 };
static char rom_patch_version[64] = { 0 };
static char fw_version[64] = { 0 };

static unsigned char probe_counter;

/* stpbt character device for meta */
static int BT_major;
struct class *pBTClass = NULL;
struct device *pBTDev = NULL;
static struct cdev BT_cdev;
static unsigned char *i_buf = NULL;	/* input buffer of read() */
static unsigned char *o_buf = NULL;	/* output buffer of write() */
static struct semaphore wr_mtx, rd_mtx;
static wait_queue_head_t inq;	/* read queues */
static int BT_devs = 1;		/* device count */
static volatile int metaMode;
static volatile int metaCount;
static volatile int leftHciEventSize;
static int leftACLSize;
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static wait_queue_head_t inq;

static int hciEventMaxSize;
struct usb_device *meta_udev = NULL;
static ring_buffer_struct *metabuffer;
struct hci_dev *meta_hdev = NULL;

#define SUSPEND_TIMEOUT (130*HZ)
#define WAKEUP_TIMEOUT (170*HZ)
struct btmtk_usb_data *g_data = NULL;

static int isbtready;
static int isUsbDisconnet;
static volatile int need_reset_stack = 0;
static volatile int skip_hci_cmd_when_stack_reset = 0;
static unsigned char reset_event[] = {0x04, 0xFF, 0x04, 0x00, 0x01, 0xFF, 0xFF};
static volatile int is_assert = 0;
static volatile int is_fops_close_ongoing = 0;

static unsigned char* g_rom_patch_image=NULL;
static u32 g_rom_patch_code_len=0;

static void btmtk_usb_cap_init(struct btmtk_usb_data *data);

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
enum
{
    BTMTK_USB_STATE_UNKNOWN,
    BTMTK_USB_STATE_INIT,
    BTMTK_USB_STATE_DISCONNECT,
    BTMTK_USB_STATE_PROBE,
    BTMTK_USB_STATE_WORKING,
    BTMTK_USB_STATE_EARLY_SUSPEND,
    BTMTK_USB_STATE_SUSPEND,
    BTMTK_USB_STATE_RESUME,
    BTMTK_USB_STATE_LATE_RESUME,
    BTMTK_USB_STATE_FW_DUMP,
    BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT,
    BTMTK_USB_STATE_SUSPEND_ERROR_PROBE,
    BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP,
    BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT,
    BTMTK_USB_STATE_RESUME_ERROR_PROBE,
    BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP,
};
typedef void (*register_early_suspend)(void (*f)(void));
typedef void (*register_late_resume)(void (*f)(void));
char *register_early_suspend_func_name = "RegisterEarlySuspendNotification";
char *register_late_resume_func_name = "RegisterLateResumeNotification";
register_early_suspend register_early_suspend_func = NULL;
register_late_resume register_late_resume_func = NULL;
static void btmtk_usb_early_suspend(void);
static void btmtk_usb_late_resume(void);
static volatile int btmtk_usb_state = BTMTK_USB_STATE_UNKNOWN;
static DEFINE_MUTEX(btmtk_usb_state_mutex);
static int btmtk_usb_get_state(void)
{
    return btmtk_usb_state;
}
static void btmtk_usb_set_state(int new_state)
{
    printk("%s : %s(%d) -> %s(%d)\n", __FUNCTION__,
            btmtk_usb_state==BTMTK_USB_STATE_INIT?"INIT":
            btmtk_usb_state==BTMTK_USB_STATE_DISCONNECT?"DISCONNECT":
            btmtk_usb_state==BTMTK_USB_STATE_PROBE?"PROBE":
            btmtk_usb_state==BTMTK_USB_STATE_WORKING?"WORKING":
            btmtk_usb_state==BTMTK_USB_STATE_EARLY_SUSPEND?"EARLY_SUSPEND":
            btmtk_usb_state==BTMTK_USB_STATE_SUSPEND?"SUSPEND":
            btmtk_usb_state==BTMTK_USB_STATE_RESUME?"RESUME":
            btmtk_usb_state==BTMTK_USB_STATE_LATE_RESUME?"LATE_RESUME":
            btmtk_usb_state==BTMTK_USB_STATE_FW_DUMP?"FW_DUMP":
            btmtk_usb_state==BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT?"SUSPEND_DISCONNECT":
            btmtk_usb_state==BTMTK_USB_STATE_SUSPEND_ERROR_PROBE?"SUSPEND_PROBE":
            btmtk_usb_state==BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP?"SUSPEND_FW_DUMP":
            btmtk_usb_state==BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT?"RESUME_DISCONNECT":
            btmtk_usb_state==BTMTK_USB_STATE_RESUME_ERROR_PROBE?"RESUME_PROBE":
            btmtk_usb_state==BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP?"RESUME_FW_DUMP":"UNKNOWN",
            btmtk_usb_state,
            new_state==BTMTK_USB_STATE_INIT?"INIT":
            new_state==BTMTK_USB_STATE_DISCONNECT?"DISCONNECT":
            new_state==BTMTK_USB_STATE_PROBE?"PROBE":
            new_state==BTMTK_USB_STATE_WORKING?"WORKING":
            new_state==BTMTK_USB_STATE_EARLY_SUSPEND?"EARLY_SUSPEND":
            new_state==BTMTK_USB_STATE_SUSPEND?"SUSPEND":
            new_state==BTMTK_USB_STATE_RESUME?"RESUME":
            new_state==BTMTK_USB_STATE_LATE_RESUME?"LATE_RESUME":
            new_state==BTMTK_USB_STATE_FW_DUMP?"FW_DUMP":
            new_state==BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT?"SUSPEND_DISCONNECT":
            new_state==BTMTK_USB_STATE_SUSPEND_ERROR_PROBE?"SUSPEND_PROBE":
            new_state==BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP?"SUSPEND_FW_DUMP":
            new_state==BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT?"RESUME_DISCONNECT":
            new_state==BTMTK_USB_STATE_RESUME_ERROR_PROBE?"RESUME_PROBE":
            new_state==BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP?"RESUME_FW_DUMP":"UNKNOWN",
            new_state);
    btmtk_usb_state = new_state;
}
#endif

#if __MTK_BT_BOOT_ON_FLASH__
static char rom_patch_crc[8] = {0};
#endif

#if BT_ROM_PATCH_FROM_BIN
int LOAD_CODE_METHOD = BIN_FILE_METHOD;
static unsigned char *mt7662_rom_patch;

	#if SUPPORT_MT7637
static unsigned char *mt7637_rom_patch;
	#endif

	#if SUPPORT_MT6632
static unsigned char *mt6632_rom_patch;
	#endif


#else
int LOAD_CODE_METHOD = HEADER_METHOD;
#include "./mt7662_rom_patch.h"

	#if SUPPORT_MT7637
#include "./mt7637_rom_patch.h"
	#endif

	#if SUPPORT_MT6632
#include "./mt6632_rom_patch.h"
	#endif

#endif

static int btmtk_usb_BT_init(void);
static void btmtk_usb_BT_exit(void);
static int btmtk_usb_send_assert_cmd(struct usb_device *udev);
static int btmtk_usb_send_assert_cmd1(struct usb_device *udev);
static int btmtk_usb_send_assert_cmd2(struct usb_device *udev);

#if SUPPORT_HCI_DUMP
#define HCI_DUMP_ENTRY_NUM 30
#define HCI_DUMP_BUF_SIZE 32
static unsigned char hci_cmd_dump[HCI_DUMP_ENTRY_NUM][HCI_DUMP_BUF_SIZE];
static unsigned char hci_cmd_dump_len[HCI_DUMP_ENTRY_NUM] = {0};
static unsigned int  hci_cmd_dump_timestamp[HCI_DUMP_ENTRY_NUM];
static unsigned char hci_event_dump[HCI_DUMP_ENTRY_NUM][HCI_DUMP_BUF_SIZE];
static unsigned char hci_event_dump_len[HCI_DUMP_ENTRY_NUM] = {0};
static unsigned int  hci_event_dump_timestamp[HCI_DUMP_ENTRY_NUM];
static unsigned char hci_acl_dump[HCI_DUMP_ENTRY_NUM][HCI_DUMP_BUF_SIZE];
static unsigned char hci_acl_dump_len[HCI_DUMP_ENTRY_NUM] = {0};
static unsigned int  hci_acl_dump_timestamp[HCI_DUMP_ENTRY_NUM];
static int hci_cmd_dump_index = HCI_DUMP_ENTRY_NUM-1;
static int hci_event_dump_index = HCI_DUMP_ENTRY_NUM-1;
static int hci_acl_dump_index = HCI_DUMP_ENTRY_NUM-1;

static unsigned int btmtk_usb_get_microseconds(void)
{
    struct timeval now;
    do_gettimeofday(&now);
	return (now.tv_sec * 1000000 + now.tv_usec);
}

static void btmtk_usb_hci_dmp_init(void)
{
    int i;
    hci_cmd_dump_index = HCI_DUMP_ENTRY_NUM-1;
    hci_event_dump_index = HCI_DUMP_ENTRY_NUM-1;
    hci_acl_dump_index = HCI_DUMP_ENTRY_NUM-1;
    for ( i = 0 ; i < HCI_DUMP_ENTRY_NUM ; i++ )
    {
        hci_cmd_dump_len[i] = 0;
        hci_event_dump_len[i] = 0;
        hci_acl_dump_len[i] = 0;
    }
}

static void btmtk_usb_save_hci_cmd(int len, unsigned char* buf)
{
    int copy_len = HCI_DUMP_BUF_SIZE;
    if ( buf )
    {
        if ( len < HCI_DUMP_BUF_SIZE )
            copy_len = len;
        hci_cmd_dump_len[hci_cmd_dump_index] = copy_len&0xff;
        memset(hci_cmd_dump[hci_cmd_dump_index], 0, HCI_DUMP_BUF_SIZE);
        memcpy(hci_cmd_dump[hci_cmd_dump_index], buf, copy_len&0xff);
        hci_cmd_dump_timestamp[hci_cmd_dump_index] = btmtk_usb_get_microseconds();

        hci_cmd_dump_index--;
        if ( hci_cmd_dump_index < 0 )
            hci_cmd_dump_index = HCI_DUMP_ENTRY_NUM-1;
    }
}

static void btmtk_usb_save_hci_event(int len, unsigned char* buf)
{
    int copy_len = HCI_DUMP_BUF_SIZE;
    if ( buf )
    {
        if ( len < HCI_DUMP_BUF_SIZE )
            copy_len = len;
        hci_event_dump_len[hci_event_dump_index] = copy_len;
        memset(hci_event_dump[hci_event_dump_index], 0, HCI_DUMP_BUF_SIZE);
        memcpy(hci_event_dump[hci_event_dump_index], buf, copy_len);
        hci_event_dump_timestamp[hci_event_dump_index] = btmtk_usb_get_microseconds();

        hci_event_dump_index--;
        if ( hci_event_dump_index < 0 )
            hci_event_dump_index = HCI_DUMP_ENTRY_NUM-1;
    }
}

static void btmtk_usb_save_hci_acl(int len, unsigned char* buf)
{
    int copy_len = HCI_DUMP_BUF_SIZE;
    if ( buf )
    {
        if ( len < HCI_DUMP_BUF_SIZE )
            copy_len = len;
        hci_acl_dump_len[hci_acl_dump_index] = copy_len&0xff;
        memset(hci_acl_dump[hci_acl_dump_index], 0, HCI_DUMP_BUF_SIZE);
        memcpy(hci_acl_dump[hci_acl_dump_index], buf, copy_len&0xff);
        hci_acl_dump_timestamp[hci_acl_dump_index] = btmtk_usb_get_microseconds();

        hci_acl_dump_index--;
        if ( hci_acl_dump_index < 0 )
            hci_acl_dump_index = HCI_DUMP_ENTRY_NUM-1;
    }
}

static void btmtk_usb_hci_dump_print_to_log(void)
{
    int counter,index,j;

    printk("btmtk_usb : HCI Command Dump\n");
    printk("    index(len)(timestamp:us) :HCI Command\n");
    index = hci_cmd_dump_index+1;
    if ( index >= HCI_DUMP_ENTRY_NUM )
        index = 0;
    for ( counter = 0 ; counter < HCI_DUMP_ENTRY_NUM ; counter++ )
    {
        if ( hci_cmd_dump_len[index] > 0 )
        {
            printk("    %d(%02d)(%u) :", counter, hci_cmd_dump_len[index], hci_cmd_dump_timestamp[index]);
            for ( j = 0 ; j < hci_cmd_dump_len[index] ; j++ )
            {
                printk("%02X ", hci_cmd_dump[index][j]);
            }
            printk("\n");
        }
        index++;
        if ( index >= HCI_DUMP_ENTRY_NUM )
            index = 0;
    }

    printk("btmtk_usb : HCI Event Dump\n");
    printk("    index(len)(timestamp:us) :HCI Event\n");
    index = hci_event_dump_index+1;
    if ( index >= HCI_DUMP_ENTRY_NUM )
        index = 0;
    for ( counter = 0 ; counter < HCI_DUMP_ENTRY_NUM ; counter++ )
    {
        if ( hci_event_dump_len[index] > 0 )
        {
            printk("    %d(%02d)(%u) :", counter, hci_event_dump_len[index], hci_event_dump_timestamp[index]);
            for ( j = 0 ; j < hci_event_dump_len[index] ; j++ )
            {
                printk("%02X ", hci_event_dump[index][j]);
            }
            printk("\n");
        }
        index++;
        if ( index >= HCI_DUMP_ENTRY_NUM )
            index = 0;
    }

    printk("btmtk_usb : HCI ACL Dump\n");
    printk("    index(len)(timestamp:us) :ACL\n");
    index = hci_acl_dump_index+1;
    if ( index >= HCI_DUMP_ENTRY_NUM )
        index = 0;
    for ( counter = 0 ; counter < HCI_DUMP_ENTRY_NUM ; counter++ )
    {
        if ( hci_acl_dump_len[index] > 0 )
        {
            printk("    %d(%02d)(%u) :", counter, hci_acl_dump_len[index], hci_acl_dump_timestamp[index]);
            for ( j = 0 ; j < hci_acl_dump_len[index] ; j++ )
            {
                printk("%02X ", hci_acl_dump[index][j]);
            }
            printk("\n");
        }
        index++;
        if ( index >= HCI_DUMP_ENTRY_NUM )
            index = 0;
    }
}
#endif

#if (SUPPORT_MT7637 || SUPPORT_MT6632)
static int btmtk_usb_io_read32_76XX(struct btmtk_usb_data *data, u32 reg, u32 *val)
{
	struct usb_device *udev = data->udev;
	unsigned char read_result[4]={0};
	int ret;
	__le16 reg_high;
	__le16 reg_low;

	reg_high = ((reg>>16)&0xffff);
	reg_low = (reg&0xffff);

	ret = usb_control_msg(udev,
		usb_rcvctrlpipe(udev, 0),
		0x63,						//bRequest
		DEVICE_VENDOR_REQUEST_IN,	//bRequestType
		reg_high,						//wValue
		reg_low,						//wIndex
		read_result,
		sizeof(read_result),
		CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0)
	{
		*val = 0xffffffff;
		printk("%s() error(%d), reg=%x, value=%x\n", __FUNCTION__, ret, reg, *val);
		return ret;
	}

	memmove(val, read_result, 4);

	*val = le32_to_cpu(*val);

	return ret;
}
#endif

void btmtk_usb_force_assert(void)
{
#if SUPPORT_FW_ASSERT_TRIGGER_KERNEL_PANIC_AND_REBOOT
    printk("%s\n", __FUNCTION__);
    BUG();
#endif
}

void btmtk_usb_toggle_rst_pin(void)
{
	printk("%s begin\n", __FUNCTION__);
	need_reset_stack = 1;
#if SUPPORT_STABLE_WOBLE_WHEN_HW_RESET
    {
        typedef void (*pdwnc_func)(unsigned char fgReset);
        char *pdwnc_func_name = "PDWNC_SetBTInResetState";
        pdwnc_func pdwndFunc = NULL;
        pdwndFunc = (pdwnc_func)kallsyms_lookup_name(pdwnc_func_name);

        if (pdwndFunc)
        {
            printk("%s - Invoke %s(%d)\n", __FUNCTION__, pdwnc_func_name, 1);
            pdwndFunc(1);
        }
        else
            printk("%s - No Exported Func Found [%s] \n", __FUNCTION__, pdwnc_func_name);
    }
#endif

#if SUPPORT_HCI_DUMP
    btmtk_usb_hci_dump_print_to_log();
#endif

#if SUPPORT_FAILED_RESET
	#if  SUPPORT_BT_RECOVER
	typedef int (*fpGpio_set)(int);

	fpGpio_set mtkGpioFunc = NULL;

	mtkGpioFunc = (fpGpio_set)kallsyms_lookup_name("MDrv_GPIO_Pull_Low");
	if(mtkGpioFunc){
		printk("[%s]-%s,Call MDrv_GPIO_Pull_Low :(%p) \n",__this_module,__FUNCTION__,mtkGpioFunc);
		mtkGpioFunc(57);
	}else{
		printk("[%s]-%s,Call MDrv_GPIO_Pull_Low :(fail!) \n",__this_module,__FUNCTION__);
	}

	mdelay(100);

	mtkGpioFunc = NULL;
	mtkGpioFunc = (fpGpio_set)kallsyms_lookup_name("MDrv_GPIO_Pull_High");
	if(mtkGpioFunc){
		printk("[%s]-%s,Call MDrv_GPIO_Pull_Hig :(%p) \n",__this_module,__FUNCTION__,mtkGpioFunc);
		mtkGpioFunc(57);
	}else{
		printk("[%s]-%s,Call MDrv_GPIO_Pull_Hig :(fail!) \n",__this_module,__FUNCTION__);
	}

	btmtk_usb_force_assert();
	#else
    printk("%s toggle GPIO %d\n", __FUNCTION__,BT_DONGLE_RESET_GPIO_PIN);
    {
        extern int mtk_gpio_direction_output(unsigned gpio, int init_value);
        mtk_gpio_direction_output(BT_DONGLE_RESET_GPIO_PIN, 0);
        mdelay(500);
        mtk_gpio_direction_output(BT_DONGLE_RESET_GPIO_PIN, 1);
    }

    btmtk_usb_force_assert();
    #endif
#else
    printk("%s : No action due to GPIO not defined.\n", __FUNCTION__);
#endif
	printk("%s end\n", __FUNCTION__);
}

void btmtk_usb_trigger_core_dump(void)
{
    if(is_assert){
      printk("%s : is_assert = %d, skip and return\n", __FUNCTION__,is_assert);
      return;
    }

#if 0
    printk("%s : Invoked by some other module (WiFi).\n", __FUNCTION__);
    btmtk_usb_send_assert_cmd(g_data->udev);
#else
    printk("%s : Invoked by some other module (WiFi).\n", __FUNCTION__);
    btmtk_usb_toggle_rst_pin();
#endif
}
EXPORT_SYMBOL(btmtk_usb_trigger_core_dump);

INT32 lock_unsleepable_lock(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
	return 0;
}

INT32 unlock_unsleepable_lock(P_OSAL_UNSLEEPABLE_LOCK pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
	return 0;
}

VOID *osal_memcpy(VOID *dst, const VOID *src, UINT32 len)
{
	return memcpy(dst, src, len);
}

static int btmtk_usb_reset(struct usb_device *udev)
{
	int ret;

	printk("%s\n", __func__);

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x01, DEVICE_VENDOR_REQUEST_OUT,
			      0x01, 0x00, NULL, 0x00, CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d)\n", __func__, ret);
		return ret;
	}

	if (ret > 0)
		ret = 0;

	return ret;
}

static int btmtk_usb_io_read32(struct btmtk_usb_data *data, u32 reg, u32 *val)
{
	u8 request = data->r_request;
	struct usb_device *udev = data->udev;
	int ret;

	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_IN,
			      0x0, reg, data->io_buf, 4, CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		*val = 0xffffffff;
		printk("%s error(%d), reg=%x, value=%x\n", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, data->io_buf, 4);

	*val = le32_to_cpu(*val);

	if (ret > 0)
		ret = 0;

	return ret;
}

static int btmtk_usb_io_write32(struct btmtk_usb_data *data, u32 reg, u32 val)
{
	u16 value, index;
	u8 request = data->w_request;
	struct usb_device *udev = data->udev;
	int ret;

	index = (u16) reg;
	value = val & 0x0000ffff;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_OUT,
			      value, index, NULL, 0, CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d), reg=%x, value=%x\n", __func__, ret, reg, val);
		return ret;
	}

	index = (u16) (reg + 2);
	value = (val & 0xffff0000) >> 16;

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_OUT,
			      value, index, NULL, 0, CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d), reg=%x, value=%x\n", __func__, ret, reg, val);
		return ret;
	}

	if (ret > 0)
		ret = 0;

	return ret;
}

#if SUPPORT_BT_ATE
static void usb_ate_hci_cmd_complete(struct urb *urb)
{
	if (urb) {
		if (urb->setup_packet)
			kfree(urb->setup_packet);
		if (urb->transfer_buffer)
			kfree(urb->transfer_buffer);
		urb->setup_packet = NULL;
		urb->transfer_buffer = NULL;
	}
}

static int usb_send_ate_hci_cmd(struct usb_device *udev, unsigned char *buf, int len)
{
	struct urb *urb;
	struct usb_ctrlrequest *class_request;
	unsigned char *hci_cmd;
	unsigned int pipe;
	int err;
	int i;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		printk("%s: allocate usb urb failed!\n", __func__);
		return -ENOMEM;
	}

	class_request = kmalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
	if (!class_request) {
		usb_free_urb(urb);
		printk("%s: allocate class request failed!\n", __func__);
		return -ENOMEM;
	}

	hci_cmd = kmalloc(len, GFP_ATOMIC);
	if (!hci_cmd) {
		usb_free_urb(urb);
		kfree(class_request);
		printk("%s: allocate hci_cmd failed!\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < len; i++) {
		hci_cmd[i] = buf[i];
	}

	class_request->bRequestType = USB_TYPE_CLASS;
	class_request->bRequest = 0;
	class_request->wIndex = 0;
	class_request->wValue = 0;
	class_request->wLength = len;

	pipe = usb_sndctrlpipe(udev, 0x0);

	usb_fill_control_urb(urb, udev, pipe, (void *)class_request,
			     hci_cmd, len, usb_ate_hci_cmd_complete, udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		kfree(urb->setup_packet);
		kfree(urb->transfer_buffer);
	} else {
		usb_mark_last_busy(udev);
	}

	usb_free_urb(urb);
	return err;
}
#endif				/* SUPPORT_BT_ATE */

#if SUPPORT_FW_DUMP
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/kthread.h>

#define FW_DUMP_BUF_SIZE (1024*512)

#if SAVE_FW_DUMP_IN_KERNEL
	static struct file *fw_dump_file = NULL;
#else
	static int	fw_dump_file = 0;
#endif

static char fw_dump_file_name[64] = {0};
int fw_dump_total_read_size = 0;
int fw_dump_total_write_size = 0;
int fw_dump_buffer_used_size = 0;
int fw_dump_buffer_full = 0;
int fw_dump_task_should_stop = 0;
u8 *fw_dump_ptr = NULL;
u8 *fw_dump_read_ptr = NULL;
u8 *fw_dump_write_ptr = NULL;
struct timeval fw_dump_last_write_time;

int fw_dump_end_checking_task_should_stop = 0;
struct urb *fw_dump_bulk_urb = NULL;

static int btmtk_usb_fw_dump_end_checking_thread(void *param)
{
	struct timeval t;
    int counter = 0;
	while (1) {
	    counter++;
		mdelay(10);

		if (kthread_should_stop() || fw_dump_end_checking_task_should_stop)
			return -ERESTARTSYS;

        if ( counter % 100 == 0 )
            printk("%s : FW dump on going ... %d:%d\n", __func__,
                    fw_dump_total_read_size, fw_dump_total_write_size);

		do_gettimeofday(&t);
		if ((t.tv_sec-fw_dump_last_write_time.tv_sec) > 3) {
			printk("%s : FW dump total read size = %d\n", __func__, fw_dump_total_read_size);
			printk("%s : FW dump total write size = %d\n", __func__, fw_dump_total_write_size);
			if (fw_dump_file) {
#if SAVE_FW_DUMP_IN_KERNEL
				vfs_fsync(fw_dump_file, 0);
				printk("%s : close file %s\n", __func__, fw_dump_file_name);
				filp_close(fw_dump_file, NULL);
#else
				//don't control file
#endif


				fw_dump_file = NULL;
			}
			fw_dump_end_checking_task_should_stop = 1;
			fw_dump_task_should_stop = 1;
			memset(fw_dump_file_name, 0, sizeof(fw_dump_file_name));
			btmtk_usb_toggle_rst_pin();
			printk("%s : stop\n", __FUNCTION__);
			return 0;
		}
	}
	return 0;
}

static int btmtk_usb_fw_dump_thread(void *param)
{
	struct btmtk_usb_data *data = param;
	mm_segment_t old_fs;
	int len;

	while (down_interruptible(&data->fw_dump_semaphore) == 0) {
		if (kthread_should_stop() || fw_dump_task_should_stop) {
			if (fw_dump_file) {
#if SAVE_FW_DUMP_IN_KERNEL
				vfs_fsync(fw_dump_file, 0);
				printk("%s : close file  %s\n", __func__, fw_dump_file_name);
				filp_close(fw_dump_file, NULL);
#else
				//don't control file
#endif

				fw_dump_file = NULL;
				printk("%s : fw dump total read size = %d\n", __func__, fw_dump_total_read_size);
				printk("%s : fw dump total write size = %d\n", __func__, fw_dump_total_write_size);
			}
			memset(fw_dump_file_name, 0, sizeof(fw_dump_file_name));
			printk("%s : stop\n", __func__);
            data->fw_dump_tsk = NULL;
			return -ERESTARTSYS;
        }

		len = fw_dump_write_ptr - fw_dump_read_ptr;

		if (len > 0 && fw_dump_read_ptr != NULL) {
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (fw_dump_file_name[0] == 0x0) {
				memset(fw_dump_file_name, 0, sizeof(fw_dump_file_name));
				snprintf(fw_dump_file_name, sizeof(fw_dump_file_name),
					        FW_DUMP_FILE_NAME"_%d", probe_counter);

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
                mutex_lock(&btmtk_usb_state_mutex);
                if (btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND)
                    btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP);
                else if (btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME)
                    btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP);
                else
                    btmtk_usb_set_state(BTMTK_USB_STATE_FW_DUMP);
                mutex_unlock(&btmtk_usb_state_mutex);
#endif

#if SAVE_FW_DUMP_IN_KERNEL
				printk("%s : open file %s\n", __func__, fw_dump_file_name);
				fw_dump_file = filp_open(fw_dump_file_name, O_RDWR | O_CREAT, 0644);

                if (IS_ERR(fw_dump_file))
                {
                    printk("%s : error occured while opening file %s.\n", __FUNCTION__, fw_dump_file_name);
                    fw_dump_file = NULL;
                }
#endif

				is_assert = 1;
				printk("%s: set current status to assert\n", __func__);
			}

			if (fw_dump_file != NULL) {
#if SAVE_FW_DUMP_IN_KERNEL
				fw_dump_file->f_op->write(fw_dump_file, fw_dump_read_ptr, len, &fw_dump_file->f_pos);
#else
				//don't control file
#endif
			}

			fw_dump_read_ptr += len;
			fw_dump_total_write_size += len;
			do_gettimeofday(&fw_dump_last_write_time);
			set_fs(old_fs);
		}

		if (fw_dump_buffer_full && fw_dump_write_ptr == fw_dump_read_ptr) {
			int err;
			fw_dump_buffer_full = 0;
			fw_dump_buffer_used_size = 0;
			fw_dump_read_ptr = fw_dump_ptr;
			fw_dump_write_ptr = fw_dump_ptr;
			printk("%s:buffer is full\n", __func__);

			err = usb_submit_urb(fw_dump_bulk_urb, GFP_ATOMIC);
			if (err < 0) {
				/* -EPERM: urb is being killed;
				* -ENODEV: device got disconnected */
				if (err != -EPERM && err != -ENODEV)
					printk("%s:urb %p failed,resubmit bulk_in_urb(%d)",
						__func__, fw_dump_bulk_urb, -err);
				usb_unanchor_urb(fw_dump_bulk_urb);
			}
		}

		if (data->fw_dump_end_check_tsk == NULL) {
			fw_dump_end_checking_task_should_stop = 0;
			data->fw_dump_end_check_tsk = kthread_create(btmtk_usb_fw_dump_end_checking_thread,
				(void *)data, "btmtk_usb_fw_dump_end_checking_thread");
			if (IS_ERR(data->fw_dump_end_check_tsk)) {
				printk("%s : create fw dump end check thread failed!\n", __func__);
				data->fw_dump_end_check_tsk = NULL;
			} else
				wake_up_process(data->fw_dump_end_check_tsk);
		}
	}

	printk("%s end : down != 0\n", __func__);
	return 0;
}
#endif

static int btmtk_usb_send_hci_suspend_cmd(struct usb_device *udev)
{
	int ret = 0;


#if defined(BT_RC_VENDOR_T1)
    {
        // yellow: gpio5; blue:gpio19
        char buf2[] = {0x6f, 0xfc, 0x38,                        //HCI header
            0x01, 0x18, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x05, 0x01, 0x00, 0x00, 0x01, 0x80, 0x64, 0x00,
            0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
            0x01, 0x80, 0x64, 0x00, 0x01, 0x80, 0x00, 0x00,
            0x13, 0x01, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80,
            0x01, 0x80, 0x01, 0x80, 0x64, 0x00, 0x01, 0x80,
            0x01, 0x80, 0x64, 0x00, 0x01, 0x80, 0x00, 0x00};
        printk("%s T1\n", __FUNCTION__, ret);
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x1, DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, buf2, 59, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0)
        {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("%s : sent WMT OK\n", __FUNCTION__);
    }
    {
        char buf[]={0xc9, 0xfc, 0x02, 0x01, 0x0c};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x05, CONTROL_TIMEOUT_JIFFIES);
    }
#elif defined(BT_RC_VENDOR_N0)
    char buf[]={0xc9, 0xfc, 0x02, 0x01, 0x0a};
    printk("%s N0\n", __FUNCTION__, ret);
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x05, CONTROL_TIMEOUT_JIFFIES);
#elif defined(BT_RC_VENDOR_S0)
    char buf[]={0xc9, 0xfc, 0x02, 0x01, 0x0b};
    printk("%s S0\n", __FUNCTION__, ret);
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x05, CONTROL_TIMEOUT_JIFFIES);
#else    // BT_RC_VENDOR_T0
    char buf[] = {0xc9, 0xfc, 0x0d, 0x01, 0x0e, 0x00, 0x05, 0x43, 0x52, 0x4b, 0x54, 0x4d, 0x20, 0x04, 0x32, 0x00};
    printk("%s T0\n", __FUNCTION__, ret);
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x10, CONTROL_TIMEOUT_JIFFIES);
#endif

    if (ret < 0)
    {
        printk("%s error1(%d)\n", __FUNCTION__, ret);
        return ret;
    }
    printk("%s : OK\n", __FUNCTION__);

	return 0;
}

static int btmtk_usb_send_assert_cmd(struct usb_device *udev)
{
    int ret = 0;
    metaMode = 1;
	ret = btmtk_usb_send_assert_cmd1(g_data->udev);
	if(ret < 0)
		ret = btmtk_usb_send_assert_cmd2(g_data->udev);
	if(ret < 0){
		printk("%s,send assert cmd fail,tigger hw reset only\n",__func__);
		btmtk_usb_toggle_rst_pin();
	}
	return ret;
}

static int btmtk_usb_send_assert_cmd1(struct usb_device *udev)
{
	int ret = 0;
	char buf[8] = { 0 };
	is_assert = 1;
	printk("%s\n", __func__);
	buf[0] = 0x6F;
	buf[1] = 0xFC;
	buf[2] = 0x05;
	buf[3] = 0x01;
	buf[4] = 0x02;
	buf[5] = 0x01;
	buf[6] = 0x00;
	buf[7] = 0x08;
	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
			0x00, 0x00, buf, 0x08, 100);
	if (ret < 0) {
		printk("%s error1(%d)\n", __func__, ret);
		return ret;
	}
	printk("%s : OK\n", __func__);
	return 0;
}

static int btmtk_usb_send_assert_cmd2(struct usb_device *udev)
{
	int ret = 0, actual_length = 9;
	char buf[9] = {0x6F, 0xFC, 0x05, 0x00, 0x01, 0x02, 0x01, 0x00, 0x08};
	is_assert = 1;
	printk("%s\n", __func__);

	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, 2), buf,0x09,&actual_length, 100);

	if (ret < 0) {
		printk("%s error1(%d)\n", __func__, ret);
		return ret;
	}
	printk("%s : OK\n", __func__);
	return 0;
}

static int btmtk_usb_send_radio_on_cmd(struct usb_device *udev)
{
	unsigned long timeout = 0;
	{
		int ret = 0;
		char buf[5] = { 0 };
		printk("%s\n", __func__);
		buf[0] = 0xC9;
		buf[1] = 0xFC;
		buf[2] = 0x02;
		buf[3] = 0x01;
		buf[4] = 0x01;
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x05, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

	printk("%s,monitor radio on complete event ,jiffies = %lu\n",__func__,jiffies);
	timeout = jiffies + msecs_to_jiffies(2000);

	do{
		int ret = 0;
		char buf[BT_MAX_EVENT_SIZE] = { 0 };
		int actual_length;
		ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
					buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

		if (ret < 0) {
			printk("%s error2(%d)\n", __func__, ret);
			return ret;
		}

		if (buf[0] == 0xE6 &&
			buf[1] == 0x02 &&
			buf[2] == 0x08 && buf[3] == 0x01) {
			printk("%s,got radio on cmd success!\n",__func__);
			return 0;
		} else {
			int i;
			printk("btmtk:unknown radio on event :\n");

			for (i = 0; i < actual_length && i < 64; i++)
				printk("%02X ", buf[i]);

			printk("\n");
		}
	}while(time_before(jiffies,timeout));

	printk("%s,got radio on cmd timeout,jiffies = %lu\n",__func__,jiffies);

	return -1;
}

static int btmtk_usb_send_radio_off_cmd(struct usb_device *udev)
{
	unsigned long timeout = 0;
	{
		int ret = 0;
		char buf[5] = { 0 };
		printk("%s\n", __func__);
		buf[0] = 0xC9;
		buf[1] = 0xFC;
		buf[2] = 0x02;
		buf[3] = 0x01;
		buf[4] = 0x00;
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x05, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

	/* Get response*/

	printk("%s,monitor radio off complete event ,jiffies = %lu\n",__func__,jiffies);
	timeout = jiffies + msecs_to_jiffies(2000);

	do {
		int ret = 0;
		char buf[BT_MAX_EVENT_SIZE] = { 0 };
		int actual_length;
		ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
					buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

		if (ret < 0) {
			printk("%s error2(%d)\n", __func__, ret);
			return ret;
		}

		if (buf[0] == 0xE6 &&
			buf[1] == 0x02 &&
			buf[2] == 0x08 && buf[3] == 0x00) {
			printk("%s,got radio off cmd success!\n",__func__);
			return 0;
		} else {
			int i;
			printk("btmtk:unknown radio off event :\n");

			for (i = 0; i < actual_length && i < 64; i++)
				printk("%02X ", buf[i]);
			}
			printk("\n");
	}while(time_before(jiffies,timeout));

	printk("%s,got radio off cmd timeout,jiffies = %lu\n",__func__,jiffies);

	return -1;
}

static int btmtk_usb_send_hci_reset_cmd(struct usb_device *udev)
{
	int retry_counter = 0;
	/* Send HCI Reset */
	{
		int ret = 0;
		char buf[4] = { 0 };
		buf[0] = 0x03;
		buf[1] = 0x0C;
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x03, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

	/* Get response of HCI reset */
	{
		while (1) {
			int ret = 0;
			char buf[BT_MAX_EVENT_SIZE] = { 0 };
			int actual_length;
			ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
						buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

			if (ret < 0) {
				printk("%s error2(%d)\n", __func__, ret);
				return ret;
            }

			if (actual_length == 6 &&
			    buf[0] == 0x0e &&
			    buf[1] == 0x04 &&
			    buf[2] == 0x01 && buf[3] == 0x03 && buf[4] == 0x0c && buf[5] == 0x00) {
				break;
			} else {
				int i;
				printk("%s drop unknown event :\n", __func__);
				for (i = 0; i < actual_length && i < 64; i++) {
					printk("%02X ", buf[i]);
				}
				printk("\n");
				mdelay(1);
				retry_counter++;
			}

			if (retry_counter > 10) {
				printk("%s retry timeout!\n", __func__);
				return ret;
			}
		}
	}

	printk("btmtk_usb_send_hci_reset_cmd : OK\n");
	return 0;
}

#define DUMMY_BULK_OUT_BUF_SIZE 8
static void btmtk_usb_send_dummy_bulk_out_packet(struct btmtk_usb_data *data)
{
    int ret = 0;
    int actual_len;
	unsigned int pipe;
    unsigned char dummy_bulk_out_fuffer[DUMMY_BULK_OUT_BUF_SIZE]={0};

	pipe = usb_sndbulkpipe(data->udev, data->bulk_tx_ep->bEndpointAddress);
    ret = usb_bulk_msg(data->udev, pipe, dummy_bulk_out_fuffer, DUMMY_BULK_OUT_BUF_SIZE, &actual_len, 100);
	if (ret)
	    printk("%s: submit dummy bulk out failed!\n", __FUNCTION__);
	else
    	printk("%s : OK\n", __FUNCTION__);

    ret = usb_bulk_msg(data->udev, pipe, dummy_bulk_out_fuffer, DUMMY_BULK_OUT_BUF_SIZE, &actual_len, 100);
	if (ret)
	    printk("%s: submit dummy bulk out failed!\n", __FUNCTION__);
	else
    	printk("%s : OK\n", __FUNCTION__);
}


#if __MTK_BT_BOOT_ON_FLASH__
//disable bt  low  power

static int btmtk_usb_send_disable_low_power_cmd(struct usb_device *udev)
{
	// Send 0x40 fc 00
	{
    	int ret = 0;
    	char buf[3] = {0x40, 0xfc, 0x00};
    	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, sizeof(buf), CONTROL_TIMEOUT_JIFFIES);

    	if (ret < 0)
    	{
    		printk("%s error1(%d)\n", __FUNCTION__, ret);
    		return ret;
    	}
	}


    // Get response
    {
        int retry_counter = 0;
        while (1)
        {
            int ret=0;
            unsigned char buf[BT_MAX_EVENT_SIZE] = {0};
            int actual_length;
            ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                    buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

            if (ret <0)
            {
    	    	printk("%s error2(%d)\n", __FUNCTION__, ret);
    		    return ret;
            }

            if ( actual_length == 6 &&
                    buf[0] == 0x0e &&
                    buf[1] == 0x04 &&
                    buf[2] == 0x01 &&
                    buf[3] == 0x40 &&
                    buf[4] == 0xfc &&
                    buf[5] == 0x00 )
            {
                break;
            }
            else
            {
                int i;
                printk("%s : drop unknown event : \n", __FUNCTION__);
                for ( i = 0 ; i < actual_length && i < 64 ; i++ )
                {
                    printk("%02X ", buf[i]);
                }
                printk("\n");
                mdelay(10);
                retry_counter++;
            }

            if ( retry_counter > 10 )
            {
                printk("%s retry timeout!\n", __FUNCTION__);
                return ret;
            }
        }
    }

	printk("%s : OK\n", __FUNCTION__);
	return 0;
}


static int btmtk_usb_read_dummy_event_2(struct usb_device *udev)
{
    // use usb_interrupt_msg to see if have unexpected hci event reset
    {
        int ret = 0;
        char buf[BT_MAX_EVENT_SIZE] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

        if (ret <0)
        {
    		printk("%s no dummy event ret(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
		printk("get unexpect dummy event_2, actual_length:%d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
    	            actual_length, buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
    }
	return 0;
}

static int btmtk_usb_read_dummy_event(struct usb_device *udev)
{
    // use usb_interrupt_msg to see if have unexpected hci event reset
    {
        int ret = 0;
        char buf[BT_MAX_EVENT_SIZE] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, BT_MAX_EVENT_SIZE, &actual_length, 20);

        if (ret <0)
        {
    		BT_DBG("%s no dummy event ret(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
		printk("get unexpect dummy event, actual_length:%d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
    	            actual_length, buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
		return ret;
    }
}

static int btmtk_usb_get_flash_patch_version(struct btmtk_usb_data *data)
{
    char result_buf[BT_MAX_EVENT_SIZE] = {0};
	u8 year1 = 0, year2 = 0, month = 0, day = 0;

	data->patch_version_all_ff = 0;
	btmtk_usb_read_dummy_event(data->udev);//read dummy hci event
    // Send fcd1 cmd to read flash patch_version flag address: 0x00503ffc
    {
    	int ret = 0;
    	char buf[8] = {0xd1, 0xFC, 0x04, 0xfc, 0x3f, 0x50, 0x00};
    	ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, 0x07, 100);

    	if (ret < 0)
    	{
    		printk("%s error1(%d)\n", __FUNCTION__, ret);
    		return ret;
    	}
	}

    // Get response
    {
        int ret = 0;
        int actual_length;
        ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
                                result_buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

        if (ret <0)
        {
    		printk("%s error2(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
        else
        {
            printk("len = %d\n", actual_length);
            printk("Current flash patch version : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                        result_buf[0],
                        result_buf[1],
                        result_buf[2],
                        result_buf[3],
                        result_buf[4],
                        result_buf[5],
                        result_buf[6],
                        result_buf[7],
                        result_buf[8],
                        result_buf[9]);
        }
		//sample result log
		//Current rom patch version : 0E 08 01 D1 FC 00 11 09 14 20
		year1 = (result_buf[9]/16)*10 + result_buf[9]%16;
		year2 = (result_buf[8]/16)*10 + result_buf[8]%16;
		month = (result_buf[7]/16)*10 + result_buf[7]%16;
		day   = (result_buf[6]/16)*10 + result_buf[6]%16;

		printk("year1:%d, year2:%d, month:%d, day:%d\n", year1, year2, month, day);
		data->flash_patch_version = year1*1000000 + year2*10000 + month*100 + day;

		if( result_buf[9] == 0xff && result_buf[8] == 0xff && result_buf[7] == 0xff && result_buf[6] == 0xff )
		{
			data->patch_version_all_ff = 1;
			printk("flash_patch_version all ff: %d\n", data->patch_version_all_ff);
 		} else {
			data->patch_version_all_ff = 0 ;

			if(year1 != 20 || year2 > 25 || year2 < 10){
				printk("flash patch version'year is not between 2010 to 2025\n");
				return -1;
			}

			if(data->flash_patch_version != data->driver_patch_version)
				data->patch_version_all_ff = 2;
		}

		printk("flash_patch_version not all ff: %d\n", data->patch_version_all_ff);
		printk("btmtk_usb_get_flash_patch_version calculated flash_patch_version is : %d\n", data->flash_patch_version);

    }
	return 0;
}
/*
function name: btmtk_usb_load_data_to_flash
parameters: data, phase, dest, sent_len, write_flash, data_part, cur_len

data:	btmtk_usb_data structure
phase 0: start, 1: continue, 2: end
dest, 0: bootloader, 1: patch, 2: version, 3: loader flag
sent_len: current sent length
cur_len: current length of image
write_flash: 0 do not write flash, 1 write flash
data_part: 0, 1, 2, 3 means first second third fourth part

struct patch_down_info
{
	u8 phase;
	u8 dest;
	u8 cur_len;
	u8 sent_len;
	u8 single_len;
	u8 write_flash;
	u8 data_part;
};
*/
//static int btmtk_usb_load_data_to_flash(struct btmtk_usb_data *data, struct patch_down_info)
static int btmtk_usb_load_data_to_flash(struct btmtk_usb_data *data, struct patch_down_info patch_info)
{

	static u16 sent_len = 0;
	u16 back_len = 0;
	static u16 sent_time = 0;

	if(patch_info.phase == 0 && patch_info.write_flash == 0){
		sent_len = 0;
		sent_time = 0;
		printk("btmtk_usb_load_data_to_flash begin and reset data\n");
	}

    {
    	int ret = 0 ;
    	char buf[256] = {0} ;

		/* prepare HCI header */
		buf[0] = 0xEE;
		buf[1] = 0xFC;
		buf[2] = patch_info.single_len + 5;
		buf[3] = patch_info.dest;

		buf[4] = patch_info.write_flash;
		buf[5] = patch_info.data_part;
		buf[6] = patch_info.phase;

		buf[7] = patch_info.single_len;
		memcpy(&buf[8], data->rom_patch + PATCH_INFO_SIZE + patch_info.cur_len, patch_info.single_len);

    	ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, patch_info.single_len + 8, 100);
		sent_time++;

    	if (ret < 0)
    	{
    		printk("%s error1(%d),sent_time(%d)\n", __FUNCTION__, ret,sent_time);
			sent_len=0;
    		return ret;
    	}
		sent_len = sent_len + patch_info.single_len;//record the sent length
	}
	{

		//TODO: do I need to wait for a moment?
		//printk("btmtk_usb_load_data_to_flash, write_flash == 1\n");
	    int ret = 0;
	    char buf[BT_MAX_EVENT_SIZE] = {0};
	    int actual_length;
	    ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
	                            buf, BT_MAX_EVENT_SIZE, &actual_length, 4000);

	    if (ret <0)
	    {
			printk("%s error2(%d),sent count(%d)\n", __FUNCTION__, ret,sent_time);
			sent_len = 0;
			return ret;
	    }

		back_len = buf[6] + buf[7]*256;//is this like this
		if( patch_info.write_flash == 1&&buf[0] == 0x0e && buf[1] == 0x06 && buf[2] == 0x01 && buf[3] == 0xee && buf[4] == 0xfc && buf[5] == 0x00 && (sent_len == back_len))
		{
			//printk("load data to flash success OK\n");
			sent_len = 0;
		} else {

			if(patch_info.write_flash != 0||buf[0] != 0x0e || buf[1] != 0x04 || buf[2] != 0x01 || buf[3] != 0xee || buf[4] != 0xfc || buf[5] != 0x00){

				printk("load data to flash: NG response, error,sent_time(%d)\n",sent_time);
				printk("btmtk_usb_load_data_to_flash event actual length %d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n", \
				actual_length, buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
				printk("phase:%d, dest:%d, cur_len:%d, sent_len:%d, single_len:%d, write_patch:%d, data_part:%d\n", \
				patch_info.phase, patch_info.dest, patch_info.cur_len, patch_info.sent_len, patch_info.single_len, \
				patch_info.write_flash, patch_info.data_part);
				printk("btmtk_usb_load_data_to_flash sent_len=%d,back_len=%d,write_flash=%d\n",sent_len,back_len,patch_info.write_flash);
				sent_len = 0;
				return -1;
			}
		}
	}
	return 0;
}

static int btmtk_usb_write_patch_version(struct btmtk_usb_data *data)
{
	printk("btmtk_usb_write_patch_version++++\n");

	btmtk_usb_read_dummy_event(data->udev);//wenhui add to read dummy hci event
    {
    	int ret = 0;
    	char buf[24] = {0};
    	buf[0] = 0xEE;
    	buf[1] = 0xFC;
		buf[2] = 0x09;
    	buf[3] = 0x02;
		buf[4] = 0x01;
    	buf[5] = 0x00;
		buf[6] = 0x00;
    	buf[7] = 0x04;
		buf[8] = data->driver_version[3];
		buf[9] = data->driver_version[2];
		buf[10] = data->driver_version[1];
		buf[11] = data->driver_version[0];

		printk("buf8~11: %02X, %02X, %02X, %02X\n",buf[8], buf[9], buf[10], buf[11]);
    	ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, 12, 100);

    	if (ret < 0)
    	{
    		printk("%s error1(%d)\n", __FUNCTION__, ret);
    		return ret;
    	}
	}

    // Get response of flash version request
    {
        int ret = 0;
        char buf[BT_MAX_EVENT_SIZE] = {0};
        int actual_length;
        ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
                                buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

        if (ret <0)
        {
    		printk("%s error2(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
		printk("btmtk_usb_write_patch_version, event: 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
    	            buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);

		if( buf[0] == 0x0e && buf[1] == 0x06 && buf[2] == 0x01 && buf[3] == 0xee && buf[4] == 0xfc && buf[5] == 0x00 && buf[6] == 0x04 && buf[7] == 0x00)
		{
			printk("write patch version success!\n");
 		} else {
 			printk("write patch version get NG response!\n");
		}
	}

	printk("btmtk_usb_write_patch_version----\n");
	return 0;
}

static int btmtk_usb_read_patch_crc(struct usb_device *udev, u32 patch_len)
{
    char result_buf[BT_MAX_EVENT_SIZE] = {0};
	char patchLen[5] = {0};

	btmtk_usb_read_dummy_event(udev);//wenhui add to read dummy hci event
    {
    	int ret = 0;
		char buf[12] = {0};
		patchLen[0] = patch_len&0xFF;
		patchLen[1] = (patch_len>>8)&0xFF;
		patchLen[2] = (patch_len>>16)&0xFF;
		patchLen[3] = (patch_len>>24)&0xFF;

		printk("patch_len: 0x%08X\n", patch_len);

		printk("patchLen 0~3: %02X, %02X, %02X, %02X\n", patchLen[0], patchLen[1], patchLen[2], patchLen[3]);

		buf[0] = 0x00;
		buf[1] = 0xfd;
		buf[2] = 0x08;
		buf[3] = 0x00;
		buf[4] = 0x40;
		buf[5] = 0x50;
		buf[6] = 0x00;
		buf[7] = patchLen[0];
		buf[8] = patchLen[1];
		buf[9] = patchLen[2];
		buf[10] = patchLen[3];

    	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, 11, 100);

    	if (ret < 0)
    	{
    		printk("%s error1(%d)\n", __FUNCTION__, ret);
    		return ret;
    	}
	}

    // Get response of Crc Request
    {
        int ret=0;
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                result_buf, BT_MAX_EVENT_SIZE, &actual_length, 5000);

        if (ret <0)
        {
    		printk("%s error2(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
        else
        {
            printk("btmtk_usb_read_patch_crc: len: %d, ret : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
						actual_length,
                        result_buf[0],
                        result_buf[1],
                        result_buf[2],
                        result_buf[3],
                        result_buf[4],
                        result_buf[5],
                        result_buf[6],
                        result_buf[7],
                        result_buf[8],
                        result_buf[9]);
        }
		if(result_buf[0] == 0x0e && result_buf[1] == 0x06 && result_buf[2] == 0x01 && result_buf[3] == 0 && result_buf[4] == 0xfd && result_buf[5] == 0x00 && result_buf[6] == rom_patch_crc[0] && result_buf[7] == rom_patch_crc[1])
		{
			printk("flash crc is the same with driver crc\n");
			return 0;
		} else {
			printk("get NG result, return state: %d, crc: %02x, %02x\n", result_buf[5], result_buf[6], result_buf[7]);
			return -1;
		}
    }

	return 0;
}

static int btmtk_usb_load_patch_to_flash(struct btmtk_usb_data *data)
{
	u32 patch_len = 0;
	int first_block = 1;

	int ret = 0;
	int crc_ret =0;
	int i = 0 , count = 0 ,tail = 0;
	struct patch_down_info patch_info;

	patch_info.phase = 0;
	patch_info.dest = 1;  //flash set to 1
	patch_info.cur_len = 0;
	patch_info.sent_len = 0;
	patch_info.write_flash = 0;
	patch_info.data_part = 0;
	patch_info.single_len = 0;
	patch_info.cur_len = 0x00;
	patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
	printk("btmtk_usb_load_patch_to_flash+++++++\n");
	while (1)
	{
		s32 sent_len_max = FLASH_MAX_UNIT;
		patch_info.sent_len = (patch_len - patch_info.cur_len) >= sent_len_max ? sent_len_max : (patch_len - patch_info.cur_len);

		if (patch_info.sent_len > 0)
		{
			if (first_block == 1)
			{
				if (patch_info.sent_len < sent_len_max)
					patch_info.phase = FLASH_PATCH_PHASE3;
				else
					patch_info.phase = FLASH_PATCH_PHASE1;
				first_block = 0;
			}
			else if (patch_info.sent_len == sent_len_max)
			{
				patch_info.phase = FLASH_PATCH_PHASE2;
			}
			else
			{
				patch_info.phase = FLASH_PATCH_PHASE3;
			}

			i=0;
			//printk("download phase = %d\n", patch_info.phase);

			count = patch_info.sent_len/FLASH_SINGLE_UNIT;
			tail = 0;
			if((patch_info.phase == FLASH_PATCH_PHASE1) || (patch_info.phase == FLASH_PATCH_PHASE2)) {
				//directly download FLASH_MAX_UNIT bytes to flash
				for(i=0; i<(FLASH_MAX_UNIT/FLASH_SINGLE_UNIT); i++) {
					if(i == (FLASH_MAX_UNIT/FLASH_SINGLE_UNIT-1))
						patch_info.write_flash = 1;
					else
						patch_info.write_flash = 0;

					patch_info.data_part = i;
					patch_info.single_len = 128;

					ret = btmtk_usb_load_data_to_flash(data, patch_info);

					if(ret !=0){
						printk("load_data_to_flash phase:%d  ret:%d\n",patch_info.phase,ret);
						return -1;//return value
					}
					patch_info.cur_len += 128;
				}

			} else if (patch_info.phase == FLASH_PATCH_PHASE3) {


			if(patch_len < FLASH_MAX_UNIT)//if patch_len less than FLASH_MAX_UNIT, then, set to phase1 to trigger erase flash
				patch_info.phase = FLASH_PATCH_PHASE1;

				count = patch_info.sent_len/FLASH_SINGLE_UNIT;
				tail  = patch_info.sent_len%FLASH_SINGLE_UNIT;
				//printk("phase: %d, count:%d, tail:%d\n", patch_info.phase, count, tail);

				for(i=0; i<count; i++)
				{
					patch_info.single_len = 128;
					patch_info.data_part = i;

					if((i == count-1) && (tail == 0))
					{
						patch_info.write_flash = 1;

						ret = btmtk_usb_load_data_to_flash(data, patch_info);

						if(ret !=0){
							printk("load_data_to_flash phase:%d  ret:%d\n",patch_info.phase,ret);
							return -1;//return value
						}

					} else {
						patch_info.write_flash = 0;

						ret = btmtk_usb_load_data_to_flash(data, patch_info);

						if(ret !=0){
							printk("load_data_to_flash phase:%d  ret:%d\n",patch_info.phase,ret);
							return -1;//return value
						}

					}
					patch_info.cur_len += 128;
				}

				if(i == 0)//means sent_len < 128
				{
					patch_info.data_part = i;
					patch_info.write_flash = 1;
					patch_info.single_len = patch_info.sent_len;

					ret = btmtk_usb_load_data_to_flash(data, patch_info);

					if(ret !=0){
						printk("load_data_to_flash phase:%d  ret:%d\n",patch_info.phase,ret);
						return -1;//return value
					}

					patch_info.cur_len += patch_info.sent_len;
				}
				if((tail != 0)&&(i != 0))// write the tail part
				{
					patch_info.data_part = i;
					patch_info.write_flash = 1;
					patch_info.single_len = tail;
					ret = btmtk_usb_load_data_to_flash(data, patch_info);

					if(ret !=0){
						printk("load_data_to_flash phase:%d  ret:%d\n",patch_info.phase,ret);
						return -1;//return value
					}

					patch_info.cur_len += tail;
				}

			}
		}
		else
		{
			break;
		}
	}

	btmtk_usb_read_dummy_event_2(data->udev);

	crc_ret =btmtk_usb_read_patch_crc(data->udev, patch_len);
	if (BT_STATUS_SUCCESS != crc_ret)
		return BT_STATUS_FLASH_CRC_ERROR;

	btmtk_usb_write_patch_version(data);//write patch version to flash
	printk("btmtk_usb_load_patch_to_flash-------\n");
	return 0;
}

#endif    //__MTK_BT_BOOT_ON_FLASH__

static int btmtk_usb_send_hci_set_ce_cmd(struct usb_device *udev)
{
	char result_buf[BT_MAX_EVENT_SIZE] = { 0 };
	BT_DBG("send hci reset cmd");

	/* Send HCI Reset, read 0x41070c */
	{
		int ret = 0;
		char buf[8] = { 0xd1, 0xFC, 0x04, 0x0c, 0x07, 0x41, 0x00 };
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x07, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

	/* Get response of HCI reset */
	{
		int ret = 0;
		int actual_length;
		ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
					result_buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

		if (ret < 0) {
			printk("%s error2(%d)\n", __func__, ret);
			return ret;
		} else {
			if (result_buf[6] & 0x01) {
				printk("warning, 0x41070c[0] is 1!\n");
			}
		}
	}

	/* Send HCI Reset, write 0x41070c[0] to 1 */
	{
		int ret = 0;
		char buf[12] = { 0xd0, 0xfc, 0x08, 0x0c, 0x07, 0x41, 0x00 };
		buf[7] = result_buf[6] | 0x01;
		buf[8] = result_buf[7];
		buf[9] = result_buf[8];
		buf[10] = result_buf[9];
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x0b, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

#if __MTK_BT_BOOT_ON_FLASH__
		// Get response of write cmd
		{
			int ret = 0;
			int actual_length;
			ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
									result_buf, 64, &actual_length, 2000);

			if (ret <0)
			{
				printk("%s error2(%d)\n", __FUNCTION__, ret);
				return ret;
			}
			else
			{
				printk("btmtk_usb_send_hci_set_ce_cmd,	read response: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", \
				result_buf[0], result_buf[1], result_buf[2], result_buf[3], result_buf[4], result_buf[5], result_buf[6], result_buf[7], result_buf[8], result_buf[9], result_buf[10] );
				if (  result_buf[6] & 0x01 )
				{
					printk("warning, 0x41020c[0] is 1!\n");
				}
			}
		}
#endif
	printk("btmtk_usb_send_hci_set_ce_cmd : OK\n");

	return 0;
}


static int btmtk_usb_send_check_rom_patch_result_cmd(struct usb_device *udev)
{
	/* Send HCI Reset */
	{
		int ret = 0;
		unsigned char buf[8] = { 0 };
		buf[0] = 0xD1;
		buf[1] = 0xFC;
		buf[2] = 0x04;
		buf[3] = 0x00;
		buf[4] = 0xE2;
		buf[5] = 0x40;
		buf[6] = 0x00;


		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
				      0x00, 0x00, buf, 0x07, 100);

		if (ret < 0) {
			printk("%s error1(%d)\n", __func__, ret);
			return ret;
		}
	}

	/* Get response of HCI reset */
	{
		int ret = 0;
		unsigned char buf[BT_MAX_EVENT_SIZE] = { 0 };
		int actual_length;
		ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
					buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

		if (ret < 0) {
			printk("%s error2(%d)\n", __func__, ret);
			return ret;
		}
		printk("Check rom patch result : ");

		if (buf[6] == 0 && buf[7] == 0 && buf[8] == 0 && buf[9] == 0) {
			printk("NG\n");
		} else {
			printk("OK\n");
		}
	}

	return 0;
}

static int btmtk_usb_switch_iobase(struct btmtk_usb_data *data, int base)
{
	int ret = 0;

	switch (base) {
	case SYSCTL:
		data->w_request = 0x42;
		data->r_request = 0x47;
		break;
	case WLAN:
		data->w_request = 0x02;
		data->r_request = 0x07;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}





u16 checksume16(u8 *pData, int len)
{
	int sum = 0;

	while (len > 1) {
		sum += *((u16 *) pData);

		pData = pData + 2;

		if (sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);

		len -= 2;
	}

	if (len)
		sum += *((u8 *) pData);

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~sum;
}

static int btmtk_usb_chk_crc(struct btmtk_usb_data *data, u32 checksum_len)
{
	int ret = 0;
	struct usb_device *udev = data->udev;

	BT_DBG("%s", __func__);

	memmove(data->io_buf, &data->rom_patch_offset, 4);
	memmove(&data->io_buf[4], &checksum_len, 4);

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x1, DEVICE_VENDOR_REQUEST_OUT,
			      0x20, 0x00, data->io_buf, 8, CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d)\n", __func__, ret);
	}

	return ret;
}

static u16 btmtk_usb_get_crc(struct btmtk_usb_data *data)
{
	int ret = 0;
	struct usb_device *udev = data->udev;
	u16 crc, count = 0;

	BT_DBG("%s", __func__);

	while (1) {
		ret =
		    usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), 0x01, DEVICE_VENDOR_REQUEST_IN,
				    0x21, 0x00, data->io_buf, 2, CONTROL_TIMEOUT_JIFFIES);

		if (ret < 0) {
			crc = 0xFFFF;
			printk("%s error(%d)\n", __func__, ret);
		}

		memmove(&crc, data->io_buf, 2);

		crc = le16_to_cpu(crc);

		if (crc != 0xFFFF)
			break;

		mdelay(1);

		if (count++ > 100) {
			printk("Query CRC over %d times\n", count);
			break;
		}
	}

	return crc;
}

static int btmtk_usb_reset_wmt(struct btmtk_usb_data *data)
{
	int ret = 0;

	/* reset command */
	u8 cmd[9] = { 0x6F, 0xFC, 0x05, 0x01, 0x07, 0x01, 0x00, 0x04 };

	memmove(data->io_buf, cmd, 8);


	ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x01,
			      DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, data->io_buf, 8,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s:Err1(%d)\n", __func__, ret);
		return ret;
	}

	mdelay(20);

	ret = usb_control_msg(data->udev, usb_rcvctrlpipe(data->udev, 0), 0x01,
			      DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, data->io_buf, 7,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s Err2(%d)\n", __func__, ret);
	}

	if (data->io_buf[0] == 0xe4 &&
	    data->io_buf[1] == 0x05 &&
	    data->io_buf[2] == 0x02 &&
	    data->io_buf[3] == 0x07 &&
	    data->io_buf[4] == 0x01 && data->io_buf[5] == 0x00 && data->io_buf[6] == 0x00) {
		printk("Check reset wmt result : OK\n");
	} else {
		printk("Check reset wmt result : NG\n");
	}


	return ret;
}

static u16 btmtk_usb_get_rom_patch_result(struct btmtk_usb_data *data)
{
	int ret = 0;

	ret = usb_control_msg(data->udev, usb_rcvctrlpipe(data->udev, 0), 0x01,
			      DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, data->io_buf, 7,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d)\n", __func__, ret);
	}

	if (data->io_buf[0] == 0xe4 &&
	    data->io_buf[1] == 0x05 &&
	    data->io_buf[2] == 0x02 &&
	    data->io_buf[3] == 0x01 &&
	    data->io_buf[4] == 0x01 && data->io_buf[5] == 0x00 && data->io_buf[6] == 0x00) {
		printk("Get rom patch result : OK\n");
	} else {
		printk("Get rom patch result : NG\n");
	}

	return ret;
}

static void load_code_from_bin(unsigned char **image, char *bin_name, struct device *dev,
			       u32 *code_len)
{
	const struct firmware *fw_entry;
	int err = 0;

  if (g_rom_patch_image && g_rom_patch_code_len){
      printk("%s : no need to request firmware again.\n", __FUNCTION__);
      *image = g_rom_patch_image;
      *code_len = g_rom_patch_code_len;
      return;
  }

	err = request_firmware(&fw_entry, bin_name, dev);
	if (err != 0) {
		*image = NULL;
		printk("%s: request_firmware fail!! error code = %d", __func__, err);
		return;
	}

	*image = kmalloc(fw_entry->size, GFP_ATOMIC);
	memcpy(*image, fw_entry->data, fw_entry->size);
	*code_len = fw_entry->size;

	g_rom_patch_image = *image;
  	g_rom_patch_code_len = *code_len;

	release_firmware(fw_entry);
}

static void load_rom_patch_complete(struct urb *urb)
{

	struct completion *sent_to_mcu_done = (struct completion *)urb->context;

	complete(sent_to_mcu_done);
}

static void btmtk_usb_cap_init(struct btmtk_usb_data *data)
{
	btmtk_usb_io_read32(data, 0x00, &data->chip_id);

#if (SUPPORT_MT7637 || SUPPORT_MT6632)
	if (data->chip_id == 0x0)
		btmtk_usb_io_read32_76XX(data, 0x80021008, &data->chip_id);
#endif

	printk("chip id = %x\n", data->chip_id);

	if (is_mt7630(data) || is_mt7650(data)) {
		data->need_load_fw = 1;
		data->need_load_rom_patch = 0;
		data->fw_header_image = NULL;
		data->fw_bin_file_name = "mtk/mt7650.bin";
		data->fw_len = 0;
	} else if (is_mt7662T(data) || is_mt7632T(data)) {
        printk("btmtk:This is 7662T chip\n");
        data->need_load_fw = 0;
        data->need_load_rom_patch = 1;
        if (LOAD_CODE_METHOD == HEADER_METHOD)
        {
            data->rom_patch_header_image = mt7662_rom_patch;
            data->rom_patch_len = sizeof(mt7662_rom_patch);
            data->rom_patch_offset = 0xBC000;
        }
        else
        {
            data->rom_patch_bin_file_name = kmalloc(32, GFP_ATOMIC);
            if (!data->rom_patch_bin_file_name)
            {
                printk("%s: Can't allocate memory (32)\n", __FUNCTION__);
                return;
            }
            memset(data->rom_patch_bin_file_name, 0, 32);

            if ( (data->chip_id & 0xf) < 0x2 )
                memcpy(data->rom_patch_bin_file_name, "mt7662t_patch_e1_hdr.bin", 24);
            else
                memcpy(data->rom_patch_bin_file_name, "mt7662t_patch_e3_hdr.bin", 24);

            data->rom_patch_offset = 0xBC000;
            data->rom_patch_len = 0;
        }
    } else if (is_mt7632(data) || is_mt7662(data)) {
		data->need_load_fw = 0;
		data->need_load_rom_patch = 1;
		if (LOAD_CODE_METHOD == HEADER_METHOD) {
			data->rom_patch_header_image = mt7662_rom_patch;
			data->rom_patch_len = sizeof(mt7662_rom_patch);
			data->rom_patch_offset = 0x90000;
		} else {
			data->rom_patch_bin_file_name = kmalloc(32, GFP_ATOMIC);
			if (!data->rom_patch_bin_file_name) {
				printk("btmtk_usb_cap_init(): Can't allocate memory (32)\n");
				return;
			}
			memset(data->rom_patch_bin_file_name, 0, 32);

			if ((data->chip_id & 0xf) < 0x2) {
				printk("chip_id < 0x2\n");
				memcpy(data->rom_patch_bin_file_name, FW_PATCH, 23);
				/* memcpy(data->rom_patch_bin_file_name, "mt7662_patch_e1_hdr.bin", 23); */
			} else {
				printk("chip_id >= 0x2\n");
				printk("patch bin file name:%s\n", FW_PATCH);
				memcpy(data->rom_patch_bin_file_name, FW_PATCH, 23);
			}

			data->rom_patch_offset = 0x90000;
			data->rom_patch_len = 0;
		}

	}
#if SUPPORT_MT7637
	else if (data->chip_id == 0x7637) {
		data->need_load_fw = 0;
		data->need_load_rom_patch = 1;
		if (LOAD_CODE_METHOD == HEADER_METHOD) {
			data->rom_patch_header_image = mt7637_rom_patch;
			data->rom_patch_len = sizeof(mt7637_rom_patch);
			data->rom_patch_offset = 0x2000000;
		} else {
			data->rom_patch_bin_file_name = kmalloc(32, GFP_ATOMIC);
			if (!data->rom_patch_bin_file_name) {
				printk("btmtk_usb_cap_init(): Can't allocate memory (32)\n");
				return;
			}
			memset(data->rom_patch_bin_file_name, 0, 32);
			memcpy(data->rom_patch_bin_file_name, "mt7637_patch_e1_hdr.bin", 23);
			data->rom_patch_offset = 0x2000000;
			data->rom_patch_len = 0;
		}

	}
#endif

#if SUPPORT_MT6632
	else if (data->chip_id == 0x6632) {
		data->need_load_fw = 0;
		data->need_load_rom_patch = 1;
		if (LOAD_CODE_METHOD == HEADER_METHOD) {
			data->rom_patch_header_image = mt6632_rom_patch;
			data->rom_patch_len = sizeof(mt6632_rom_patch);
			data->rom_patch_offset = 0x2000000;
		} else {
			data->rom_patch_bin_file_name = kmalloc(32, GFP_ATOMIC);
			if (!data->rom_patch_bin_file_name) {
				printk("btmtk_usb_cap_init(): Can't allocate memory (32)\n");
				return;
			}
			memset(data->rom_patch_bin_file_name, 0, 32);
			memcpy(data->rom_patch_bin_file_name, "mt6632_patch_e1_hdr.bin", 23);
			data->rom_patch_offset = 0x2000000;
			data->rom_patch_len = 0;
		}
	}
#endif
	else {
		printk("unknow chip(%x)\n", data->chip_id);
	}
}


#if (SUPPORT_MT7637 || SUPPORT_MT6632)

static int btmtk_usb_send_power_on_cmd(struct btmtk_usb_data *data)
{
	int ret = 0;

	/* reset command */
	u8 cmd[10] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01 };

	memmove(data->io_buf, cmd, 10);

	ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x00,
			      DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, data->io_buf, 9,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s:Err1(%d)\n", __func__, ret);
		return ret;
	}

	mdelay(1000);

	ret = usb_control_msg(data->udev, usb_rcvctrlpipe(data->udev, 0), 0x01,
			      DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, data->io_buf, 7,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s Err2(%d)\n", __func__, ret);
	}

	if (data->io_buf[0] == 0xe4 && data->io_buf[1] == 0x05 && data->io_buf[2] == 0x02 && data->io_buf[3] == 0x06 &&
	    data->io_buf[4] == 0x01 && data->io_buf[5] == 0x00 && data->io_buf[6] == 0x00) {
		printk("%s : OK\n", __FUNCTION__);
	} else {
		printk("%s : NG\n", __FUNCTION__);
	}

	return ret;
}


static int btmtk_usb_load_rom_patch_763x(struct btmtk_usb_data *data)
{
	u32 loop = 0;
	u32 value;
	s32 sent_len;
	int ret = 0;//, total_checksum = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	unsigned char phase;
	void *buf;
	char *pos;
	char *tmp_str;
	unsigned int pipe = usb_sndbulkpipe(data->udev,
					    data->bulk_tx_ep->bEndpointAddress);

	printk("%s() btmtk_usb_load_rom_patch begin\n", __FUNCTION__);
 load_patch_protect:
	//if (data->chip_id==0x7636 || data->chip_id==0x7637)//scofield
	{
		btmtk_usb_io_read32_76XX(data, 0x80000600, &value);//commmon register
		btmtk_usb_io_read32_76XX(data, 0x80000610, &value);//semaphore
	}
	//else {//scofield
	//btmtk_usb_switch_iobase(data, WLAN);
	//btmtk_usb_io_read32(data, SEMAPHORE_03, &value);
	//}//scofield
	loop++;

	if ((value & 0x01) == 0x00) {
		if (loop < 1000) {
			mdelay(1);
			goto load_patch_protect;
		} else {
			printk("btmtk_usb_load_rom_patch ERR! Can't get semaphore! Continue\n");
		}
	}

	btmtk_usb_switch_iobase(data, SYSCTL);

	btmtk_usb_io_write32(data, 0x1c, 0x30);

	btmtk_usb_switch_iobase(data, WLAN);

	/* check ROM patch if upgrade */
	if ((MT_REV_GTE(data, mt7662, REV_MT76x2E3)) || (MT_REV_GTE(data, mt7632, REV_MT76x2E3))) {
		btmtk_usb_io_read32(data, CLOCK_CTL, &value);
		if ((value & 0x01) == 0x01)
		{
		    printk("%s : no need to load rom patch\n", __FUNCTION__);
		    if ( rom_patch_version[0] != 0 )
    		    printk("%s : FW version = %s\n", __FUNCTION__, rom_patch_version);
#if SUPPORT_CHECK_WAKE_UP_REASON_DECIDE_SEND_HCI_RESET
            {
                extern unsigned int PDWNC_ReadWakeupReason(void);
                if ( PDWNC_ReadWakeupReason() == 2 )
                {
                    printk("%s : Don't send hci_reset due to wake-up reason is 2!\n", __FUNCTION__);
                }
                else
                {
                    btmtk_usb_send_hci_reset_cmd(data->udev);
                }
            }
#else
		    btmtk_usb_send_hci_reset_cmd(data->udev);
#endif

#if SUPPORT_SEND_DUMMY_BULK_OUT_AFTER_RESUME
		    btmtk_usb_send_dummy_bulk_out_packet(data);
		    btmtk_usb_send_dummy_bulk_out_packet(data);
#endif
			goto error0;
		}
	} else {
		//btmtk_usb_io_read32(data, COM_REG0, &value);
		btmtk_usb_io_read32_76XX(data, 0x80000600, &value);//commmon register
		//if ((value & 0x02) == 0x02)
		if ((value & 0x01) == 0x01)
		{
		    printk("%s : no need to load rom patch\n", __FUNCTION__);
		    if ( rom_patch_version[0] != 0 )
    		    printk("%s : FW version = %s\n", __FUNCTION__, rom_patch_version);
#if SUPPORT_CHECK_WAKE_UP_REASON_DECIDE_SEND_HCI_RESET
            {
                extern unsigned int PDWNC_ReadWakeupReason(void);
                if ( PDWNC_ReadWakeupReason() == 2 )
                {
                    printk("%s : Don't send hci_reset due to wake-up reason is 2!\n", __FUNCTION__);
                }
                else
                {
                    btmtk_usb_send_hci_reset_cmd(data->udev);
                }
            }
#else
			/* CHIP_RESET */
			ret = btmtk_usb_reset_wmt(data);

			mdelay(20);
			if (data->chip_id != 0x7636)
				btmtk_usb_send_power_on_cmd(data);

			mdelay(20);
			/* BT_RESET */
			btmtk_usb_send_hci_reset_cmd(data->udev);

			/* for WoBLE/WoW low power */
			btmtk_usb_send_hci_set_ce_cmd(data->udev);
#endif

#if SUPPORT_SEND_DUMMY_BULK_OUT_AFTER_RESUME
		    btmtk_usb_send_dummy_bulk_out_packet(data);
		    btmtk_usb_send_dummy_bulk_out_packet(data);
#endif
			goto error0;
		}
	}

	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (!urb) {
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(data->udev, UPLOAD_PATCH_UNIT, GFP_ATOMIC, &data_dma);

	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;

	if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
		load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name,
				   &data->udev->dev, &data->rom_patch_len);
		printk("LOAD_CODE_METHOD : BIN_FILE_METHOD\n");
	} else {
		data->rom_patch = data->rom_patch_header_image;
		printk("LOAD_CODE_METHOD : HEADER_METHOD\n");
	}

	if (!data->rom_patch) {
		if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
			printk
			    ("%s:please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)\n",
			     __func__, data->rom_patch_bin_file_name,
			     data->rom_patch_bin_file_name);
		} else {
			printk("%s:please assign a rom patch\n", __func__);
		}

		ret = -1;
		goto error2;
	}

    tmp_str = data->rom_patch;
	printk("%s : FW Version = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", __FUNCTION__,
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

	printk("%s : build Time = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", __FUNCTION__,
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

    memset(rom_patch_version, 0, sizeof(rom_patch_version));
    memcpy(rom_patch_version, tmp_str, 15);

    tmp_str = data->rom_patch + 16;
	printk("%s : platform = %c%c%c%c\n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


    tmp_str = data->rom_patch + 20;
    printk("%s : HW/SW version = %c%c%c%c \n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 24;
//	printk("%s : Patch version = %c%c%c%c \n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


	printk("\nloading rom patch...\n");

	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
	printk("%s : patch_len = %d\n", __FUNCTION__, patch_len);

	printk("%s : loading rom patch... \n\n", __FUNCTION__);
	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		sent_len =
		    (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		//printk("cur_len = %d\n", cur_len);//scofield mark
		//printk("sent_len = %d\n", sent_len);//scofield mark

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);

			//printk("sent_len + PATCH_HEADER_SIZE = %d, phase = %d\n",
			       //sent_len + PATCH_HEADER_SIZE, phase);//scofield mark

			usb_fill_bulk_urb(urb,
					  data->udev,
					  pipe,
					  buf,
					  sent_len + PATCH_HEADER_SIZE,
					  load_rom_patch_complete, &sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_ATOMIC);

			if (ret)
				goto error2;

			if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				printk("%s : upload rom_patch timeout\n", __FUNCTION__);
				goto error2;
			}

			mdelay(1);

			#if 1
			mdelay(20);
			ret = btmtk_usb_get_rom_patch_result(data);
			mdelay(20);
			#endif

			cur_len += sent_len;

		} else {
			break;
		}
	}

#if 0
	mdelay(20);
	ret = btmtk_usb_get_rom_patch_result(data);
	mdelay(20);

	/* Send Checksum request */
	total_checksum = checksume16(data->rom_patch + PATCH_INFO_SIZE, patch_len);
	btmtk_usb_chk_crc(data, patch_len);

	mdelay(20);

	if (total_checksum != btmtk_usb_get_crc(data)) {
		printk("checksum fail!, local(0x%x) <> fw(0x%x)\n", total_checksum,
		       btmtk_usb_get_crc(data));
		//ret = -1;
		//goto error2;
	}

	mdelay(20);
	/* send check rom patch result request */
	btmtk_usb_send_check_rom_patch_result_cmd(data->udev);
#endif

//btmtk_usb_io_read32_76XX(data, 0x80000600, &value);//commmon register

	mdelay(20);
	/* CHIP_RESET */
	ret = btmtk_usb_reset_wmt(data);

	mdelay(20);
	if (data->chip_id != 0x7636)
		btmtk_usb_send_power_on_cmd(data);

	mdelay(20);
	/* BT_RESET */
	btmtk_usb_send_hci_reset_cmd(data->udev);

	/* for WoBLE/WoW low power */
	btmtk_usb_send_hci_set_ce_cmd(data->udev);
 error2:
	usb_free_coherent(data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
 error1:
	usb_free_urb(urb);
 error0:
	//btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);
	printk("btmtk_usb_load_rom_patch end\n");
	return ret;
}

#endif//#if (SUPPORT_MT7637 || SUPPORT_MT6632)//btmtk_usb_send_power_on_cmd(struct btmtk_usb_data *data)

static int btmtk_usb_send_set_tx_power_cmd(struct usb_device *udev)
{
	printk("btmtk_usb_send_set_tx_power_cmd  7....\n");

	//btmtk_usb_read_dummy_event(udev);//wenhui add to read dummy hci event
    // btmtk_usb_send_set_tx_power_cmd
    {
    	int ret = 0;
		char buf[10] = {0x79, 0xFC, 0x06, 0x07, 0x80, 0x00, 0x06, 0x07, 0x07, 0x00};

    	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
    						  0x00, 0x00, buf, 0x09, 100);

    	if (ret < 0)
    	{
    		printk("%s error1(%d)\n", __FUNCTION__, ret);
    		return ret;
    	}
	}

    // Get response of HCI reset
    {
        int ret = 0;
        char buf[BT_MAX_EVENT_SIZE] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, BT_MAX_EVENT_SIZE, &actual_length, 2000);

        if (ret <0)
        {
    		printk("%s error2(%d)\n", __FUNCTION__, ret);
    		return ret;
        }
		printk("btmtk_usb_send_set_tx_power_cmd 7 get response  0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
    	            buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
    }

	return 0;
}


#if __MTK_BT_BOOT_ON_FLASH__

static int btmtk_usb_load_patch_to_ram(struct btmtk_usb_data *data)
{
	s32 sent_len;
	int ret = 0, total_checksum = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	unsigned char phase;
	void *buf;
	char *pos;
	unsigned int pipe = usb_sndbulkpipe(data->udev,
										data->bulk_tx_ep->bEndpointAddress);

	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (!urb)
	{
		ret = -ENOMEM;
		printk("btmtk_usb_load_patch_to_ram,go to error0\n");
		goto error0;
	}

	buf = usb_alloc_coherent(data->udev, UPLOAD_PATCH_UNIT, GFP_ATOMIC, &data_dma);

	if (!buf) {
		ret = -ENOMEM;
		printk("btmtk_usb_load_patch_to_ram,go to error1\n");
		goto error1;
	}

	pos = buf;
	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
	/* loading rom patch */
	while (1)
	{
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		printk("patch_len = %d\n", patch_len);
		printk("cur_len = %d\n", cur_len);
		printk("sent_len = %d\n", sent_len);

		if (sent_len > 0)
		{
			if (first_block == 1)
			{
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			}
			else if (sent_len == sent_len_max)
			{
				phase = PATCH_PHASE2;
			}
			else
			{
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;
			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);

			printk("sent_len + PATCH_HEADER_SIZE = %d, phase = %d\n", sent_len + PATCH_HEADER_SIZE, phase);

			usb_fill_bulk_urb(urb,
							  data->udev,
							  pipe,
							  buf,
							  sent_len + PATCH_HEADER_SIZE,
							  load_rom_patch_complete,
							  &sent_to_mcu_done);
			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_ATOMIC);

			if (ret){
				printk("btmtk_usb_load_patch_to_ram,submit urb fail,go to error2\n");
				goto error2;
			}
			if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				printk("upload rom_patch timeout\n");
				goto error2;
			}

			mdelay(1);

			cur_len += sent_len;

		}
		else
		{
			break;
		}
	}

	mdelay(20);
	ret = btmtk_usb_get_rom_patch_result(data);

	mdelay(20);
	// Send Checksum request
	total_checksum = checksume16(data->rom_patch + PATCH_INFO_SIZE, patch_len);
	btmtk_usb_chk_crc(data, patch_len);

	mdelay(20);

	if (total_checksum != btmtk_usb_get_crc(data))
	{
		printk("checksum fail!, local(0x%x) <> fw(0x%x)\n", total_checksum, btmtk_usb_get_crc(data));
		ret = -1;
		printk("btmtk_usb_load_patch_to_ram,go to error2\n");
		goto error2;

	}

	mdelay(20);
	// send check rom patch result request
	btmtk_usb_send_check_rom_patch_result_cmd(data->udev);
	mdelay(20);
	// CHIP_RESET
	ret = btmtk_usb_reset_wmt(data);
	mdelay(20);
	// BT_RESET
	btmtk_usb_send_hci_reset_cmd(data->udev);

error2:
	usb_free_coherent(data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
	usb_free_urb(urb);
error0:
	btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);
	printk("btmtk_usb_load_patch_to_ram end\n");
	return ret;
}


static int btmtk_usb_load_rom_patch(struct btmtk_usb_data *data)
{
	u32 loop = 0;
	u32 value;
	int ret = 0,i =0 ;
	u8 boot_on_flag = 0,retry_count = 2,driver_version[8] = {0};
	char *tmp_str;

    printk("btmtk_usb_load_rom_patch begin+++\n");

load_patch_protect:
	btmtk_usb_switch_iobase(data, WLAN);
	btmtk_usb_io_read32(data, SEMAPHORE_03, &value);
	loop++;

	if (((value & 0x01) == 0x00) && (loop < 600)) {
		mdelay(1);
		goto load_patch_protect;
	}

	btmtk_usb_switch_iobase(data, SYSCTL);

	btmtk_usb_io_write32(data, 0x1c, 0x30);

	btmtk_usb_switch_iobase(data, WLAN);
	/* check ROM patch if upgrade */
	if ( (MT_REV_GTE(data, mt7662, REV_MT76x2E3)) ||
	     (MT_REV_GTE(data, mt7632, REV_MT76x2E3)))
	{
		btmtk_usb_io_read32(data, CLOCK_CTL, &value);
		if ((value & 0x01) != 0x01)
		{
			printk("btmtk_usb_load_rom_patch : no patch on flash\n");
		   // btmtk_usb_send_hci_reset_cmd(data->udev);
			boot_on_flag = 1;//flash broken or boot on rom
		}
	}
	else
	{
		btmtk_usb_io_read32(data, COM_REG0, &value);
		if ((value & 0x02) != 0x02)
		{
			printk("btmtk_usb_load_rom_patch : no patch on flash \n");
		   // btmtk_usb_send_hci_reset_cmd(data->udev);
			boot_on_flag = 1;//flash broken or boot on rom
		}
	}


	if (LOAD_CODE_METHOD == BIN_FILE_METHOD)
	{
		load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name, &data->udev->dev, &data->rom_patch_len);
        printk("LOAD_CODE_METHOD : BIN_FILE_METHOD\n");
	}
	else
	{
		data->rom_patch = data->rom_patch_header_image;
        printk("LOAD_CODE_METHOD : HEADER_METHOD\n");
	}

	if (!data->rom_patch)
	{
		if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
			printk("%s:please assign a rom patch(/etc/firmware/%s)\n",
				__FUNCTION__, data->rom_patch_bin_file_name);
		} else {
			printk("%s:please assign a rom patch\n", __FUNCTION__);
		}
		printk("goto load patch end\n");
		goto load_patch_end;
	}

	msleep(200);

    tmp_str = data->rom_patch;
	printk("FW Version = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

	printk("build Time = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

    memset(rom_patch_version, 0, sizeof(rom_patch_version));
    memcpy(rom_patch_version, tmp_str, 15);

    tmp_str = data->rom_patch + 16;
	printk("platform = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 20;
    	printk("HW/SW version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 24;
	printk("Patch version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


	memset(rom_patch_crc, 0, sizeof(rom_patch_crc));

	tmp_str = data->rom_patch + 0x1c;
	rom_patch_crc[0] = tmp_str[0];
	rom_patch_crc[1] = tmp_str[1];

	printk("\n driver patch crc: %02x, %02x", rom_patch_crc[0], rom_patch_crc[1]);

	//get patch info
	data->driver_patch_version = 0;
	for(i=0; i<8; i++){
		driver_version[i] = rom_patch_version[i]-0x30;
	}

	printk("\n");
	for(i=0; i<4; i++){
		data->driver_version[i] = (rom_patch_version[2*i]-0x30)*16 + (rom_patch_version[2*i+1]-0x30);
		printk("data->driver_version[%d]: %02X\n", i, data->driver_version[i] );
	}
	data->driver_patch_version =  driver_version[0]*10000000 + driver_version[1]*1000000 + driver_version[2]*100000 + driver_version[3]*10000 + \
		 driver_version[4]*1000 + driver_version[5]*100 + driver_version[6]*10 + driver_version[7];


	printk("probe, driver_patch_version: %d\n", data->driver_patch_version);

    //step1: send hci_reset first
	btmtk_usb_send_hci_reset_cmd(data->udev);

	//step2: prepare patch info & flash patch info & set to btmtk_usb_data struct
	btmtk_usb_get_flash_patch_version(data);

	if(boot_on_flag == 1 || data->patch_version_all_ff == 1)
	{
		ret=btmtk_usb_load_patch_to_ram(data);
		if(ret<0){
			printk("err:btmtk_usb_load_patch_to_ram return %d\n",ret);
		}
		printk("+++begin receive dummy event+++\n");
		btmtk_usb_read_dummy_event(data->udev);
		printk("+++end receive dummy event----\n");
	}

	if(data->patch_version_all_ff != 0){

			retry_count = 2;
			ret = btmtk_usb_load_patch_to_flash(data);

			//need to always load patch or not?
			while((ret != 0)&&(retry_count > 0)){
				ret = btmtk_usb_load_patch_to_flash(data);
				retry_count = retry_count - 1;

			}
			if(ret!=0)
				printk("btmtk_usb_load_patch_to_flash fail count:%d  ,ret:%d \n",3-retry_count,ret);
			else
				printk("btmtk_usb_load_patch_to_flash success count:%d  ,ret:%d \n",3-retry_count,ret);
	}
load_patch_end:

	mdelay(20);
    // hci_reset
	btmtk_usb_send_hci_reset_cmd(data->udev);

	// for WoBLE/WoW low power
	btmtk_usb_send_hci_set_ce_cmd(data->udev);

	//btmtk_usb_send_enable_low_power_cmd(data->udev);  //send FC41

	btmtk_usb_send_disable_low_power_cmd(data->udev);//send FC40
#ifdef SET_TX_POWER_AFTER_ROM_PATCH
	mdelay(20);
	btmtk_usb_send_set_tx_power_cmd(data->udev);//default set to 7
#endif
	btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);
    printk("btmtk_usb_load_rom_patch 2@end---\n");
	return ret;
}

#else	// __MTK_BT_BOOT_ON_FLASH__
static int btmtk_usb_load_rom_patch(struct btmtk_usb_data *data)
{
	u32 loop = 0;
	u32 value;
	s32 sent_len;
	int ret = 0, total_checksum = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	unsigned char phase;
	void *buf;
	char *pos;
	char *tmp_str;
	unsigned int pipe = usb_sndbulkpipe(data->udev,
					    data->bulk_tx_ep->bEndpointAddress);

	printk("btmtk_usb_load_rom_patch begin\n");
 load_patch_protect:
	btmtk_usb_switch_iobase(data, WLAN);
	btmtk_usb_io_read32(data, SEMAPHORE_03, &value);
	loop++;

	if ((value & 0x01) == 0x00) {
		if (loop < 1000) {
			mdelay(1);
			goto load_patch_protect;
		} else {
			printk("btmtk_usb_load_rom_patch ERR! Can't get semaphore! Continue\n");
		}
	}

	btmtk_usb_switch_iobase(data, SYSCTL);

	btmtk_usb_io_write32(data, 0x1c, 0x30);

	btmtk_usb_switch_iobase(data, WLAN);

	/* check ROM patch if upgrade */
	if ((MT_REV_GTE(data, mt7662, REV_MT76x2E3)) || (MT_REV_GTE(data, mt7632, REV_MT76x2E3))) {
		btmtk_usb_io_read32(data, CLOCK_CTL, &value);
		if ((value & 0x01) == 0x01) {
			printk("btmtk_usb_load_rom_patch : no need to load rom patch\n");
//			btmtk_usb_send_hci_reset_cmd(data->udev);

            if ( data->chip_id == 0x76620044 ) {
                // Send dummy bulk out packet for MT7662U
                btmtk_usb_send_dummy_bulk_out_packet(data);
            }

			goto error0;
		}
	} else {
		btmtk_usb_io_read32(data, COM_REG0, &value);
		if ((value & 0x02) == 0x02) {
			printk("btmtk_usb_load_rom_patch : no need to load rom patch\n");
//			btmtk_usb_send_hci_reset_cmd(data->udev);
			goto error0;
		}
	}

	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (!urb) {
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(data->udev, UPLOAD_PATCH_UNIT, GFP_ATOMIC, &data_dma);

	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;

	if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
		load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name,
				   &data->udev->dev, &data->rom_patch_len);
		printk("LOAD_CODE_METHOD : BIN_FILE_METHOD\n");
	} else {
		data->rom_patch = data->rom_patch_header_image;
		printk("LOAD_CODE_METHOD : HEADER_METHOD\n");
	}

	if (!data->rom_patch) {
		if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
			printk
			    ("%s:please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)\n",
			     __func__, data->rom_patch_bin_file_name,
			     data->rom_patch_bin_file_name);
		} else {
			printk("%s:please assign a rom patch\n", __func__);
		}

		ret = -1;
		goto error2;
	}

	tmp_str = data->rom_patch;
	printk("FW Version = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
	       tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
	       tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
	       tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
	       tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

	printk("build Time = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
	       tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
	       tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
	       tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
	       tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

	memset(rom_patch_version, 0, sizeof(rom_patch_version));
	memcpy(rom_patch_version, tmp_str, 15);

	tmp_str = data->rom_patch + 16;
	printk("platform = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


	tmp_str = data->rom_patch + 20;
	printk("HW/SW version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = data->rom_patch + 24;
	printk("Patch version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


#if SUPPORT_STABLE_WOBLE_WHEN_HW_RESET
    {
        typedef void (*pdwnc_func)(unsigned char fgReset);
        char *pdwnc_func_name = "PDWNC_SetBTInResetState";
        pdwnc_func pdwndFunc = NULL;
        pdwndFunc = (pdwnc_func)kallsyms_lookup_name(pdwnc_func_name);

        if (pdwndFunc)
        {
            printk("%s - Invoke %s(%d)\n", __FUNCTION__, pdwnc_func_name, 1);
            pdwndFunc(1);
        }
        else
            printk("%s - No Exported Func Found [%s] \n", __FUNCTION__, pdwnc_func_name);
    }
#endif

	printk("\nloading rom patch...\n");

	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = data->rom_patch_len - PATCH_INFO_SIZE;

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		sent_len =
		    (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		printk("patch_len = %d\n", patch_len);
		printk("cur_len = %d\n", cur_len);
		printk("sent_len = %d\n", sent_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);

			printk("sent_len + PATCH_HEADER_SIZE = %d, phase = %d\n",
			       sent_len + PATCH_HEADER_SIZE, phase);

			usb_fill_bulk_urb(urb,
					  data->udev,
					  pipe,
					  buf,
					  sent_len + PATCH_HEADER_SIZE,
					  load_rom_patch_complete, &sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_ATOMIC);

			if (ret)
				goto error2;

			if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				printk("upload rom_patch timeout\n");
				goto error2;
			}

			mdelay(1);

			cur_len += sent_len;

		} else {
			break;
		}
	}

	mdelay(1);
	ret = btmtk_usb_get_rom_patch_result(data);
	mdelay(1);

	/* Send Checksum request */
	total_checksum = checksume16(data->rom_patch + PATCH_INFO_SIZE, patch_len);
	btmtk_usb_chk_crc(data, patch_len);

	mdelay(1);

	if (total_checksum != btmtk_usb_get_crc(data)) {
		printk("checksum fail!, local(0x%x) <> fw(0x%x)\n", total_checksum,
		       btmtk_usb_get_crc(data));
		ret = -1;
		goto error2;
	}

	mdelay(1);
	/* send check rom patch result request */
	btmtk_usb_send_check_rom_patch_result_cmd(data->udev);
	mdelay(1);
	/* CHIP_RESET */
	ret = btmtk_usb_reset_wmt(data);
	mdelay(1);
	/* BT_RESET */
	btmtk_usb_send_hci_reset_cmd(data->udev);

	/* for WoBLE/WoW low power */
	btmtk_usb_send_hci_set_ce_cmd(data->udev);

#ifdef SET_TX_POWER_AFTER_ROM_PATCH
	mdelay(20);
	btmtk_usb_send_set_tx_power_cmd(data->udev);//default set to 7
#endif

 error2:
	usb_free_coherent(data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
 error1:
	usb_free_urb(urb);
 error0:
	btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);

#if SUPPORT_STABLE_WOBLE_WHEN_HW_RESET
    {
        typedef void (*pdwnc_func)(unsigned char fgReset);
        char *pdwnc_func_name = "PDWNC_SetBTInResetState";
        pdwnc_func pdwndFunc = NULL;
        pdwndFunc = (pdwnc_func)kallsyms_lookup_name(pdwnc_func_name);

        if (pdwndFunc)
        {
            printk("%s - Invoke %s(%d)\n", __FUNCTION__, pdwnc_func_name, 0);
            pdwndFunc(0);
        }
        else
            printk("%s - No Exported Func Found [%s] \n", __FUNCTION__, pdwnc_func_name);
    }
#endif

	printk("btmtk_usb_load_rom_patch end\n");
	return ret;
}
#endif

static int load_fw_iv(struct btmtk_usb_data *data)
{
	int ret;
	struct usb_device *udev = data->udev;
	char *buf = kmalloc(64, GFP_ATOMIC);

	memmove(buf, data->fw_image + 32, 64);

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x01,
			      DEVICE_VENDOR_REQUEST_OUT, 0x12, 0x0, buf, 64,
			      CONTROL_TIMEOUT_JIFFIES);

	if (ret < 0) {
		printk("%s error(%d) step4\n", __func__, ret);
		kfree(buf);
		return ret;
	}

	if (ret > 0)
		ret = 0;

	kfree(buf);

	return ret;
}

static void load_fw_complete(struct urb *urb)
{

	struct completion *sent_to_mcu_done = (struct completion *)urb->context;

	complete(sent_to_mcu_done);
}

static int btmtk_usb_load_fw(struct btmtk_usb_data *data)
{
	struct usb_device *udev = data->udev;
	struct urb *urb;
	void *buf;
	u32 cur_len = 0;
	u32 packet_header = 0;
	u32 value;
	u32 ilm_len = 0, dlm_len = 0;
	u16 fw_ver, build_ver;
	u32 loop = 0;
	dma_addr_t data_dma;
	int ret = 0, sent_len;
	struct completion sent_to_mcu_done;
	unsigned int pipe = usb_sndbulkpipe(data->udev,
					    data->bulk_tx_ep->bEndpointAddress);

	printk("bulk_tx_ep = %x\n", data->bulk_tx_ep->bEndpointAddress);

 loadfw_protect:
	btmtk_usb_switch_iobase(data, WLAN);
	btmtk_usb_io_read32(data, SEMAPHORE_00, &value);
	loop++;

	if (((value & 0x1) == 0) && (loop < 10000))
		goto loadfw_protect;

	/* check MCU if ready */
	btmtk_usb_io_read32(data, COM_REG0, &value);

	if ((value & 0x01) == 0x01)
		goto error0;

	/* Enable MPDMA TX and EP2 load FW mode */
	btmtk_usb_io_write32(data, 0x238, 0x1c000000);

	btmtk_usb_reset(udev);
	mdelay(100);

	if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
		load_code_from_bin(&data->fw_image, data->fw_bin_file_name, &data->udev->dev,
				   &data->fw_len);
		printk("LOAD_CODE_METHOD : BIN_FILE_METHOD\n");
	} else {
		data->fw_image = data->fw_header_image;
		printk("LOAD_CODE_METHOD : HEADER_METHOD\n");
	}

	if (!data->fw_image) {
		if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
			printk("%s:please assign a fw(/etc/firmware/%s)or(/lib/firmware/%s)\n",
			       __func__, data->fw_bin_file_name, data->fw_bin_file_name);
		} else {
			printk("%s:please assign a fw\n", __func__);
		}

		ret = -1;
		goto error0;
	}

	ilm_len = (*(data->fw_image + 3) << 24) | (*(data->fw_image + 2) << 16) |
	    (*(data->fw_image + 1) << 8) | (*data->fw_image);

	dlm_len = (*(data->fw_image + 7) << 24) | (*(data->fw_image + 6) << 16) |
	    (*(data->fw_image + 5) << 8) | (*(data->fw_image + 4));

	fw_ver = (*(data->fw_image + 11) << 8) | (*(data->fw_image + 10));

	build_ver = (*(data->fw_image + 9) << 8) | (*(data->fw_image + 8));

	printk("fw version:%d.%d.%02d ", (fw_ver & 0xf000) >> 8,
	       (fw_ver & 0x0f00) >> 8, (fw_ver & 0x00ff));

	printk("build:%x\n", build_ver);

	printk("build Time =");

	for (loop = 0; loop < 16; loop++)
		printk("%c", *(data->fw_image + 16 + loop));

	printk("\n");

	printk("ILM length = %d(bytes)\n", ilm_len);
	printk("DLM length = %d(bytes)\n", dlm_len);

	btmtk_usb_switch_iobase(data, SYSCTL);

	/* U2M_PDMA rx_ring_base_ptr */
	btmtk_usb_io_write32(data, 0x790, 0x400230);

	/* U2M_PDMA rx_ring_max_cnt */
	btmtk_usb_io_write32(data, 0x794, 0x1);

	/* U2M_PDMA cpu_idx */
	btmtk_usb_io_write32(data, 0x798, 0x1);

	/* U2M_PDMA enable */
	btmtk_usb_io_write32(data, 0x704, 0x44);

	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (!urb) {
		ret = -ENOMEM;
		goto error1;
	}

	buf = usb_alloc_coherent(udev, 14592, GFP_ATOMIC, &data_dma);

	if (!buf) {
		ret = -ENOMEM;
		goto error2;
	}

	printk("loading fw");

	init_completion(&sent_to_mcu_done);

	btmtk_usb_switch_iobase(data, SYSCTL);

	cur_len = 0x40;

	/* Loading ILM */
	while (1) {
		sent_len = (ilm_len - cur_len) >= 14336 ? 14336 : (ilm_len - cur_len);

		if (sent_len > 0) {
			packet_header &= ~(0xffffffff);
			packet_header |= (sent_len << 16);
			packet_header = cpu_to_le32(packet_header);

			memmove(buf, &packet_header, 4);
			memmove(buf + 4, data->fw_image + 32 + cur_len, sent_len);

			/* U2M_PDMA descriptor */
			btmtk_usb_io_write32(data, 0x230, cur_len);

			while ((sent_len % 4) != 0) {
				sent_len++;
			}

			/* U2M_PDMA length */
			btmtk_usb_io_write32(data, 0x234, sent_len << 16);

			usb_fill_bulk_urb(urb,
					  udev,
					  pipe,
					  buf, sent_len + 4, load_fw_complete, &sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_ATOMIC);

			if (ret)
				goto error3;

			if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				printk("upload ilm fw timeout\n");
				goto error3;
			}

			BT_DBG(".");

			mdelay(200);

			cur_len += sent_len;
		} else {
			break;
		}
	}

	init_completion(&sent_to_mcu_done);
	cur_len = 0x00;

	/* Loading DLM */
	while (1) {
		sent_len = (dlm_len - cur_len) >= 14336 ? 14336 : (dlm_len - cur_len);

		if (sent_len > 0) {
			packet_header &= ~(0xffffffff);
			packet_header |= (sent_len << 16);
			packet_header = cpu_to_le32(packet_header);

			memmove(buf, &packet_header, 4);
			memmove(buf + 4, data->fw_image + 32 + ilm_len + cur_len, sent_len);

			/* U2M_PDMA descriptor */
			btmtk_usb_io_write32(data, 0x230, 0x80000 + cur_len);

			while ((sent_len % 4) != 0) {
				BT_DBG("sent_len is not divided by 4\n");
				sent_len++;
			}

			/* U2M_PDMA length */
			btmtk_usb_io_write32(data, 0x234, sent_len << 16);

			usb_fill_bulk_urb(urb,
					  udev,
					  pipe,
					  buf, sent_len + 4, load_fw_complete, &sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_ATOMIC);

			if (ret)
				goto error3;

			if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				printk("upload dlm fw timeout\n");
				goto error3;
			}

			BT_DBG(".");

			mdelay(500);

			cur_len += sent_len;

		} else {
			break;
		}
	}

	/* upload 64bytes interrupt vector */
	ret = load_fw_iv(data);
	mdelay(100);

	btmtk_usb_switch_iobase(data, WLAN);

	/* check MCU if ready */
	loop = 0;

	do {
		btmtk_usb_io_read32(data, COM_REG0, &value);

		if (value == 0x01)
			break;

		mdelay(10);
		loop++;
	} while (loop <= 100);

	if (loop > 1000) {
		printk("wait for 100 times\n");
		ret = -ENODEV;
	}

 error3:
	usb_free_coherent(udev, 14592, buf, data_dma);
 error2:
	usb_free_urb(urb);
 error1:
	/* Disbale load fw mode */
	btmtk_usb_io_read32(data, 0x238, &value);
	value = value & ~(0x10000000);
	btmtk_usb_io_write32(data, 0x238, value);
 error0:
	btmtk_usb_io_write32(data, SEMAPHORE_00, 0x1);
	return ret;
}

static int inc_tx(struct btmtk_usb_data *data)
{
	unsigned long flags;
	int rv;
	printk("btmtk:inc_tx\n");

	spin_lock_irqsave(&data->txlock, flags);
	rv = test_bit(BTUSB_SUSPENDING, &data->flags);
	if (!rv) {
		data->tx_in_flight++;
		printk("btmtk:tx_in_flight++\n");
	}

	spin_unlock_irqrestore(&data->txlock, flags);

	return rv;
}

static void btmtk_usb_intr_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	unsigned char *event_buf;
	UINT32 roomLeft, last_len, length;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	int err;
#if SUPPORT_BT_ATE
	int is_ate_event = 0;
#endif

	if (urb->status != 0)
		printk("%s: %s urb %p urb->status %d count %d\n", __func__, hdev->name,
	       urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags) && (metaMode == 0)) {
		printk("stop interrupt urb\n");
		goto intr_resub;
	}

	if (urb->status == 0 && metaMode == 0) {
		hdev->stat.byte_rx += urb->actual_length;

#if SUPPORT_BT_ATE
		/* check ATE cmd event */
		{
			unsigned char *event_buf = urb->transfer_buffer;
			u32 event_buf_len = urb->actual_length;
			u8 matched = 0;
			int i;
			u32 Count_Tx_ACL = 0;
			u32 Count_ACK = 0;
			u8 PHY_RATE = 0;
			if (event_buf) {
				if (event_buf[3] == 0x4D && event_buf[4] == 0xFC)
					matched = 1;

				if (event_buf[3] == 0x3F && event_buf[4] == 0x0C)
					matched = 2;

				if (matched == 1) {
					is_ate_event = 1;
					printk("Got ATE event result:(%d)\n    ", event_buf_len);
					for (i = 0; i < event_buf_len; i++) {
						printk("%02X ", event_buf[i]);
					}
					printk("\n");

					Count_Tx_ACL =
					    event_buf[6] | ((event_buf[7] << 8) & 0xff00) |
					    ((event_buf[8] << 16) & 0xff0000) |
					    ((event_buf[9] << 24) & 0xff000000);
					Count_ACK =
					    event_buf[10] | ((event_buf[11] << 8) & 0xff00) |
					    ((event_buf[12] << 16) & 0xff0000) |
					    ((event_buf[13] << 24) & 0xff000000);
					PHY_RATE = event_buf[14];

					printk("Count_Tx_ACL = 0x%08X\n", Count_Tx_ACL);
					printk("Count_ACK = 0x%08X\n", Count_ACK);
					if (PHY_RATE == 0)
						printk("PHY_RATE = 1M_DM\n");
					else if (PHY_RATE == 1)
						printk("PHY_RATE = 1M_DH\n");
					else if (PHY_RATE == 2)
						printk("PHY_RATE = 2M\n");
					else if (PHY_RATE == 3)
						printk("PHY_RATE = 3M\n");
				} else if (matched == 2) {
					printk
					    ("Got ATE AFH Host Channel Classification Command Result:(%d)\n    ",
					     event_buf_len);
					for (i = 0; i < event_buf_len; i++) {
						printk("%02X ", event_buf[i]);
					}
					printk("\n");

					if (event_buf[5] == 0)
						printk("Result: Success\n");
					else
						printk("Result: Failed(0x%x)\n", event_buf[5]);

				}
			}
		}

		if (is_ate_event == 0)
#endif				/* SUPPORT_BT_ATE */
			if (hci_recv_fragment(hdev, HCI_EVENT_PKT,
					      urb->transfer_buffer, urb->actual_length) < 0) {
				printk("%s corrupted event packet", hdev->name);
				hdev->stat.err_rx++;
			}
	} else if (urb->status == 0 && metaMode == 1 && urb->actual_length != 0) {
		event_buf = urb->transfer_buffer;

#if SUPPORT_HCI_DUMP
        btmtk_usb_save_hci_event(urb->actual_length, urb->transfer_buffer);
#endif

		/* In this condition, need to add header*/
		if (leftHciEventSize == 0)
			length = urb->actual_length + 1;
		else
			length = urb->actual_length;

		lock_unsleepable_lock(&(metabuffer->spin_lock));
		/* clayton: roomleft means the usable space */

		if (metabuffer->read_p <= metabuffer->write_p) {
			roomLeft = META_BUFFER_SIZE - metabuffer->write_p + metabuffer->read_p - 1;
		} else {
			roomLeft = metabuffer->read_p - metabuffer->write_p - 1;
		}

		/* clayton: no enough space to store the received data */
		if (roomLeft < length) {
			printk("%s : Queue is full !!\n", __func__);
		}

		if (length + metabuffer->write_p < META_BUFFER_SIZE) {
			/* only one interrupt event, not be splited */
			if (leftHciEventSize == 0) {
				/* copy event header: 0x04 */
				metabuffer->buffer[metabuffer->write_p] = 0x04;
				metabuffer->write_p += 1;
			}
			/* copy event data */
			osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf,
				    urb->actual_length);
			metabuffer->write_p += urb->actual_length;
		} else {
			pr_debug("btmtk:back to meta buffer head\n");
			last_len = META_BUFFER_SIZE - metabuffer->write_p;

			/* only one interrupt event, not be splited */
			if (leftHciEventSize == 0) {
				if (last_len != 0) {
					/* copy evnet header: 0x04 */
					metabuffer->buffer[metabuffer->write_p] = 0x04;
					metabuffer->write_p += 1;
					last_len--;
					/* copy event data */
					osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf, last_len);
					osal_memcpy(metabuffer->buffer, event_buf + last_len, urb->actual_length - last_len);
					metabuffer->write_p = urb->actual_length - last_len;
				} else {
					metabuffer->buffer[0] = 0x04;
					metabuffer->write_p = 1;
					/* copy event data */
					osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf, urb->actual_length);
					metabuffer->write_p += urb->actual_length;
				}
			} else {	// leftHciEventSize!=0
				/* copy event data */
				osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf, last_len);
					osal_memcpy(metabuffer->buffer, event_buf + last_len, urb->actual_length - last_len);
				metabuffer->write_p = urb->actual_length - last_len;
			}
		}

		unlock_unsleepable_lock(&(metabuffer->spin_lock));

		/* means there is some data in next event */
		if (leftHciEventSize == 0 && event_buf[1] >= (HCI_MAX_EVENT_SIZE - 1)) {
			leftHciEventSize = event_buf[1] + 2 - HCI_MAX_EVENT_SIZE;	/* the left data size in next interrupt event */
			//pr_debug("there is left event, size:%d\n", leftHciEventSize);
		} else if (leftHciEventSize != 0) {
			leftHciEventSize -= urb->actual_length;

			/* error handling. Length wrong, drop some bytes to recovery counter!! */
			if (leftHciEventSize < 0) {
				printk("%s: *size wrong(%d), this event may be wrong!!\n", __FUNCTION__, leftHciEventSize);
				leftHciEventSize = 0; /* reset count */
			}

			if (leftHciEventSize == 0) {
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
                if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING)
                {
    				wake_up(&BT_wq);
    				wake_up_interruptible(&inq);
                }
                else
            		printk("%s: current is in suspend/resume (%d), Don't wake-up wait queue\n", __func__, btmtk_usb_get_state());
#else
				wake_up(&BT_wq);
				wake_up_interruptible(&inq);
#endif
			}
		} else if (leftHciEventSize == 0 && event_buf[1] < (HCI_MAX_EVENT_SIZE - 1)) {
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
            if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING)
            {
				wake_up(&BT_wq);
				wake_up_interruptible(&inq);
            }
            else
        		printk("%s: current is in suspend/resume (%d), Don't wake-up wait queue\n", __func__, btmtk_usb_get_state());
#else
			wake_up(&BT_wq);
			wake_up_interruptible(&inq);
#endif
		}
	} else {
		printk("meta mode:%d, urb->status:%d, urb->length:%d", metaMode, urb->status, urb->actual_length);
	}

	if (!test_bit(BTUSB_INTR_RUNNING, &data->flags) && metaMode == 0) {
		printk("btmtk:Not in meta mode\n");
		return;
	}
intr_resub:
	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);

	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p failed to resubmit intr_in_urb(%d)",
			       hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btmtk_usb_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
	struct btmtk_usb_data *data;

	if (hdev == NULL)
		return -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	printk("%s:begin\n", __func__);

	if (!data->intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

//	size = le16_to_cpu(data->intr_ep->wMaxPacketSize);
	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}
	/* clayton: generate a interrupt pipe, direction: usb device to cpu */
	pipe = usb_rcvintpipe(data->udev, data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, data->udev, pipe, buf, size,
			 btmtk_usb_intr_complete, hdev, data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);
	printk("%s:end\n", __func__);

	return err;


}

static void btmtk_usb_bulk_in_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btmtk_usb_data *data;
	/* actual_length: the ACL data size (doesn't contain header) */
	UINT32 roomLeft, last_len, length, index, actual_length;
	unsigned char *event_buf;
	int err;
	u8 *buf;
	u16 len;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif
	roomLeft = 0;
	last_len = 0;
	length = 0;
	index = 0;
	actual_length = 0;

	if (urb->status != 0)
		printk("%s:%s urb %p urb->status %d count %d\n", __func__, hdev->name,
	       urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags) && (metaMode == 0)) {
		printk("stop bulk urb\n");
		goto bulk_resub;
	}

#if SUPPORT_FW_DUMP
	{
		buf = urb->transfer_buffer;
		len = 0;
		if (urb->actual_length > 4) {
			len = buf[2] + ((buf[3]<<8)&0xff00);
			if (buf[0] == 0x6f && buf[1] == 0xfc && len+4 == urb->actual_length) {
				is_assert = 1;
				if (fw_dump_buffer_full)
					printk("btmtk FW DUMP : data comes when buffer full!! (Should Never Hzappen!!)\n");

				fw_dump_total_read_size += len;
				if (fw_dump_write_ptr + len < fw_dump_ptr+FW_DUMP_BUF_SIZE) {
					fw_dump_buffer_used_size += len;
					if (fw_dump_buffer_used_size + 512 > FW_DUMP_BUF_SIZE)
						fw_dump_buffer_full = 1;

					memcpy(fw_dump_write_ptr, &buf[4], len);
					fw_dump_write_ptr += len;
					up(&data->fw_dump_semaphore);
				} else
					printk("btmtk FW DUMP:buffer size too small!(%d:%d)\n",
						FW_DUMP_BUF_SIZE, fw_dump_total_read_size);
                // set to 0xff to avoide BlueDroid dropping this ACL packet.
                buf[0] = 0xff;
                buf[1] = 0xff;
			}
		}
	}
#endif

	if (urb->status == 0 && metaMode == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		if (hci_recv_fragment(hdev, HCI_ACLDATA_PKT,
				      urb->transfer_buffer, urb->actual_length) < 0) {
			printk("%s corrupted ACL packet", hdev->name);
			hdev->stat.err_rx++;
		}
	} else if (urb->status == 0 && metaMode == 1) {
		event_buf = urb->transfer_buffer;
		len = buf[2] + ((buf[3] << 8) & 0xff00);

#if SUPPORT_HCI_DUMP
        btmtk_usb_save_hci_acl(urb->actual_length, urb->transfer_buffer);
#endif

		if (urb->actual_length > 4 && event_buf[0] == 0x6f && event_buf[1] == 0xfc
			&& len + 4 == urb->actual_length)
		{
			//printk("Coredump message\n");
		}
		else {
			length = urb->actual_length + 1;

			actual_length = 1*(event_buf[2]&0x0f) + 16*((event_buf[2]&0xf0)>>4)
				+ 256*((event_buf[3]&0x0f)) + 4096*((event_buf[3]&0xf0)>>4);

			lock_unsleepable_lock(&(metabuffer->spin_lock));

			/* clayton: roomleft means the usable space */
			if (metabuffer->read_p <= metabuffer->write_p)
				roomLeft = META_BUFFER_SIZE - metabuffer->write_p + metabuffer->read_p - 1;
			else
				roomLeft = metabuffer->read_p - metabuffer->write_p - 1;

			/* clayton: no enough space to store the received data */
			if (roomLeft < length)
    			printk("%s : Queue is full !!\n", __func__);

			if (length + metabuffer->write_p < META_BUFFER_SIZE) {

				if (leftACLSize == 0) {
					/* copy ACL data header: 0x02 */
					metabuffer->buffer[metabuffer->write_p] = 0x02;
					metabuffer->write_p += 1;
				}

				/* copy event data */
				osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf,
						urb->actual_length);
				metabuffer->write_p += urb->actual_length;
			} else {
				last_len = META_BUFFER_SIZE - metabuffer->write_p;
				if (leftACLSize == 0) {
					if (last_len != 0) {
						/* copy ACL data header: 0x02 */
						metabuffer->buffer[metabuffer->write_p] = 0x02;
						metabuffer->write_p += 1;
						last_len--;
						/* copy event data */
						osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf, last_len);
						osal_memcpy(metabuffer->buffer, event_buf + last_len,urb->actual_length - last_len);
						metabuffer->write_p = urb->actual_length - last_len;
					} else {
						metabuffer->buffer[0] = 0x02;
						metabuffer->write_p = 1;
						/* copy event data */
						osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf,urb->actual_length);
						metabuffer->write_p += urb->actual_length;
					}
				}else { // leftACLSize !=0

					/* copy event data */
					osal_memcpy(metabuffer->buffer + metabuffer->write_p, event_buf, last_len);
					osal_memcpy(metabuffer->buffer, event_buf + last_len,
							urb->actual_length - last_len);
					metabuffer->write_p = urb->actual_length - last_len;
				}
			}

			unlock_unsleepable_lock(&(metabuffer->spin_lock));

			/* the maximize bulk in ACL data packet size is 512 (4byte header + 508 byte data)*/
			/* maximum recieved data size of one packet is 1025 (4byte header + 1021 byte data) */
			if (leftACLSize == 0 && actual_length > 1017) {
				/* the data in next interrupt event */
				leftACLSize = actual_length + 4 - urb->actual_length;
				//pr_debug("there is left ACL event, size:%d\n", leftACLSize);
			} else if (leftACLSize > 0) {
				leftACLSize -= urb->actual_length;

				/* error handling. Length wrong, drop some bytes to recovery counter!! */
				if (leftACLSize < 0) {
					printk("*size wrong(%d), this acl data may be wrong!!\n", leftACLSize);
					leftACLSize = 0; /* reset count */
				}

				if (leftACLSize == 0) {
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
                    if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING ||
                        btmtk_usb_get_state() == BTMTK_USB_STATE_FW_DUMP ||
                        btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
                        btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP)
                    {
        				wake_up(&BT_wq);
        				wake_up_interruptible(&inq);
                    }
                    else
                    {
                		//printk("%s: current is in suspend/resume (%d), Don't wake-up wait queue\n", __func__, btmtk_usb_get_state());
                    }
#else
					wake_up(&BT_wq);
					wake_up_interruptible(&inq);
#endif
				}
			} else if (leftACLSize == 0 && actual_length <= 1017) {
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
                if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING ||
                    btmtk_usb_get_state() == BTMTK_USB_STATE_FW_DUMP ||
                    btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
                    btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP)
                {
    				wake_up(&BT_wq);
    				wake_up_interruptible(&inq);
                }
                else
                {
            		//printk("%s: current is in suspend/resume (%d), Don't wake-up wait queue\n", __func__, btmtk_usb_get_state());
            	}
#else
				wake_up(&BT_wq);
				wake_up_interruptible(&inq);
#endif
			} else {
				printk("ACL data count fail, leftACLSize:%d", leftACLSize);
			}
		}

	} else {
		printk("meta mode:%d, urb->status:%d\n", metaMode, urb->status);
	}

	if (!test_bit(BTUSB_BULK_RUNNING, &data->flags) && metaMode == 0)
		return;

bulk_resub:
	usb_anchor_urb(urb, &data->bulk_anchor);
	usb_mark_last_busy(data->udev);

#if SUPPORT_FW_DUMP
	if (fw_dump_buffer_full) {
		fw_dump_bulk_urb = urb;
		err = 0;
	} else {
		err = usb_submit_urb(urb, GFP_ATOMIC);
	}
#else
	err = usb_submit_urb(urb, GFP_ATOMIC);
#endif
	if (err != 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p failed to resubmit bulk_in_urb(%d)",
			       hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btmtk_usb_submit_bulk_in_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btmtk_usb_data *data;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size = HCI_MAX_FRAME_SIZE;

	if (hdev == NULL)
		return -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	printk("%s:%s\n", __func__, hdev->name);

	if (!data->bulk_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}
#if BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER
	pipe = usb_rcvintpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
	usb_fill_int_urb(urb, data->udev, pipe, buf, size, btmtk_usb_bulk_in_complete, hdev, 4);	/* interval : 1ms */
#else
	pipe = usb_rcvbulkpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
	usb_fill_bulk_urb(urb, data->udev, pipe, buf, size, btmtk_usb_bulk_in_complete, hdev);
#endif

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->bulk_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btmtk_usb_isoc_in_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	int i, err;

	printk("%s: %s urb %p status %d count %d", __func__, hdev->name,
	       urb, urb->status, urb->actual_length);

	if (urb->status == 0) {
		for (i = 0; i < urb->number_of_packets; i++) {
			unsigned int offset = urb->iso_frame_desc[i].offset;
			unsigned int length = urb->iso_frame_desc[i].actual_length;

			if (urb->iso_frame_desc[i].status)
				continue;

			hdev->stat.byte_rx += length;

			if (hci_recv_fragment(hdev, HCI_SCODATA_PKT,
					      urb->transfer_buffer + offset, length) < 0) {
				printk("%s corrupted SCO packet", hdev->name);
				hdev->stat.err_rx++;
			}
		}
	}

	if (!test_bit(BTUSB_ISOC_RUNNING, &data->flags))
		return;

	usb_anchor_urb(urb, &data->isoc_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p failed to resubmit iso_in_urb(%d)",
			       hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	BT_DBG("len %d mtu %d", len, mtu);

	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu; i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

static int btmtk_usb_submit_isoc_in_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;
	struct btmtk_usb_data *data;

	if (hdev == NULL)
		return -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	if (!data->isoc_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize) * BTUSB_MAX_ISOC_FRAMES;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvisocpipe(data->udev, data->isoc_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, data->udev, pipe, buf, size, btmtk_usb_isoc_in_complete,
			 hdev, data->isoc_rx_ep->bInterval);

	urb->transfer_flags = URB_FREE_BUFFER | URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, size, le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &data->isoc_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static int btmtk_usb_open(struct hci_dev *hdev)
{
	struct btmtk_usb_data *data;
	int err;

	if (hdev == NULL)
		return -1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	printk("%s in new kernel\n", __func__);

	/* assign to meta hdev */
	meta_hdev = hdev;

	g_data = data;

	/* clayton: add count to this interface to avoid autosuspend */
	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		return err;

	data->intf->needs_remote_wakeup = 1;

	/* clayton: set bit[HCI_RUNNING] to 1 */
	if (test_and_set_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	/* clayton: set bit[BTUSB_INTR_RUNNING] to 1 */
	if (test_and_set_bit(BTUSB_INTR_RUNNING, &data->flags))
		goto done;

	/* clayton: polling to receive interrupt from usb device */
	err = btmtk_usb_submit_intr_urb(hdev, GFP_KERNEL);
	if (err < 0)
		goto failed;

	/* clayton: polling to receive ACL data from usb device */
	err = btmtk_usb_submit_bulk_in_urb(hdev, GFP_KERNEL);
	if (err < 0) {
		usb_kill_anchored_urbs(&data->intr_anchor);
		goto failed;
	}
	/* clayton: set bit[BTUSB_BULK_RUNNING] to 1 */
	set_bit(BTUSB_BULK_RUNNING, &data->flags);

	set_bit(HCI_RAW, &hdev->flags);	/* BlueAngel has own initialization procedure, so we don't do initialization like hci_init_req */
#if LOAD_PROFILE
	btmtk_usb_load_profile(hdev);
#endif

#if SUPPORT_FW_DUMP
	if (data->fw_dump_tsk == NULL)
	{
		sema_init(&data->fw_dump_semaphore, 0);
		data->fw_dump_tsk =
		    kthread_create(btmtk_usb_fw_dump_thread, (void *)data,
				   "btmtk_usb_fw_dump_thread");
		if (IS_ERR(data->fw_dump_tsk)) {
			printk("%s : create fw dump thread failed!\n", __func__);
			err = PTR_ERR(data->fw_dump_tsk);
			data->fw_dump_tsk = NULL;
			usb_kill_anchored_urbs(&data->intr_anchor);
			usb_kill_anchored_urbs(&data->bulk_anchor);
			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
			goto failed;
		}
		fw_dump_task_should_stop = 0;
		if (fw_dump_ptr == NULL)
		{
			fw_dump_ptr = kmalloc(FW_DUMP_BUF_SIZE, GFP_ATOMIC);
		}
		if (fw_dump_ptr == NULL) {
			printk("%s : kmalloc(%d) failed!\n", __func__, FW_DUMP_BUF_SIZE);
			usb_kill_anchored_urbs(&data->intr_anchor);
			usb_kill_anchored_urbs(&data->bulk_anchor);
			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
			goto failed;
		}
		memset(fw_dump_ptr, 0, FW_DUMP_BUF_SIZE);

		fw_dump_file = NULL;
		fw_dump_read_ptr = fw_dump_ptr;
		fw_dump_write_ptr = fw_dump_ptr;
		fw_dump_total_read_size = 0;
		fw_dump_total_write_size = 0;
		fw_dump_buffer_used_size = 0;
		fw_dump_buffer_full = 0;
		fw_dump_bulk_urb = NULL;
		data->fw_dump_end_check_tsk = NULL;
		wake_up_process(data->fw_dump_tsk);
	}
#endif

 done:
	usb_autopm_put_interface(data->intf);
	printk("%s:successfully\n", __func__);
	return 0;

 failed:
	printk("%s:failed\n", __func__);
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);
	clear_bit(HCI_RUNNING, &hdev->flags);
	usb_autopm_put_interface(data->intf);
	return err;
}

static void btmtk_usb_stop_traffic(struct btmtk_usb_data *data)
{
	printk("%s:start\n", __func__);

	usb_kill_anchored_urbs(&data->intr_anchor);
	usb_kill_anchored_urbs(&data->bulk_anchor);
	/* SCO */
	/* usb_kill_anchored_urbs(&data->isoc_anchor); */

	printk("%s:end\n", __func__);
}

static int btmtk_usb_close(struct hci_dev *hdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	int err;
	err = 0;

	printk("%s need_reset_stack:%d\n", __func__, need_reset_stack);

	/* SCO */
	/* clear_bit(BTUSB_ISOC_RUNNING, &data->flags); */
	clear_bit(BTUSB_BULK_RUNNING, &data->flags);
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);

	btmtk_usb_stop_traffic(data);

	if (err < 0)
		goto failed;

	if (need_reset_stack) {
		/* wake up interrupt to let stack to read data from driver to trigger reset */
		printk("%s:wake up inq\n", __func__);
		wake_up_interruptible(&inq);
	}

    isbtready = 1;
	printk("%s:successfully\n", __func__);
	return 0;

 failed:
	/* usb_scuttle_anchored_urbs(&data->deferred); */
	isbtready = 0;
	return 0;
}

static int btmtk_usb_flush(struct hci_dev *hdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif

	BT_DBG("%s", __func__);

	usb_kill_anchored_urbs(&data->tx_anchor);

	return 0;
}

static void btmtk_usb_tx_complete_meta(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);

	if (!test_bit(HCI_RUNNING, &hdev->flags) && (metaMode == 0)) {
		printk("tx complete error\n");
		goto done;
	}
done:

	data->meta_tx = 0;
	/* printk("\tACL TX %s() done meta_tx=%d\n", __FUNCTION__, data->meta_tx);*/
	kfree(urb->setup_packet);
}

static void btmtk_usb_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;
	struct btmtk_usb_data *data;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	BT_DBG("%s: %s urb %p status %d count %d\n", __func__, hdev->name,
	       urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

 done:
	spin_lock(&data->txlock);
	data->tx_in_flight--;
	spin_unlock(&data->txlock);

	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static void btmtk_usb_isoc_tx_complete_meta(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	if (!test_bit(HCI_RUNNING, &hdev->flags) && (metaMode == 0))
		goto done;

 done:
	kfree(urb->setup_packet);
}



static void btmtk_usb_isoc_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;

	BT_DBG("%s: %s urb %p status %d count %d", __func__, hdev->name,
	       urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

 done:
	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static int btmtk_usb_send_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;
	unsigned char *dPoint;
	struct btmtk_usb_data *data;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned int pipe;
	int err, i, length;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	BT_DBG("%s\n", __func__);

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		return 0;
		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		dr = kmalloc(sizeof(*dr), GFP_ATOMIC);
		if (!dr) {
			usb_free_urb(urb);
			return -ENOMEM;
		}
		printk("HCI command:\n");


		dr->bRequestType = data->cmdreq_type;
		dr->bRequest = 0;
		dr->wIndex = 0;
		dr->wValue = 0;
		dr->wLength = __cpu_to_le16(skb->len);
		printk("bRequestType = %d\n", dr->bRequestType);
		printk("wLength = %d\n", dr->wLength);


		pipe = usb_sndctrlpipe(data->udev, 0x00);

		if (test_bit(HCI_RUNNING, &hdev->flags)) {
			u16 op_code;
			memcpy(&op_code, skb->data, 2);
			BT_DBG("ogf = %x\n", (op_code & 0xfc00) >> 10);
			BT_DBG("ocf = %x\n", op_code & 0x03ff);
		}

		length = skb->len;
		dPoint = skb->data;
		printk("skb length = %d\n", length);


		for (i = 0; i < length; i++) {
			printk("0x%02x", dPoint[i]);
		}
		printk("\n");

		usb_fill_control_urb(urb, data->udev, pipe, (void *)dr,
				     skb->data, skb->len, btmtk_usb_tx_complete, skb);

		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		if (!data->bulk_tx_ep)
			return -ENODEV;

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndbulkpipe(data->udev, data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, data->udev, pipe,
				  skb->data, skb->len, btmtk_usb_tx_complete, skb);

		hdev->stat.acl_tx++;
		BT_DBG("HCI_ACLDATA_PKT:\n");
		break;

	case HCI_SCODATA_PKT:
		if (!data->isoc_tx_ep || hdev->conn_hash.sco_num < 1)
			return -ENODEV;

		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndisocpipe(data->udev, data->isoc_tx_ep->bEndpointAddress);

		usb_fill_int_urb(urb, data->udev, pipe,
				 skb->data, skb->len, btmtk_usb_isoc_tx_complete,
				 skb, data->isoc_tx_ep->bInterval);

		urb->transfer_flags = URB_ISO_ASAP;

		__fill_isoc_descriptor(urb, skb->len,
				       le16_to_cpu(data->isoc_tx_ep->wMaxPacketSize));

		hdev->stat.sco_tx++;
		goto skip_waking;

	default:
		return -EILSEQ;
	}

	err = inc_tx(data);

	if (err) {
		usb_anchor_urb(urb, &data->deferred);
		schedule_work(&data->waker);
		err = 0;
		goto done;
	}

 skip_waking:
	usb_anchor_urb(urb, &data->tx_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else {
		usb_mark_last_busy(data->udev);
	}

 done:
	usb_free_urb(urb);
	return err;
}

#if LOAD_PROFILE
static void btmtk_usb_ctrl_complete(struct urb *urb)
{
	BT_DBG("btmtk_usb_ctrl_complete\n");
	kfree(urb->setup_packet);
	kfree(urb->transfer_buffer);
}

static int btmtk_usb_submit_ctrl_urb(struct hci_dev *hdev, char *buf, int length)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	struct usb_ctrlrequest *setup_packet;
	struct urb *urb;
	unsigned int pipe;
	char *send_buf;
	int err;

	BT_DBG("btmtk_usb_submit_ctrl_urb, length=%d\n", length);

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		printk("btmtk_usb_submit_ctrl_urb error1\n");
		return -ENOMEM;
	}

	send_buf = kmalloc(length, GFP_ATOMIC);
	if (!send_buf) {
		printk("btmtk_usb_submit_ctrl_urb error2\n");
		usb_free_urb(urb);
		return -ENOMEM;
	}
	memcpy(send_buf, buf, length);

	setup_packet = kmalloc(sizeof(*setup_packet), GFP_ATOMIC);
	if (!setup_packet) {
		printk("btmtk_usb_submit_ctrl_urb error3\n");
		usb_free_urb(urb);
		kfree(send_buf);
		return -ENOMEM;
	}

	setup_packet->bRequestType = data->cmdreq_type;
	setup_packet->bRequest = 0;
	setup_packet->wIndex = 0;
	setup_packet->wValue = 0;
	setup_packet->wLength = __cpu_to_le16(length);

	pipe = usb_sndctrlpipe(data->udev, 0x00);

	usb_fill_control_urb(urb, data->udev, pipe, (void *)setup_packet,
			     send_buf, length, btmtk_usb_ctrl_complete, hdev);

	usb_anchor_urb(urb, &data->tx_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else {
		usb_mark_last_busy(data->udev);
	}

	usb_free_urb(urb);
	return err;
}

int _ascii_to_int(char buf)
{
	switch (buf) {
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	default:
		return buf - '0';
	}
}

void btmtk_usb_load_profile(struct hci_dev *hdev)
{
	mm_segment_t old_fs;
	struct file *file = NULL;
	unsigned char *buf;
	unsigned char target_buf[256 + 4] = { 0 };
	int i = 0;
	int j = 4;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file = filp_open("/etc/Wireless/RT2870STA/BT_CONFIG.dat", O_RDONLY, 0);
	if (IS_ERR(file)) {
		set_fs(old_fs);
		return;
	}

	buf = kmalloc(1280, GFP_ATOMIC);
	if (!buf) {
		printk
		    ("malloc error when parsing /etc/Wireless/RT2870STA/BT_CONFIG.dat, exiting...\n");
		filp_close(file, NULL);
		set_fs(old_fs);
		return;
	}

	printk("/etc/Wireless/RT2870STA/BT_CONFIG.dat exits, parse it.\n");
	memset(buf, 0, 1280);
	file->f_op->read(file, buf, 1280, &file->f_pos);

	for (i = 0; i < 1280; i++) {
		if (buf[i] == '\r')
			continue;
		if (buf[i] == '\n')
			continue;
		if (buf[i] == 0)
			break;
		if (buf[i] == '0' && buf[i + 1] == 'x') {
			i += 1;
			continue;
		}

		{
			if (buf[i + 1] == '\r' || buf[i + 1] == '\n' || buf[i + 1] == 0) {
				target_buf[j] = _ascii_to_int(buf[i]);
				j++;
			} else {
				target_buf[j] =
				    _ascii_to_int(buf[i]) << 4 | _ascii_to_int(buf[i + 1]);
				j++;
				i++;
			}
		}
	}
	kfree(buf);
	filp_close(file, NULL);
	set_fs(old_fs);

	/* Send to dongle */
	{
		target_buf[0] = 0xc3;
		target_buf[1] = 0xfc;
		target_buf[2] = j - 4 + 1;
		target_buf[3] = 0x01;

		printk("Profile Configuration : (%d)\n", j);
		for (i = 0; i < j; i++) {
			printk("    0x%02X\n", target_buf[i]);
		}

		if (hdev != NULL)
			btmtk_usb_submit_ctrl_urb(hdev, target_buf, j);
	}
}
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
static void btmtk_usb_destruct(struct hci_dev *hdev)
{
/* struct btmtk_usb_data *data = hdev->driver_data; */
}
#endif

static void btmtk_usb_notify(struct hci_dev *hdev, unsigned int evt)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif

	BT_DBG("%s evt %d", hdev->name, evt);

	if (hdev->conn_hash.sco_num != data->sco_num) {
		data->sco_num = hdev->conn_hash.sco_num;
		schedule_work(&data->work);
	}
}

#if SUPPORT_BT_ATE
	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
static int btmtk_usb_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg)
{
#define ATE_TRIGGER	_IOW('H', 300, int)
#define ATE_PARAM_LEN	_IOW('H', 301, int)
#define ATE_PARAM_0	    _IOW('H', 302, unsigned char)
#define ATE_PARAM_1	    _IOW('H', 303, unsigned char)
#define ATE_PARAM_2	_IOW('H', 304, unsigned char)
#define ATE_PARAM_3	_IOW('H', 305, unsigned char)
#define ATE_PARAM_4	_IOW('H', 306, unsigned char)
#define ATE_PARAM_5	_IOW('H', 307, unsigned char)
#define ATE_PARAM_6	_IOW('H', 308, unsigned char)
#define ATE_PARAM_7	_IOW('H', 309, unsigned char)
#define ATE_PARAM_8	_IOW('H', 310, unsigned char)
#define ATE_PARAM_9	_IOW('H', 311, unsigned char)
#define ATE_PARAM_10	_IOW('H', 312, unsigned char)
#define ATE_PARAM_11	_IOW('H', 313, unsigned char)
#define ATE_PARAM_12	_IOW('H', 314, unsigned char)
#define ATE_PARAM_13	_IOW('H', 315, unsigned char)
#define ATE_PARAM_14	_IOW('H', 316, unsigned char)
#define ATE_PARAM_15	_IOW('H', 317, unsigned char)
#define ATE_PARAM_16	_IOW('H', 318, unsigned char)
#define ATE_PARAM_17	_IOW('H', 319, unsigned char)
#define ATE_READ_DRIVER_VERSION     _IOW('H', 320, int)
#define ATE_READ_ROM_PATCH_VERSION  _IOW('H', 321, int)
#define ATE_READ_FW_VERSION         _IOW('H', 322, int)
#define ATE_READ_PROB_COUNTER       _IOW('H', 323, int)
/* SCO */
#define FORCE_SCO_ISOC_ENABLE    _IOW('H', 1, int)
#define FORCE_SCO_ISOC_DISABLE   _IOW('H', 0, int)



#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif

	static char cmd_str[32] = { 0 };
	static int cmd_len;
	int copy_result;

	switch (cmd) {
	case ATE_TRIGGER:
		{
			int i;
			printk("Send ATE cmd string:(%d)\n", cmd_len);

			for (i = 0; i < cmd_len; i++) {
				if (i == 8)
					printk("\n");

				if (i == 0 || i == 8)
					printk("    ");

				printk("%02X ", (unsigned char)cmd_str[i]);
			}
			printk("\n");

			usb_send_ate_hci_cmd(data->udev, cmd_str, cmd_len);

			break;
		}
	case ATE_PARAM_LEN:
		cmd_len = arg & 0xff;
		break;
	case ATE_PARAM_0:
		cmd_str[0] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_1:
		cmd_str[1] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_2:
		cmd_str[2] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_3:
		cmd_str[3] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_4:
		cmd_str[4] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_5:
		cmd_str[5] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_6:
		cmd_str[6] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_7:
		cmd_str[7] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_8:
		cmd_str[8] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_9:
		cmd_str[9] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_10:
		cmd_str[10] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_11:
		cmd_str[11] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_12:
		cmd_str[12] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_13:
		cmd_str[13] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_14:
		cmd_str[14] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_15:
		cmd_str[15] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_16:
		cmd_str[16] = (unsigned char)(arg & 0xff);
		break;
	case ATE_PARAM_17:
		cmd_str[17] = (unsigned char)(arg & 0xff);
		break;

	case ATE_READ_DRIVER_VERSION:
		copy_result = copy_to_user((char *)arg, driver_version, sizeof(driver_version));
		if (copy_result)
			printk("ATE_READ_DRIVER_VERSION copy fail:%d", copy_result);
		break;
	case ATE_READ_ROM_PATCH_VERSION:
		copy_result = copy_to_user((char *)arg, rom_patch_version, sizeof(rom_patch_version));
		if (copy_result)
			printk("ATE_READ_ROM_PATCH_VERSION copy fail:%d", copy_result);
		break;
	case ATE_READ_FW_VERSION:
		copy_result = copy_to_user((char *)arg, fw_version, sizeof(fw_version));
		if (copy_result)
			printk("ATE_READ_FW_VERSION copy fail:%d", copy_result);
		break;
	case ATE_READ_PROB_COUNTER:
		copy_result = copy_to_user((char *)arg, &probe_counter, 1);
		if (copy_result)
			printk("ATE_READ_PROB_COUNTER copy fail:%d", copy_result);
		break;

	case FORCE_SCO_ISOC_ENABLE:
		printk(KERN_INFO "FORCE  SCO btusb_probe sco_num=%d ", hdev->conn_hash.sco_num);
		if (!hdev->conn_hash.sco_num)
			hdev->conn_hash.sco_num = 1;
		hdev->voice_setting = 0x0020;
		btmtk_usb_notify(hdev, HCI_NOTIFY_VOICE_SETTING);

		break;
	case FORCE_SCO_ISOC_DISABLE:
		printk(KERN_INFO "FORCE  SCO btusb_probe sco_num=%d ", hdev->conn_hash.sco_num);
		if (hdev->conn_hash.sco_num)
			hdev->conn_hash.sco_num = 0;
		btmtk_usb_notify(hdev, HCI_NOTIFY_VOICE_SETTING);

		break;

	default:
		break;
	}

	return 0;
}
	#endif
#endif				/* SUPPORT_BT_ATE */

static inline int __set_isoc_interface(struct hci_dev *hdev, int altsetting)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	struct usb_interface *intf = data->isoc;
	struct usb_endpoint_descriptor *ep_desc;
	int i, err;

	if (!data->isoc)
		return -ENODEV;

	err = usb_set_interface(data->udev, 1, altsetting);
	if (err < 0) {
		printk("%s setting interface failed (%d)", hdev->name, -err);
		return err;
	}

	data->isoc_altsetting = altsetting;

	data->isoc_tx_ep = NULL;
	data->isoc_rx_ep = NULL;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (!data->isoc_tx_ep && usb_endpoint_is_isoc_out(ep_desc)) {
			data->isoc_tx_ep = ep_desc;
			continue;
		}

		if (!data->isoc_rx_ep && usb_endpoint_is_isoc_in(ep_desc)) {
			data->isoc_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->isoc_tx_ep || !data->isoc_rx_ep) {
		printk("%s invalid SCO descriptors", hdev->name);
		return -ENODEV;
	}

	return 0;
}

static void btmtk_usb_work(struct work_struct *work)
{
	struct btmtk_usb_data *data = container_of(work, struct btmtk_usb_data, work);
	struct hci_dev *hdev = data->hdev;
	int new_alts;
	int err;

	printk("%s\n", __func__);

	printk("\t%s() sco_num=%d\n", __func__, hdev->conn_hash.sco_num);
	if (hdev->conn_hash.sco_num > 0) {
		if (!test_bit(BTUSB_DID_ISO_RESUME, &data->flags)) {
			err = usb_autopm_get_interface(data->isoc ? data->isoc : data->intf);
			if (err < 0) {
				printk("%s: get isoc interface fail\n", __func__);
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
				usb_kill_anchored_urbs(&data->isoc_anchor);
				return;
			}

			set_bit(BTUSB_DID_ISO_RESUME, &data->flags);
		}

		if (hdev->voice_setting & 0x0020) {
			static const int alts[3] = { 2, 4, 5 };
			new_alts = alts[hdev->conn_hash.sco_num - 1];
			printk("\t%s() 0x0020 new_alts=%d\n", __FUNCTION__, new_alts);
		} else if (hdev->voice_setting & 0x2000) {
			new_alts = 4;
			printk("\t%s() 0x2000 new_alts=%d\n", __FUNCTION__, new_alts);
		} else {
			new_alts = hdev->conn_hash.sco_num;
			printk("\t%s() ? new_alts=%d\n", __FUNCTION__, new_alts);
		}

		if (data->isoc_altsetting != new_alts) {
			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
			usb_kill_anchored_urbs(&data->isoc_anchor);

			if (__set_isoc_interface(hdev, new_alts) < 0)
				return;
		}

		if (!test_and_set_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
			if (btmtk_usb_submit_isoc_in_urb(hdev, GFP_KERNEL) < 0) {
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
				printk("%s: submit isoc urb fail\n", __func__);
			} else
				btmtk_usb_submit_isoc_in_urb(hdev, GFP_KERNEL);
		}
	} else {
		clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		usb_kill_anchored_urbs(&data->isoc_anchor);

		__set_isoc_interface(hdev, 0);

		if (test_and_clear_bit(BTUSB_DID_ISO_RESUME, &data->flags))
			usb_autopm_put_interface(data->isoc ? data->isoc : data->intf);
	}
}

static void btmtk_usb_waker(struct work_struct *work)
{
	struct btmtk_usb_data *data = container_of(work, struct btmtk_usb_data, waker);
	int err;

	err = usb_autopm_get_interface(data->intf);

	if (err < 0)
		return;

	usb_autopm_put_interface(data->intf);
}


static irqreturn_t mt76xx_wobt_isr(int irq, void *dev)
{
	struct btmtk_usb_data *data = (struct btmtk_usb_data *)dev;
	printk("%s()\n", __func__);
	disable_irq_nosync(data->wobt_irq);
	atomic_dec(&(data->irq_enable_count));
	printk("%s():disable BT IRQ\n", __func__);
	return IRQ_HANDLED;
}


static int mt76xxRegisterBTIrq(struct btmtk_usb_data *data)
{
	struct device_node *eint_node = NULL;
	eint_node = of_find_compatible_node(NULL, NULL, "mediatek,mt76xx_bt_ctrl");
	printk("btmtk:%s()\n", __func__);
	if (eint_node) {
		printk("Get mt76xx_bt_ctrl compatible node\n");
		data->wobt_irq = irq_of_parse_and_map(eint_node, 0);
		printk("btmtk:wobt_irq number:%d", data->wobt_irq);
		if (data->wobt_irq) {
			int interrupts[2];
			of_property_read_u32_array(eint_node, "interrupts",
						   interrupts, ARRAY_SIZE(interrupts));
			data->wobt_irqlevel = interrupts[1];
			if (request_irq(data->wobt_irq, mt76xx_wobt_isr,
					data->wobt_irqlevel, "mt76xx_bt_ctrl-eint", data))
				printk("MT76xx WOBTIRQ LINE NOT AVAILABLE!!\n");
			else {
				printk("%s():disable BT IRQ\n", __func__);
				disable_irq_nosync(data->wobt_irq);
			}

		} else
			printk("can't find mt76xx_bt_ctrl irq\n");

	} else {
		data->wobt_irq = 0;
		printk("can't find mt76xx_bt_ctrl compatible node\n");
	}
	printk("btmtk:%s(): end\n", __func__);
	return 0;
}


static int mt76xxUnRegisterBTIrq(struct btmtk_usb_data *data)
{
	printk("%s()\n", __func__);
	if (data->wobt_irq)
		free_irq(data->wobt_irq, data);
	return 0;
}

void suspend_workqueue(struct work_struct *work)
{
	printk("%s\n", __func__);
	btmtk_usb_send_assert_cmd(g_data->udev);
}

static int btmtk_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct btmtk_usb_data *data;
	struct usb_endpoint_descriptor *ep_desc;
	int i, err;
	struct hci_dev *hdev;

	printk("===========================================\n");
	printk("Mediatek Bluetooth USB driver ver %s\n", VERSION);
	printk("===========================================\n");
	memset(driver_version, 0, sizeof(driver_version));
	memcpy(driver_version, VERSION, sizeof(VERSION));
	probe_counter++;
	isbtready = 0;
	is_assert = 0;
	printk("probe_counter = %d\n", probe_counter);

	printk("btmtk_usb_probe begin\n");

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if ( btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT )
        btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_ERROR_PROBE);
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT )
        btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_ERROR_PROBE);
    else
        btmtk_usb_set_state(BTMTK_USB_STATE_PROBE);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif

	/* interface numbers are hardcoded in the spec */
	if (intf->cur_altsetting->desc.bInterfaceNumber != 0) {
		printk("[ERR] interface number != 0 (%d)\n",
		       intf->cur_altsetting->desc.bInterfaceNumber);
		printk("btmtk_usb_probe end Error 1\n");
		return -ENODEV;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);

	if (!data) {
		printk("[ERR] kzalloc failed !\n");
		printk("btmtk_usb_probe end Error 2\n");
		return -ENOMEM;
	}

    g_data = data;
	mt76xxRegisterBTIrq(data);
	/* clayton: set the endpoint type of the interface to btmtk_usb_data */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;	/* clayton: Endpoint descriptor */

		if (!data->intr_ep && usb_endpoint_is_int_in(ep_desc)) {
			data->intr_ep = ep_desc;
			continue;
		}

		if (!data->bulk_tx_ep && usb_endpoint_is_bulk_out(ep_desc)) {
			data->bulk_tx_ep = ep_desc;
			continue;
		}

		if (!data->bulk_rx_ep && usb_endpoint_is_bulk_in(ep_desc)) {
			data->bulk_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->intr_ep || !data->bulk_tx_ep || !data->bulk_rx_ep) {
		kfree(data);
		printk("btmtk_usb_probe end Error 3\n");
		return -ENODEV;
	}

    data->fw_dump_tsk = NULL;
	data->cmdreq_type = USB_TYPE_CLASS;

	data->udev = interface_to_usbdev(intf);
	meta_udev = data->udev;

	data->intf = intf;

	spin_lock_init(&data->lock);
	INIT_WORK(&data->work, btmtk_usb_work);
	INIT_WORK(&data->waker, btmtk_usb_waker);
	spin_lock_init(&data->txlock);

	data->meta_tx = 0;

	/* clayton: init all usb anchor */
	init_usb_anchor(&data->tx_anchor);
	init_usb_anchor(&data->intr_anchor);
	init_usb_anchor(&data->bulk_anchor);
	init_usb_anchor(&data->isoc_anchor);
	init_usb_anchor(&data->deferred);

	data->io_buf = kmalloc(256, GFP_ATOMIC);

	btmtk_usb_switch_iobase(data, WLAN);

	metabuffer->read_p = 0;
	metabuffer->write_p = 0;
	memset(metabuffer->buffer,0,META_BUFFER_SIZE);

	if(i_buf == NULL)
	 i_buf = kmalloc(BUFFER_SIZE, GFP_ATOMIC);
	if(o_buf == NULL)
	 o_buf = kmalloc(BUFFER_SIZE, GFP_ATOMIC);

	if(i_buf == NULL || o_buf == NULL){
		printk("%s,malloc memory fail\n",__func__);
		return -ENOMEM;
	}

	/* clayton: according to the chip id, load f/w or rom patch */
	btmtk_usb_cap_init(data);

	if (data->need_load_rom_patch) {
#if (SUPPORT_MT6632 || SUPPORT_MT7637)
		if (data->chip_id==0x7636 || data->chip_id==0x7637 || data->chip_id==0x6632)
			err = btmtk_usb_load_rom_patch_763x(data);
		else
			err = btmtk_usb_load_rom_patch(data);

#else
		err = btmtk_usb_load_rom_patch(data);
#endif

		if (err < 0) {
			kfree(data->io_buf);
			kfree(data);
			printk("btmtk_usb_probe end Error 4\n");
			return err;
		}
	}

	if (data->need_load_fw) {
		err = btmtk_usb_load_fw(data);

		if (err < 0) {
			kfree(data->io_buf);
			kfree(data);
			printk("btmtk_usb_probe end Error 5\n");
			return err;
		}
	}
	/* allocate a hci device */
	hdev = hci_alloc_dev();
	if (!hdev) {
		kfree(data);
		printk("btmtk_usb_probe end Error 6\n");
		return -ENOMEM;
	}

	hdev->bus = HCI_USB;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	hci_set_drvdata(hdev, data);
#else
	hdev->driver_data = data;
#endif

	data->hdev = hdev;


	/* clayton: set the interface device to the hdev's parent */
	SET_HCIDEV_DEV(hdev, &intf->dev);

	hdev->open = btmtk_usb_open;	/* used when device is power on/receving HCIDEVUP command */
	hdev->close = btmtk_usb_close;	/* remove interrupt-in/bulk-in urb.used when  power off/receiving HCIDEVDOWN command */
	hdev->flush = btmtk_usb_flush;	/* Not necessary.remove tx urb. Used in device reset/power on fail/power off */
	hdev->send = btmtk_usb_send_frame;	/* used when transfer data to device */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
	hdev->destruct = btmtk_usb_destruct;
#endif
	hdev->notify = btmtk_usb_notify;	/* Not necessary. Used to adjust sco related setting */
#if SUPPORT_BT_ATE
	#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
	hdev->ioctl = btmtk_usb_ioctl;	/* Not necessary. used when getting command from blueAngel via ioctl */
	#endif
#endif

	/* Interface numbers are hardcoded in the specification */
	data->isoc = usb_ifnum_to_if(data->udev, 1);

	/* initialize the IRQ enable counter */
	atomic_set(&(data->irq_enable_count), 0);

	if (data->rom_patch_bin_file_name)
		kfree(data->rom_patch_bin_file_name);

	/* bind isoc interface to usb driver */
	if (data->isoc) {
		err = usb_driver_claim_interface(&btmtk_usb_driver, data->isoc, data);
		if (err < 0) {
			hci_free_dev(hdev);
			kfree(data->io_buf);
			kfree(data);
			printk("btmtk_usb_probe end Error 7\n");
			return err;
		}
	}

	err = hci_register_dev(hdev);
	if (err < 0) {
		hci_free_dev(hdev);
		kfree(data->io_buf);
		kfree(data);
		printk("btmtk_usb_probe end Error 8\n");
		return err;
	}

	usb_set_intfdata(intf, data);
	isUsbDisconnet = 0;

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    {
        register_early_suspend_func = (register_early_suspend)kallsyms_lookup_name(register_early_suspend_func_name);
        register_late_resume_func = (register_late_resume)kallsyms_lookup_name(register_late_resume_func_name);

        if ( register_early_suspend_func && register_late_resume_func )
        {
            printk("%s : Register early suspend/late resume nitofication success. \n",
                __FUNCTION__);
            register_early_suspend_func(&btmtk_usb_early_suspend);
            register_late_resume_func(&btmtk_usb_late_resume);
        }
        else
        {
            printk("%s : Failed to register early suspend/ late resume notification \n",
                __FUNCTION__);
        }

        mutex_lock(&btmtk_usb_state_mutex);
        if ( btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_PROBE )
            btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND);
        else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_PROBE )
            btmtk_usb_set_state(BTMTK_USB_STATE_RESUME);
        else
            btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
        mutex_unlock(&btmtk_usb_state_mutex);
    }
#endif

	printk("btmtk_usb_probe end\n");
	return 0;
}

static void btmtk_usb_disconnect(struct usb_interface *intf)
{
	struct btmtk_usb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev;

	if (!data)
		return;

	printk("%s begin\n", __func__);

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if ( btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND ||
         btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
         btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT )
    {
        printk("%s : disconnect hapens when driver is in suspend state, should stay in suspend state later !\n", __func__);
        btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT);
    }
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT )
    {
        printk("%s : disconnect hapens when driver is in resume state, should stay in resume state later !\n", __func__);
        btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT);
    }
    else
        btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif

	hdev = data->hdev;

	usb_set_intfdata(data->intf, NULL);

	if (data->isoc)
		usb_set_intfdata(data->isoc, NULL);

    if (data->fw_dump_tsk) {
        int wait_retry_conter=100;
        fw_dump_task_should_stop = 1;
        up(&data->fw_dump_semaphore);
        // stop is not necessary since fw_dump_task_should_stop is 1
        //kthread_stop(data->fw_dump_tsk);
        msleep(10);
        while(wait_retry_conter > 0 && data->fw_dump_tsk) {
            wait_retry_conter--;
            printk("%s : fw_dump_thread not stopped yet, wait 100ms.\n", __func__);
            msleep(100);
        }

        if (data->fw_dump_tsk) {
            printk("%s : ERROR! fw_dump_thread not stopped !!\n", __func__);
        }
    }

	hci_unregister_dev(hdev);

	if (intf == data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, data->intf);
	else if (data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, data->isoc);

	hci_free_dev(hdev);
	kfree(data->io_buf);

	isbtready = 0;
	metaCount = 0;
	metabuffer->read_p = 0;
	metabuffer->write_p = 0;

	/* reset the IRQ enable counter to zero */
	atomic_set(&(data->irq_enable_count), 0);

	if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
// don't free rom_patch buffer pointer, reuse it later.
//		if (data->need_load_rom_patch)
//			kfree(data->rom_patch);

		if (data->need_load_fw)
			kfree(data->fw_image);
	}

	printk("unregister bt irq\n");
	mt76xxUnRegisterBTIrq(data);

	isUsbDisconnet = 1;
	printk("btmtk: stop all URB\n");
	clear_bit(BTUSB_BULK_RUNNING, &data->flags);
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);
	clear_bit(BTUSB_ISOC_RUNNING, &data->flags);

    // Usually, there is nothing wrong if invoke here.
    // However, when doing aging test for dongle reset, some issue happen.
    // No need to invoke stop traffic again, since it's already handled by USB driver.
	//btmtk_usb_stop_traffic(data);

	cancel_work_sync(&data->work);
	cancel_work_sync(&data->waker);

	kfree(data);
    printk("%s end\n", __func__);
}

#ifdef CONFIG_PM
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
#define PMSG_IS_AUTO(msg)       (((msg).event & PM_EVENT_AUTO) != 0)
#endif
static int btmtk_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct btmtk_usb_data *data = usb_get_intfdata(intf);

	/*The value of meta mode  should be 1*/
	printk("%s: meta value:%d, suspend_count:%d\n", __func__, metaMode, data->suspend_count);

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif

	if ((data->suspend_count++)) {
		printk("%s:Has suspended. suspend_count:%d, meta value:%d\n", __func__, data->suspend_count, metaMode);
		return 0;
	}

#if BT_SEND_HCI_CMD_BEFORE_SUSPEND
	btmtk_usb_send_hci_suspend_cmd(data->udev);
#endif

	spin_lock_irq(&data->txlock);
	if (!(PMSG_IS_AUTO(message) && data->tx_in_flight)) {
		set_bit(BTUSB_SUSPENDING, &data->flags);
		spin_unlock_irq(&data->txlock);
		printk("Enter suspend\n");
		printk("tx in flight:%d\n", data->tx_in_flight);
	} else {
		spin_unlock_irq(&data->txlock);
		data->suspend_count--;
		printk("Current is busy\n");
		return -EBUSY;
	}

	cancel_work_sync(&data->work);

	btmtk_usb_stop_traffic(data);
	usb_kill_anchored_urbs(&data->tx_anchor);

	need_reset_stack = 1;

	return 0;
}

#if BT_SEND_HCI_CMD_BEFORE_STANDBY
int btmtk_usb_standby(void)
{
	printk("%s() begin\n", __func__);
	if (g_data != NULL)
	{
		printk("%s() g_data:0x%p\n", __func__, g_data);
		if (g_data->udev != NULL)
			printk("%s() g_data->udev:0x%p\n", __func__, g_data->udev);
		else
		{
			printk("%s() g_data->udev is NULL pointer!,return 0\n", __func__);
			return 0;
		}
	}
	else
	{
		printk("%s() g_data is NULL pointer!, return 0\n", __func__);
		return 0;
	}

	if ((g_data->suspend_count)) {
		printk("%s:Has suspended. suspend_count:%d, meta value:%d\n", __func__, g_data->suspend_count, metaMode);
		return 0;
	}

	/*The value of meta mode  should be 1*/
	printk("%s: meta value:%d, suspend_count:%d\n", __func__, metaMode, g_data->suspend_count);

#if BT_SEND_HCI_CMD_BEFORE_SUSPEND
	btmtk_usb_send_hci_suspend_cmd(g_data->udev);
#endif

	spin_lock_irq(&g_data->txlock);
	if (g_data->tx_in_flight) {
		set_bit(BTUSB_SUSPENDING, &g_data->flags);
		spin_unlock_irq(&g_data->txlock);
		printk("Enter suspend\n");
		printk("tx in flight:%d\n", g_data->tx_in_flight);
	} else {
		spin_unlock_irq(&g_data->txlock);
		printk("Current is busy\n");
		return -EBUSY;
	}

	cancel_work_sync(&g_data->work);

	btmtk_usb_stop_traffic(g_data);
	usb_kill_anchored_urbs(&g_data->tx_anchor);

	need_reset_stack = 1;
	printk("%s() end\n", __func__);
	return 0;
}
#endif

static void play_deferred(struct btmtk_usb_data *data)
{
	struct urb *urb;
	int err;

	while ((urb = usb_get_from_anchor(&data->deferred))) {
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (err < 0)
			break;

		data->tx_in_flight++;
	}

	usb_scuttle_anchored_urbs(&data->deferred);
}

static int meta_usb_submit_intr_urb(struct hci_dev *hdev)
{
	struct btmtk_usb_data *data;
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;

	if (hdev == NULL)
		return -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	data = hci_get_drvdata(hdev);
#else
	data = hdev->driver_data;
#endif

	if (!data->intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return -ENOMEM;

//	size = le16_to_cpu(data->intr_ep->wMaxPacketSize);
    size = le16_to_cpu(HCI_MAX_EVENT_SIZE);

	hciEventMaxSize = size;
	printk("%s : maximum packet size:%d\n", __func__, hciEventMaxSize);

	buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}
	/* clayton: generate a interrupt pipe, direction: usb device to cpu */
	pipe = usb_rcvintpipe(data->udev, data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, meta_udev, pipe, buf, size,
			 btmtk_usb_intr_complete, hdev, data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static int meta_usb_submit_bulk_in_urb(struct hci_dev *hdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
	struct btmtk_usb_data *data = hdev->driver_data;
#endif
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size = HCI_MAX_FRAME_SIZE;

	printk("%s:%s\n", __func__, hdev->name);

	if (!data->bulk_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, GFP_NOIO);
	if (!urb)
		return -ENOMEM;

	buf = kmalloc(size, GFP_NOIO);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}
#if BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER
	pipe = usb_rcvintpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
	usb_fill_int_urb(urb, data->udev, pipe, buf, size, btmtk_usb_bulk_in_complete, hdev, 4);	/* interval : 1ms */
#else
	pipe = usb_rcvbulkpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
	usb_fill_bulk_urb(urb, data->udev, pipe, buf, size, btmtk_usb_bulk_in_complete, hdev);
#endif

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->bulk_anchor);

	err = usb_submit_urb(urb, GFP_NOIO);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			printk("%s urb %p submission failed (%d)", hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}


static int btmtk_usb_resume(struct usb_interface *intf)
{
	struct btmtk_usb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev = data->hdev;
	int err = 0;
	printk("%s meta value:%d begin\n", __func__,metaMode);

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_RESUME);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif

	data->suspend_count--;

	if (data->suspend_count) {
		printk("%s data->suspend_count %d, return 0\n", __func__,data->suspend_count);
		return 0;
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    (void)hdev;
    (void)err;
    g_data = data;
	printk("%s end\n", __func__);
	return 0;
#else
	if (!test_bit(HCI_RUNNING, &hdev->flags) && (metaMode == 0)) {
		/* BT_RESET */
		if ( btmtk_usb_send_hci_reset_cmd(data->udev) < 0 )
		    btmtk_usb_send_assert_cmd(g_data->udev);

		/* for WoBLE/WoW low power */
		if ( btmtk_usb_send_hci_set_ce_cmd(data->udev) < 0 )
		    btmtk_usb_send_assert_cmd(g_data->udev);

		printk("%s goto done,\n", __func__);
		goto done;
	}

	/* BT_RESET */
	if ( btmtk_usb_send_hci_reset_cmd(data->udev) < 0 )
	{
    	if (test_bit(BTUSB_BULK_RUNNING, &data->flags) || metaMode != 0) {
    		printk("%s: BT USB BULK RUNNING\n", __func__);
    		err = meta_usb_submit_bulk_in_urb(hdev);
    		if (err < 0) {
    			printk("%s: error number:%d\n", __func__, err);
    			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
    			goto failed;
    		}
    	}

	    btmtk_usb_send_assert_cmd(g_data->udev);
	    goto failed;
    }

	/* for WoBLE/WoW low power */
	if ( btmtk_usb_send_hci_set_ce_cmd(data->udev) < 0 )
	{
    	if (test_bit(BTUSB_BULK_RUNNING, &data->flags) || metaMode != 0) {
    		printk("%s: BT USB BULK RUNNING\n", __func__);
    		err = meta_usb_submit_bulk_in_urb(hdev);
    		if (err < 0) {
    			printk("%s: error number:%d\n", __func__, err);
    			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
    			goto failed;
    		}
    	}

	    btmtk_usb_send_assert_cmd(g_data->udev);
	    goto failed;
	}

	if (test_bit(BTUSB_INTR_RUNNING, &data->flags) || metaMode != 0) {
		printk("%s: BT USB INTR RUNNING\n", __func__);
		err = meta_usb_submit_intr_urb(hdev);
		if (err < 0) {
			printk("%s: error number:%d\n", __func__, err);
			clear_bit(BTUSB_INTR_RUNNING, &data->flags);
			goto failed;
		}
	}

	if (test_bit(BTUSB_BULK_RUNNING, &data->flags) || metaMode != 0) {
		printk("%s: BT USB BULK RUNNING\n", __func__);
		err = meta_usb_submit_bulk_in_urb(hdev);
		if (err < 0) {
			printk("%s: error number:%d\n", __func__, err);
			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
			goto failed;
		}
	}

	if (test_bit(BTUSB_ISOC_RUNNING, &data->flags) || metaMode != 0) {
		if (btmtk_usb_submit_isoc_in_urb(hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		else
			btmtk_usb_submit_isoc_in_urb(hdev, GFP_NOIO);
	}

	if (need_reset_stack) {
		/* wake up interrupt to let stack to read data from driver to trigger reset */
		printk("%s : need_reset_stack=%d, wake up inq\n", __func__, need_reset_stack);
		wake_up_interruptible(&inq);
	}

	spin_lock_irq(&data->txlock);
	play_deferred(data);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);
	schedule_work(&data->work);

	printk("%s end\n", __func__);
	return 0;

 failed:
	usb_scuttle_anchored_urbs(&data->deferred);
 done:
	spin_lock_irq(&data->txlock);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);
	printk("%s skip send cmd to FW end\n", __func__);
	return err;
#endif
}
#endif


static int btmtk_usb_meta_send_data(const unsigned char *buffer, const unsigned int length)
{
	int ret = 0;

	if (buffer[0] != 0x01) {
		printk("the data from meta isn't HCI command, value:%d", buffer[0]);
	} else {
		char *buf = kmalloc(length - 1, GFP_ATOMIC);

		if (buf == NULL) {
			printk("%s:allocate memory fail\n", __func__);
			return -ENOMEM;
		}

        memset(buf, 0, length-1);
		memcpy(buf, buffer + 1, length - 1);	/* without copy the first byte about 0x01 */
		ret =
		    usb_control_msg(meta_udev, usb_sndctrlpipe(meta_udev, 0), 0x0,
				    DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, length-1,
				    CONTROL_TIMEOUT_JIFFIES);

		kfree(buf);
	}

	if (ret < 0) {
		printk("%s error1(%d)\n", __func__, ret);
		return ret;
	} else {
		//pr_info("send HCI command length:%d\n", ret);
	}

	return length;
}

/*
static struct usb_device_id btmtk_usb_table[] = {
	// Mediatek MT7662
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01)},
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01)},
	{}			//
};*/



#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
static void btmtk_usb_early_suspend(void)
{
    printk("%s begin\n", __FUNCTION__);
    printk("%s : skip this state\n", __FUNCTION__);
    //btmtk_usb_set_state(BTMTK_USB_STATE_EARLY_SUSPEND);
    printk("%s end\n", __FUNCTION__);
}

static void btmtk_usb_late_resume(void)
{
	int err = 0;
	int retry_counter = 10;
    printk("%s begin\n", __FUNCTION__);
late_resume_again:
    mutex_lock(&btmtk_usb_state_mutex);
    if ( btmtk_usb_get_state() == BTMTK_USB_STATE_EARLY_SUSPEND ||
         btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING )
    {
        printk("%s invoked immediately after early suspend, ignore.\n", __FUNCTION__);
        btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_DISCONNECT ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_DISCONNECT ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_DISCONNECT )
    {
        printk("%s previous state is error disconnect(%d), ignore and set to disconnect state.\n", __FUNCTION__, btmtk_usb_get_state());
        btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_FW_DUMP ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP )
    {
        printk("%s previous state is error fw dump(%d), ignore and set to fw dump state.\n", __FUNCTION__, btmtk_usb_get_state());
        btmtk_usb_set_state(BTMTK_USB_STATE_FW_DUMP);
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_PROBE ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_PROBE ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_PROBE )
    {
        printk("%s previous state is error probe(%d), ignore and set to probe state.\n", __FUNCTION__, btmtk_usb_get_state());
        btmtk_usb_set_state(BTMTK_USB_STATE_PROBE);
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING )
    {
        printk("%s previous state is working(%d), ignore and return.\n", __FUNCTION__, btmtk_usb_get_state());
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }
    else if ( btmtk_usb_get_state() != BTMTK_USB_STATE_RESUME &&  btmtk_usb_get_state() != BTMTK_USB_STATE_SUSPEND)
    {
        printk("%s previous state is not resume(%d), ignore.\n", __FUNCTION__, btmtk_usb_get_state());
        printk("%s end\n", __func__);
        mutex_unlock(&btmtk_usb_state_mutex);
        return;
    }

    btmtk_usb_set_state(BTMTK_USB_STATE_LATE_RESUME);
    mutex_unlock(&btmtk_usb_state_mutex);

    clear_bit(BTUSB_BULK_RUNNING, &g_data->flags);
	clear_bit(BTUSB_INTR_RUNNING, &g_data->flags);
    btmtk_usb_stop_traffic(g_data);

	if (test_bit(BTUSB_INTR_RUNNING, &g_data->flags) || metaMode != 0) {
		printk("%s: BT USB INTR RUNNING\n", __func__);
		err = meta_usb_submit_intr_urb(g_data->hdev);
		if (err < 0) {
			printk("%s: error number:%d\n", __func__, err);
			clear_bit(BTUSB_INTR_RUNNING, &g_data->flags);

			if ( retry_counter > 0) {
            	retry_counter --;
                printk("%s wait 500ms and submit urb again.\n", __FUNCTION__);
                msleep(500);
                goto late_resume_again;
			} else {
    			goto late_resume_failed;
    	    }
		}
	}

	if (test_bit(BTUSB_BULK_RUNNING, &g_data->flags) || metaMode != 0) {
		printk("%s: BT USB BULK RUNNING\n", __func__);
		err = meta_usb_submit_bulk_in_urb(g_data->hdev);
		if (err < 0) {
			printk("%s: error number:%d\n", __func__, err);
			clear_bit(BTUSB_BULK_RUNNING, &g_data->flags);

			if ( retry_counter > 0 ) {
            	retry_counter --;
                printk("%s wait 500ms and submit urb again.\n", __FUNCTION__);
                msleep(500);
                goto late_resume_again;
			} else {
    			goto late_resume_failed;
    	    }
		}
	}

	if (test_bit(BTUSB_ISOC_RUNNING, &g_data->flags) || metaMode != 0) {
		if (btmtk_usb_submit_isoc_in_urb(g_data->hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_ISOC_RUNNING, &g_data->flags);
		else
			btmtk_usb_submit_isoc_in_urb(g_data->hdev, GFP_NOIO);
	}

    spin_lock_irq(&g_data->txlock);
	play_deferred(g_data);
	clear_bit(BTUSB_SUSPENDING, &g_data->flags);
	spin_unlock_irq(&g_data->txlock);
	schedule_work(&g_data->work);

    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
    mutex_unlock(&btmtk_usb_state_mutex);

	wake_up_interruptible(&inq);
	printk("%s end\n", __func__);
	return;

late_resume_failed:
	printk("%s failed \n", __func__);
	usb_scuttle_anchored_urbs(&g_data->deferred);

	spin_lock_irq(&g_data->txlock);
	clear_bit(BTUSB_SUSPENDING, &g_data->flags);
	spin_unlock_irq(&g_data->txlock);

    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
    mutex_unlock(&btmtk_usb_state_mutex);

	printk("%s end, skip send cmd to FW\n", __func__);
	return;
}
#endif


/* clayton: usb interface driver */
static struct usb_driver btmtk_usb_driver = {
	.name = "btmtk_usb",
	.probe = btmtk_usb_probe,
	.disconnect = btmtk_usb_disconnect,
#ifdef CONFIG_PM
	.suspend = btmtk_usb_suspend,
	.resume = btmtk_usb_resume,
#endif
	.id_table = btmtk_usb_table,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

static int btmtk_usb_send_data(struct hci_dev *hdev, const unsigned char *buffer, const unsigned int length)
{
	struct urb *urb;
	unsigned int pipe;
	int err;

	struct btmtk_usb_data *data = hci_get_drvdata(hdev);

	if (!data->bulk_tx_ep) {
		printk("%s: No bulk_tx_ep\n", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (!urb) {
		printk("%s: No memory\n", __func__);
		return -ENOMEM;
	}

	if (buffer[0] == 0x02) {

		while (data->meta_tx != 0) {
			udelay(500);
		}

		data->meta_tx = 1;

		//pr_debug("%s:send ACL Data\n", __func__);

		pipe = usb_sndbulkpipe(data->udev, data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, data->udev, pipe, (void *)buffer+1, length-1, btmtk_usb_tx_complete_meta, hdev);

		urb->transfer_dma = 0;
		urb->transfer_flags |=URB_NO_TRANSFER_DMA_MAP ;

		usb_anchor_urb(urb, &data->tx_anchor);

		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (err != 0) {
			printk("send ACL data fail, error:%d", err);
		}
		mdelay(1);
	} else if (buffer[0] == 0x03) {

		//pr_debug("%s:send SCO Data\n", __func__);

		if (!data->isoc_tx_ep || hdev->conn_hash.sco_num < 1)
			return -ENODEV;

		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndisocpipe(data->udev,
					data->isoc_tx_ep->bEndpointAddress);

		/* remove the SCO header:0x03 */
		usb_fill_int_urb(urb, data->udev, pipe,
				(void *)buffer+1, length-1, btmtk_usb_isoc_tx_complete_meta,
				hdev, data->isoc_tx_ep->bInterval);
		urb->transfer_dma = 0;

		urb->transfer_flags  = URB_ISO_ASAP;
		urb->transfer_flags |=URB_NO_TRANSFER_DMA_MAP ;

		__fill_isoc_descriptor(urb, length,
				le16_to_cpu(data->isoc_tx_ep->wMaxPacketSize));
		err = 0;

	} else {
		printk("%s:unknown data\n", __func__);
		err = 0;
	}

	if (err < 0) {
		printk("%s urb %p submission failed (%d)",
			hdev->name, urb, -err);

		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else {
		usb_mark_last_busy(data->udev);
	}

	usb_free_urb(urb);
	return err;

}


long btmtk_usb_fops_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	INT32 retval = 0;

	switch (cmd) {
	case IOCTL_FW_ASSERT:
		/* BT trigger fw assert for debug */
		printk("BT Set fw assert......, reason:%lu\n", arg);
		break;
	default:
		retval = -EFAULT;
		printk("BT_ioctl(): unknown cmd (%d)\n", cmd);
		break;
	}

	return retval;
}


unsigned int btmtk_usb_fops_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	if (metabuffer->read_p == metabuffer->write_p) {
		poll_wait(filp, &inq,  wait);

		/* empty let select sleep */
		if ((metabuffer->read_p != metabuffer->write_p) || need_reset_stack == 1) {
			mask |= POLLIN | POLLRDNORM;  /* readable */
		}
	} else {
		mask |= POLLIN | POLLRDNORM;  /* readable */
	}

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM; /* writable */

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING)
    {}
    else if ( btmtk_usb_get_state() == BTMTK_USB_STATE_FW_DUMP ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
              btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP ) {
        mask |= POLLIN | POLLRDNORM;  /* readable */
    }
    else{
        mask = 0;
    }
    mutex_unlock(&btmtk_usb_state_mutex);
#endif
	return mask;
}


ssize_t btmtk_usb_fops_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = 0;
	metaCount++;
	metaMode = 1;

	/* create interrupt in endpoint */
	if (metaCount) {
		if (!meta_hdev) {
			printk("btmtk:meta hdev isn't ready\n");
			metaCount = 0;
			return -EAGAIN;
		}
	}

	if (isUsbDisconnet) {
		printk("%s:btmtk: usb driver is disconnect now\n", __func__);
		return -EAGAIN;
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if (btmtk_usb_get_state() != BTMTK_USB_STATE_WORKING) {
		printk("%s: current is in suspend/resume (%d).\n", __func__, btmtk_usb_get_state());
		mutex_unlock(&btmtk_usb_state_mutex);
		return -EAGAIN;
	}
	mutex_unlock(&btmtk_usb_state_mutex);
#else
	if (need_reset_stack || skip_hci_cmd_when_stack_reset) {
        printk("%s: current is in suspend/resume, ignore.\n", __func__);
		return BT_STACK_RESET_CODE;
	}
#endif

	/* clayton: semaphore mechanism, the waited process will sleep */
	down(&wr_mtx);

	if (count > 0) {
		int copy_size = (count < BUFFER_SIZE) ? count : BUFFER_SIZE;
		if (copy_from_user(&o_buf[0], &buf[0], copy_size)) {
			retval = -EFAULT;
			printk("%s: copy data from user fail\n", __FUNCTION__);
			goto OUT;
		}

		/* command */
		if (o_buf[0] == 0x01) {
		    if ( copy_size == 9 &&
		         o_buf[0] == 0x01 &&
		         o_buf[1] == 0x6f &&
		         o_buf[2] == 0xfc &&
		         o_buf[3] == 0x05 &&
		         o_buf[4] == 0x01 &&
		         o_buf[5] == 0x02 &&
		         o_buf[6] == 0x01 &&
		         o_buf[7] == 0x00 &&
		         o_buf[8] == 0x08 )
            {
                printk("%s: Donge FW Assert Triggered by BT Stack!\n", __FUNCTION__);
                btmtk_usb_hci_dump_print_to_log();
            }
            else if ( copy_size == 4 &&
                      o_buf[0] == 0x01 &&
                      o_buf[1] == 0x03 &&
                      o_buf[2] == 0x0c &&
                      o_buf[3] == 0x00 )
            {
                printk("%s: got command : 0x03 0c 00 (HCI_RESET)\n", __FUNCTION__);
            }
            else if ( copy_size == 4 &&
                      o_buf[0] == 0x01 &&
                      o_buf[1] == 0x01 &&
                      o_buf[2] == 0x10 &&
                      o_buf[3] == 0x00 )
            {
                printk("%s: got command : 0x01 10 00 (READ_LOCAL_VERSION)\n", __FUNCTION__);
            }

#if SUPPORT_HCI_DUMP
            btmtk_usb_save_hci_cmd(copy_size, &o_buf[0]);
#endif
			retval = btmtk_usb_meta_send_data(&o_buf[0], copy_size);
		} else if (o_buf[0] == 0x02) {
			retval = btmtk_usb_send_data(meta_hdev, &o_buf[0], copy_size);
		} else if (o_buf[0] == 0x03) {
			retval = btmtk_usb_send_data(meta_hdev, &o_buf[0], copy_size);
		} else {
			printk("%s:this is unknown bt data:0x%02x\n", __FUNCTION__, o_buf[0]);
		}
	} else {
		retval = -EFAULT;
		printk("%s:target packet length:%zu is not allowed, retval = %d.\n", __FUNCTION__, count, retval);
	}

 OUT:
	up(&wr_mtx);
	return (retval);
}

ssize_t btmtk_usb_fops_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = 0;
	int copyLen = 0;
	unsigned short tailLen = 0;
	UINT8 *buffer = i_buf;

	down(&rd_mtx);

	if (count > BUFFER_SIZE) {
		count = BUFFER_SIZE;
		printk("read size is bigger than 1024\n");
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
	if ( btmtk_usb_get_state() == BTMTK_USB_STATE_FW_DUMP ||
	     btmtk_usb_get_state() == BTMTK_USB_STATE_SUSPEND_ERROR_FW_DUMP ||
	     btmtk_usb_get_state() == BTMTK_USB_STATE_RESUME_ERROR_FW_DUMP )
    {
        mutex_unlock(&btmtk_usb_state_mutex);
    }
    else if (btmtk_usb_get_state() != BTMTK_USB_STATE_WORKING) {
		printk("%s: current is in suspend/resume (%d).\n", __func__, btmtk_usb_get_state());
		mutex_unlock(&btmtk_usb_state_mutex);
		return -EAGAIN;
	}
    else
    {
        mutex_unlock(&btmtk_usb_state_mutex);
#endif
    	if (need_reset_stack) {
    		pr_warn("%s:Reset BT stack\n", __func__);
    		need_reset_stack = 0;
            skip_hci_cmd_when_stack_reset = 1;
    		if (copy_to_user(buf, reset_event, sizeof(reset_event))) {
    			pr_warn("%s: Copy reset event to stack fail\n", __func__);
    			need_reset_stack = 1;
    			up(&rd_mtx);
    			return -EFAULT;
    		}
    		up(&rd_mtx);
    		return sizeof(reset_event);
    	}
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    }
#endif


	if (isUsbDisconnet) {
		printk("%s:btmtk: usb driver is disconnect now\n", __func__);
		up(&rd_mtx);
		return -EIO;
	}

	lock_unsleepable_lock(&(metabuffer->spin_lock));

	/* means the buffer is empty */
	while (metabuffer->read_p == metabuffer->write_p) {

		/*  unlock the buffer to let other process write data to buffer */
		unlock_unsleepable_lock(&(metabuffer->spin_lock));

		/*If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (filp->f_flags & O_NONBLOCK) {
			//pr_info("Non-blocking btmtk_usb_fops_read()\n");
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_wq, metabuffer->read_p != metabuffer->write_p);
		lock_unsleepable_lock(&(metabuffer->spin_lock));
	}

	while (metabuffer->read_p != metabuffer->write_p) {
		if (metabuffer->write_p > metabuffer->read_p) {
			copyLen = metabuffer->write_p - metabuffer->read_p;
			if (copyLen > count) {
				copyLen = count;
			}
			osal_memcpy(i_buf, metabuffer->buffer + metabuffer->read_p, copyLen);
			metabuffer->read_p += copyLen;
			break;
		} else {
			tailLen = META_BUFFER_SIZE - metabuffer->read_p;
			if (tailLen > count) {	/* exclude equal case to skip wrap check */
				copyLen = count;
				osal_memcpy(i_buf, metabuffer->buffer + metabuffer->read_p, copyLen);
				metabuffer->read_p += copyLen;
			} else {
				/* part 1: copy tailLen */
				osal_memcpy(i_buf, metabuffer->buffer + metabuffer->read_p, tailLen);

				buffer += tailLen;	/* update buffer offset */

				/* part 2: check if head length is enough */
				copyLen = count - tailLen;

				/* clayton: if write_p < copyLen: means we can copy all data until write_p; else: we can only copy data for copyLen */
				copyLen =
				    (metabuffer->write_p < copyLen) ? metabuffer->write_p : copyLen;

				/* clayton: if copylen not 0, copy data to buffer */
				if (copyLen) {
					osal_memcpy(buffer, metabuffer->buffer + 0, copyLen);
				}
				/* Update read_p final position */
				metabuffer->read_p = copyLen;

				/* update return length: head + tail */
				copyLen += tailLen;
			}
			break;
		}
	}

	unlock_unsleepable_lock(&(metabuffer->spin_lock));

	if (copy_to_user(buf, i_buf, copyLen)) {
		printk("copy to user fail\n");
		copyLen = -EFAULT;
		goto OUT;
	}
 OUT:
	up(&rd_mtx);
	return (copyLen);
}

static int btmtk_usb_fops_open(struct inode *inode, struct file *file)
{
	int ret = 0;

    mutex_lock(&btmtk_usb_state_mutex);
	if (btmtk_usb_get_state() == BTMTK_USB_STATE_INIT) {
	    mutex_unlock(&btmtk_usb_state_mutex);
		return -EAGAIN;
	}
	mutex_unlock(&btmtk_usb_state_mutex);

	printk("%s:Mediatek Bluetooth USB driver ver %s\n",__func__, VERSION);

    if ( is_fops_close_ongoing ){
		printk("%s: fops close is on-going !", __func__);
		return -ENODEV;
    }

	if (meta_hdev == NULL){
		printk("%s: meta_hdev is NULL!!", __func__);
		return -ENODEV;
	}

	printk("%s: major %d minor %d (pid %d), probe counter:%d\n", __func__,
	       imajor(inode), iminor(inode), current->pid, probe_counter);
	if (current->pid == 1)
		return 0;

	if (isbtready == 0) {
		printk("%s:bluetooth driver is not ready\n", __func__);
		return -ENODEV;
	}

	if (isUsbDisconnet) {
		printk("%s:btmtk: usb driver is disconnect now\n", __func__);
		return -EIO;
	}

	if (is_assert) {
		printk("%s: current is doing coredump\n", __func__);
		return -EIO;
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if (btmtk_usb_get_state() != BTMTK_USB_STATE_WORKING) {
		printk("%s: current is in suspend/resume (%d).\n", __func__, btmtk_usb_get_state());
        mutex_unlock(&btmtk_usb_state_mutex);
//		return -EIO;
        return 0;
	}
	mutex_unlock(&btmtk_usb_state_mutex);
#endif

	metaMode = 1;
    skip_hci_cmd_when_stack_reset = 0;

	/* init meta buffer */
	spin_lock_init(&(metabuffer->spin_lock.lock));

	sema_init(&wr_mtx, 1);
	sema_init(&rd_mtx, 1);

	/* init wait queue */
	init_waitqueue_head(&(inq));

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
	clear_bit(BTUSB_BULK_RUNNING, &g_data->flags);
	clear_bit(BTUSB_INTR_RUNNING, &g_data->flags);
	btmtk_usb_stop_traffic(g_data);
	need_reset_stack = 0;

	/* BT_RESET */
	if ( btmtk_usb_send_hci_reset_cmd(g_data->udev) < 0 )
	{
	    meta_usb_submit_bulk_in_urb(meta_hdev);
	    btmtk_usb_send_assert_cmd(g_data->udev);
	    return -EIO;
	}

	/* for WoBLE/WoW low power */
	if ( btmtk_usb_send_hci_set_ce_cmd(g_data->udev) < 0 )
	{
	    meta_usb_submit_bulk_in_urb(meta_hdev);
	    btmtk_usb_send_assert_cmd(g_data->udev);
	    return -EIO;
	}

#ifdef SET_TX_POWER_AFTER_ROM_PATCH
	mdelay(20);
	btmtk_usb_send_set_tx_power_cmd(g_data->udev);//default set to 7
#endif

	lock_unsleepable_lock(&(metabuffer->spin_lock));
	metabuffer->read_p = 0;
	metabuffer->write_p = 0;
	unlock_unsleepable_lock(&(metabuffer->spin_lock));
#endif

	ret = btmtk_usb_send_radio_on_cmd(g_data->udev);

	printk("enable interrupt and bulk in urb\n");
	meta_usb_submit_intr_urb(meta_hdev);
	meta_usb_submit_bulk_in_urb(meta_hdev);
	printk("%s:OK\n", __func__);

    if(ret < 0){
		btmtk_usb_send_assert_cmd(g_data->udev);
		mdelay(5000);
	}

#if SUPPORT_HCI_DUMP
    btmtk_usb_hci_dmp_init();
#endif

	return 0;
}

static int btmtk_usb_fops_close(struct inode *inode, struct file *file)
{
	int ret = 0;
	is_fops_close_ongoing = 1;
	printk("%s: major %d minor %d (pid %d), probe:%d\n", __func__,
	       imajor(inode), iminor(inode), current->pid, probe_counter);

	if (isUsbDisconnet) {
		printk("%s:btmtk: usb driver is disconnect now\n", __func__);
		is_fops_close_ongoing = 0;
		return -EIO;
	}

	if (is_assert) {
		printk("%s: current is doing coredump\n", __func__);
		is_fops_close_ongoing = 0;
		return -EIO;
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    if (btmtk_usb_get_state() != BTMTK_USB_STATE_WORKING) {
		printk("%s: current is in suspend/resume (%d).\n", __func__, btmtk_usb_get_state());
		is_fops_close_ongoing = 0;
        mutex_unlock(&btmtk_usb_state_mutex);
//		return -EIO;
		return 0;
	}
	mutex_unlock(&btmtk_usb_state_mutex);
#endif
	btmtk_usb_stop_traffic(g_data);
	btmtk_usb_send_hci_reset_cmd(g_data->udev);
	ret = btmtk_usb_send_radio_off_cmd(g_data->udev);


	lock_unsleepable_lock(&(metabuffer->spin_lock));
	metabuffer->read_p = 0;
	metabuffer->write_p = 0;
	unlock_unsleepable_lock(&(metabuffer->spin_lock));

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
#else
	if(ret < 0){
	    printk("%s: Resume Rx for core dump data", __func__);
		meta_usb_submit_bulk_in_urb(meta_hdev);
		meta_usb_submit_intr_urb(meta_hdev);
		btmtk_usb_send_assert_cmd(g_data->udev);
		mdelay(5000);
	}
#endif
	metaMode = 0; /* means BT is close */
	is_fops_close_ongoing = 0;

	if (current->pid == 1)
		return 0;

	printk("%s:OK\n", __func__);
	return 0;
}

struct file_operations BT_fops = {
	.open = btmtk_usb_fops_open,
	.release = btmtk_usb_fops_close,
	.read = btmtk_usb_fops_read,
	.write = btmtk_usb_fops_write,
	.poll = btmtk_usb_fops_poll,
	.unlocked_ioctl = btmtk_usb_fops_unlocked_ioctl
};

static int btmtk_usb_BT_init(void)
{
	dev_t devID = MKDEV(BT_major, 0);
	int ret = 0;
	int cdevErr = 0;
	int major = 0;
	printk("%s\n", __FUNCTION__);

	ret = alloc_chrdev_region(&devID, 0, 1, BT_DRIVER_NAME);
	if (ret) {
		printk("fail to allocate chrdev\n");
		return ret;
	}

	BT_major = major = MAJOR(devID);
	printk("major number:%d", BT_major);

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&BT_cdev, devID, BT_devs);
	if (cdevErr)
		goto error;

	printk("%s driver(major %d) installed.\n", BT_DRIVER_NAME, BT_major);

	pBTClass = class_create(THIS_MODULE, BT_DRIVER_NAME);
	if (IS_ERR(pBTClass)) {
		printk("class create fail, error code(%ld)\n", PTR_ERR(pBTClass));
		goto err1;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, BT_NODE);
	if (IS_ERR(pBTDev)) {
		printk("device create fail, error code(%ld)\n", PTR_ERR(pBTDev));
		goto err2;
	}
	/* init wait queue */
	init_waitqueue_head(&(inq));

	if(NULL == i_buf)
	 i_buf = kmalloc(BUFFER_SIZE, GFP_ATOMIC);
	if(NULL == o_buf)
	 o_buf = kmalloc(BUFFER_SIZE, GFP_ATOMIC);
	if(NULL == metabuffer)
		metabuffer = kmalloc(sizeof(ring_buffer_struct), GFP_ATOMIC);

	if(i_buf == NULL || o_buf == NULL || NULL == metabuffer){//metabuffer alloc in btmtk_usb_init
		printk("%s,malloc memory fail\n",__func__);
		if(NULL == metabuffer)
			printk("%s,NULL == metabuffer \n",__func__);

		return -ENOMEM;
	}

#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_INIT);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif

	return 0;

 err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

 err1:

 error:
	if (cdevErr == 0)
		cdev_del(&BT_cdev);

	if (ret == 0)
		unregister_chrdev_region(devID, BT_devs);

	return -1;
}

static void btmtk_usb_BT_exit(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	printk("%s\n", __FUNCTION__);

	if(i_buf != NULL){
		kfree(i_buf);
		i_buf = NULL;
	}
	if(o_buf != NULL){
		kfree(o_buf);
		o_buf = NULL;
	}

	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}

	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	if(metabuffer)
		kfree(metabuffer);

    if (fw_dump_ptr) {
        kfree(fw_dump_ptr);
        fw_dump_ptr = NULL;
    }

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, 1);
#if SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION
    mutex_lock(&btmtk_usb_state_mutex);
    btmtk_usb_set_state(BTMTK_USB_STATE_UNKNOWN);
    mutex_unlock(&btmtk_usb_state_mutex);
#endif
	printk("%s driver removed.\n", BT_DRIVER_NAME);
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
/* module_usb_driver(btmtk_usb_driver); */
#if BT_SEND_HCI_CMD_BEFORE_STANDBY
void RegisterPdwncCallback(int (*f)(void));
#endif
static int __init btmtk_usb_init(void)
{
	printk("%s: btmtk usb driver ver %s", __FUNCTION__, VERSION);
	btmtk_usb_BT_init();
#if BT_SEND_HCI_CMD_BEFORE_STANDBY
	RegisterPdwncCallback(&btmtk_usb_standby);
#endif
	return usb_register(&btmtk_usb_driver);
}

static void __exit btmtk_usb_exit(void)
{
	printk("%s: btmtk usb driver ver %s", __FUNCTION__, VERSION);
	usb_deregister(&btmtk_usb_driver);
	btmtk_usb_BT_exit();
}
module_init(btmtk_usb_init);
module_exit(btmtk_usb_exit);
#else
static int __init btmtk_usb_init(void)
{
	printk("%s: btmtk usb driver ver %s", __FUNCTION__, VERSION);
	btmtk_usb_BT_init();

	return usb_register(&btmtk_usb_driver);
}

static void __exit btmtk_usb_exit(void)
{
	usb_deregister(&btmtk_usb_driver);
	btmtk_usb_BT_exit();
}
module_init(btmtk_usb_init);
module_exit(btmtk_usb_exit);
#endif



MODULE_DESCRIPTION("Mediatek Bluetooth USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");