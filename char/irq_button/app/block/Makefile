objs := test.c
TARGET := test

CC      = arm-linux-gcc
LD      = arm-linux-ld
AR      = arm-linux-ar
OBJCOPY = arm-linux-objcopy
OBJDUMP = arm-linux-objdump

$(TARGET) : $(objs)     
	${CC} -static -o $@ $<




