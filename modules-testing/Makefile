obj-m :=Hello.o  #产生hello模块的目标文件
CURRENT_PATH:=$(shell pwd)
LINUX_KERNEL_PATH:=/home/wrsadmin/RedHat-Linux8/linux-4.18.20
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules    #编译模块
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean      #清理
