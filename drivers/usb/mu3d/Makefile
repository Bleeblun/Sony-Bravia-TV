ifeq ($(USB_ADB_PORT), 0)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=0
endif
ifeq ($(USB_ADB_PORT), 1)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=1
endif
ifeq ($(USB_ADB_PORT), 2)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=2
endif
ifeq ($(USB_ADB_PORT), 3)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=3
endif
ifeq ($(CONFIG_USB_MU3D_O0),y)
	EXTRA_CFLAGS += -O0
endif


obj-$(CONFIG_USB_MU3D_UDC)	+= udc-core.o
obj-$(CONFIG_USB_MU3D)		+= mu3d_hal/
obj-$(CONFIG_USB_MU3D_DRV)	+= mu3d_drv/
obj-$(CONFIG_USB_MU3D_TEST_DRV)	+= mu3d_test_drv/
