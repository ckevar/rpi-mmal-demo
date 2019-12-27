# Simple makefile for rpi-openmax-demos.

PROGRAMS = buffer_demo main video_record
CC       = gcc
CFLAGS 	 = -I/opt/vc/include
#CFLAGS   += -DSTANDALONE
#CFLAGS   += -D__STDC_CONSTANT_MACROS
#CFLAGS   += -D__STDC_LIMIT_MACROS
#CFLAGS   += -DTARGET_POSIX -D_LINUX
#CFLAGS   += -DPIC -D_REENTRANT
#CFLAGS   += -D_LARGEFILE64_SOURCE
#CFLAGS   += -D_FILE_OFFSET_BITS=64
#CFLAGS   += -U_FORTIFY_SOURCE 
#CFLAGS   += -DHAVE_LIBOPENMAX=2 
#CFLAGS   += -DOMX 
#CFLAGS   += -DOMX_SKIP64BIT 
#CFLAGS   += -ftree-vectorize 
#CFLAGS   += -pipe 
#CFLAGS   += -DUSE_EXTERNAL_OMX 
#CFLAGS   += -DHAVE_LIBBCM_HOST 
#CFLAGS   += -DUSE_EXTERNAL_LIBBCM_HOST 
#CFLAGS   += -DUSE_VCHIQ_ARM 
#CFLAGS   += -fPIC
#CFLAGS   += -O2
#CFLAGS   += -g
#CFLAGS   += -I/opt/vc/include/interface/vcos/pthreads 
#CFLAGS   += -I/opt/vc/include/interface/vmcs_host/linux
CFLAGS   += -Wall
#CFLAGS 	 += -Werror
LDFLAGS  = -L/opt/vc/lib
LDFLAGS  += -lbcm_host -lvcos -pthread
LDFLAGS  += -lmmal -lmmal_core -lmmal_util
all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean
