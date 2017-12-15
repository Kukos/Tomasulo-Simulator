CC := gcc

CCWARNINGS := 	-Wall -Wextra -pedantic -Wcast-align \
				-Winit-self -Wlogical-op -Wmissing-include-dirs \
				-Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wundef  \
				-Wwrite-strings -Wpointer-arith -Wmissing-declarations \
				-Wuninitialized -Wold-style-definition -Wstrict-prototypes \
				-Wmissing-prototypes -Wswitch-default -Wbad-function-cast \
				-Wnested-externs -Wconversion -Wunreachable-code

CFLAGS := -std=gnu99 $(CCWARNINGS) -O3

PROJECT_DIR := $(shell pwd)

IDIR := $(PROJECT_DIR)/include
SDIR := $(PROJECT_DIR)/src
ODIR := $(PROJECT_DIR)/obj
EDIR := $(PROJECT_DIR)/external
LDIR := $(EDIR)/libs
EIDIR := $(EDIR)/include
ESDIR := $(EDIR)/libs

SRCS := $(wildcard $(SDIR)/*.c)
SRCS += $(ESDIR)/log.c
OBJS := $(SRCS:%.c=%.o)
DEPS := $(wildcard $(IDIR)/*.h)
DEPS += $(wildcard $(EIDIR)/*.h)

SUBDIR := $(PROJECT_DIR)/submodules

LIBS := -lfilebuffer -ldarray -lgetch

EXEC := tomasulo.out

ifeq ("$(origin V)", "command line")
  VERBOSE = $(V)
endif

ifndef VERBOSE
  VERBOSE = 0
endif

ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

define print_info
	$(if $(Q), @echo "$(1)")
endef

define print_make
	$(if $(Q), @echo "[MAKE]    $(1)")
endef

define print_cc
	$(if $(Q), @echo "[CC]      $(1)")
endef

define print_bin
	$(if $(Q), @echo "[BIN]     $(1)")
endef

all: $(EXEC)

libs:
	$(Q)if [ ! -d $(LDIR) ]; then \
	cd $(SUBDIR)/MyLibs/scripts && \
	./install_libs.sh $(EDIR) && \
	cd $(PROJECT_DIR) ;fi

%.o: %.c
	$(call print_cc, $<)
	$(Q)$(CC) $(CFLAGS) -I$(IDIR) -I$(EIDIR) -c $< -o $@

$(EXEC): libs $(OBJS)
	$(call print_bin, $@)
	$(Q)$(CC) $(CFLAGS) -L$(LDIR) -I$(IDIR) -I$(EIDIR) $(OBJS) $(LIBS) -o $@

clean:
	$(call print_info,Cleaning)
	$(Q)rm -f $(OBJS)
	$(Q)rm -rf $(EDIR)/*
	$(Q)rm -f $(EXEC)
	$(Q)cd $(SUBDIR)/MyLibs && $(MAKE) clean --no-print-directory
