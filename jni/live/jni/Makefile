##### Change the following for your environment:
#-------------------------------------------------------------------------------------------------
#
# 	Create BY zhang zhiwei 2014.07.03
# 	sunfrank2012@gmail.com
#
#-------------------------------------------------------------------------------------------------
# NDK ROOT
ANDROID_NDK_ROOT=/home/zhongyuxin/android-ndk-r13b
# For sysroot | arm-linux-androideabi-gcc can't find cygwin path
NDK_ROOT=/home/zhongyuxin/android-ndk-r13b
# Fro x86_64
PREBUILT=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
PLATFORM=android-16

CROSS_COMPILE=		$(PREBUILT)/bin/arm-linux-androideabi-
COMPILE_OPTS =		$(INCLUDES) -fPIC -DANDROID -std=c99 -mfpu=neon -mfloat-abi=softfp -I. -O2 -DXLOCALE_NOT_USED -DANDROID -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DNULL=0 --sysroot=${NDK_ROOT}/platforms/${PLATFORM}/arch-arm/  -I${ANDROID_NDK_ROOT}/platforms/${PLATFORM}/arch-arm/usr/include
C =			c
C_COMPILER =		$(CROSS_COMPILE)gcc
C_FLAGS =		$(COMPILE_OPTS)
CPP =			cpp
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1 -fexceptions
OBJ =			o
LINK =			$(CROSS_COMPILE)g++ -o
LINK_OPTS = 	
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		$(CROSS_COMPILE)ar cr 
LIBRARY_LINK_OPTS =	$(LINK_OPTS)
LIB_SUFFIX =			a
LIBS_FOR_CONSOLE_APPLICATION =-lc -lm -lz -L./ -lsupc++	-fexceptions -L${ANDROID_NDK_ROOT}/platforms/${PLATFORM}/arch-arm/usr/lib -lstdc++ 
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change

LIVEMEDIA_DIR = liveMedia
GROUPSOCK_DIR = groupsock
USAGE_ENVIRONMENT_DIR = UsageEnvironment
BASIC_USAGE_ENVIRONMENT_DIR = BasicUsageEnvironment

TESTPROGS_DIR = testProgs

MEDIA_SERVER_DIR = mediaServer

PROXY_SERVER_DIR = proxyServer

all:
	cd $(LIVEMEDIA_DIR) ; $(MAKE)
	cd $(GROUPSOCK_DIR) ; $(MAKE)
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(TESTPROGS_DIR) ; $(MAKE)
	cd $(MEDIA_SERVER_DIR) ; $(MAKE)
	cd $(PROXY_SERVER_DIR) ; $(MAKE)
	@echo
	@echo "For more information about this source code (including your obligations under the LGPL), please see our FAQ at http://live555.com/liveMedia/faq.html"

install:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) install
	cd $(GROUPSOCK_DIR) ; $(MAKE) install
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	cd $(TESTPROGS_DIR) ; $(MAKE) install
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) install
	cd $(PROXY_SERVER_DIR) ; $(MAKE) install

clean:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) clean
	cd $(GROUPSOCK_DIR) ; $(MAKE) clean
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(TESTPROGS_DIR) ; $(MAKE) clean
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) clean
	cd $(PROXY_SERVER_DIR) ; $(MAKE) clean

distclean: clean
	-rm -f $(LIVEMEDIA_DIR)/Makefile $(GROUPSOCK_DIR)/Makefile \
	  $(USAGE_ENVIRONMENT_DIR)/Makefile $(BASIC_USAGE_ENVIRONMENT_DIR)/Makefile \
	  $(TESTPROGS_DIR)/Makefile $(MEDIA_SERVER_DIR)/Makefile \
	  $(PROXY_SERVER_DIR)/Makefile Makefile
