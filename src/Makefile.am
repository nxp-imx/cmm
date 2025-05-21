AM_CFLAGS = -O2 -g -Wall -Werror
ACLOCAL_AMFLAGS = -I m4

CMM_VERSION_FILE = version.h
ROOT_DIR = ../
GIT_DIR = $(ROOT_DIR)/.git

BUILT_SOURCES = version

version:
	if [ -d $(GIT_DIR) ] ; then \
		echo retreiving cmm version from git ; \
		cd $(ROOT_DIR)/ ; \
		git describe --always --tags --dirty ; \
		CMM_GIT_VERSION=`git describe --dirty --always --tags` ; \
		cd -; \
		printf "/*Auto-generated file. Do not edit !*/\n#ifndef VERSION_H\n#define VERSION_H\n\n#define CMM_VERSION \"$${CMM_GIT_VERSION}\"\n\n#endif /* VERSION_H */\n" > $(CMM_VERSION_FILE) ; \
	else \
		CMM_GIT_VERSION=${CMM}; \
		printf "/*Auto-generated file. Do not edit !*/\n#ifndef VERSION_H\n#define VERSION_H\n\n#define CMM_VERSION \"$${CMM_GIT_VERSION}\"\n\n#endif /* VERSION_H */\n" > $(CMM_VERSION_FILE) ; \
	fi


bin_PROGRAMS = cmm libcmm_sample

cmm_SOURCES = 	neighbor_resolution.c  \
		module_expt.c  \
		cmm.c  \
		conntrack.c  \
		ffcontrol.c  \
		forward_engine.c  \
		route_cache.c \
		pppoe.c \
		ffbridge.c \
		client_daemon.c  \
		rtnl.c		\
		module_rx.c  \
		module_tx.c  \
		module_qm.c  \
		module_prf.c \
		module_vlan.c \
		module_macvlan.c \
		module_mcast.c \
		module_mc6.c \
		module_mc4.c \
		keytrack.c \
		timeout.c \
		module_tunnel.c \
		module_relay.c \
		itf.c \
		module_stat.c \
		module_route.c \
		module_socket.c \
		module_rtp.c \
		module_natpt.c \
		module_wifi.c \
		alt_conf.c \
		voicebuf.c \
		module_icc.c \
		third_part.c \
		cmm_asym_ff.c \
		module_l2tp.c \
		module_lro.c \
		module_ipsec.c \
		module_ipr.c

if CPAL_BPF
CPAL_LIB = -lbpf
AM_CFLAGS += -DCPAL_BPF=1
cmm_SOURCES += cpal_bpf.c
else
CPAL_LIB = -lfci
cmm_SOURCES += cpal_fci.c
endif
cmm_LDADD = -lpthread -lcli libcmm.la  @CMM_LIBS@ $(CPAL_LIB)
cmm_DEPENDENCIES = libcmm.la

libcmm_sample_SOURCES = libcmm_sample.c
libcmm_sample_LDADD = libcmm.la
libcmm_sample_DEPENDENCIES = libcmm.la


lib_LTLIBRARIES = libcmm.la
libcmm_la_SOURCES = libcmm.c
