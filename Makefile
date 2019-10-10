.PHONY: doc
ifeq ($(ARCH),)
$(error Target architecture is not specified: $${ARCH})
endif

ifeq ($(ARCH),amd64)
CFLAGS+=-DARCH_AMD64
endif

include config

export O?=$(abspath build)

# Include base system
include sys/conf.mk
# Arch details
include sys/$(ARCH)/conf.mk

all: mkdirs config $(TARGETS)

clean:
	@rm -rf $(O)

mkdirs:
	@mkdir -p $(O) $(DIRS)
