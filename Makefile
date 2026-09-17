# grgsm_exe - la couche 1 gr-gsm du Calypso, hors QEMU.
# Les sources viennent de /opt/GSM/qosmo et ne sont PAS recopiees ici.
QOSMO   ?= /opt/GSM/qosmo
CAL     := $(QOSMO)/hw/arm/calypso
L1G     := $(CAL)/l1-grgsm
HORS    := $(QOSMO)/contrib/hors-qemu

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wno-unused-function -Wno-unused-variable \
           -Wno-unused-but-set-variable -Wno-sign-compare
CPPFLAGS := -D_GNU_SOURCE -I$(HORS)/doublures -I$(L1G) -I$(CAL) -I$(QOSMO)/include -I$(QOSMO)
LDLIBS  := -lpthread -lm -lrt

SRC := src/main.c $(HORS)/cales-qemu.c \
       $(L1G)/calypso_l1_grgsm.c \
       $(CAL)/calypso_trf6151.c

all: grgsm_exe

grgsm_exe: $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f grgsm_exe

.PHONY: all clean
