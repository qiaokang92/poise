# Makefile for compile kernel for Andriod
KERNEL_DIR := /home/jeremy/Android/kernel/goldfish
CROSS_COMPILE := /home/jeremy/Android/aosp/Android-7.1.0_r1/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android-
EXTRA_CFLAGS  := -g -Wall
ARCH := x86_64
PWD := $(shell pwd)
obj-m := poise.o
poise-objs := mon_poise.o mon_rs.o mon_fs.o mon_nf.o mon_sk.o mon_lsm.o totp.o sha1.o
# poise-objs := helloworld.o

.PHONY: modules package clean
default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
