out=$(PROGNAME)
sources=$(PROGNAME).c vendor/mpc/mpc.c polish.c lval.c operator.c vendor/mini-gmp/mini-gmp.c
headers=vendor/mpc/mpc.h polish.h lval.h operator.h vendor/mini-gmp/mini-gmp.h
objects=$(sources:%.c=%.o)

version_file:=version.mk
build_file:=buildnumber.mk
test_file:=test.mk
version_header:=version.h

CC=gcc
SHELL:=/bin/bash
DEBUG?=-ggdb3 -O0
CFLAGS:=-Wall -std=c99 $(DEBUG)
LDFLAGS:=-Wall -lreadline -lm
VGFLAGS?=\
	--quiet --leak-check=full --show-leak-kinds=all \
	--track-origins=yes --error-exitcode=1 --error-limit=no \
	--suppressions=./valgrind-libraryleaks.supp

# Define PROGNAME, VERSION, CODENAME.
include $(version_file)
# Define BUILD.
include $(build_file)

all: build

build: $(out)

clean::
	rm -f *.o *.d $(objects) $(version_header) tags $(out)

# test target.
include $(test_file)

leakcheck: $(out)
	valgrind $(VGFLAGS) ./$^

# Build executable.
$(out): $(objects)

# Generate C source files dependancies.
%.d: %.c $(version_header)
	@set -e; rm -f $@; \
		$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

# Include dependancies makefiles.
include $(sources:%.c=%.d)

# Generate tags file.
tags:
	ctags -R .

# Increment build number & rewrite $(build_file).
$(build_file): $(sources) $(headers)
# Update $(BUILDNUMBER) value in this Makefile.
	$(eval BUILDNUMBER:=$(shell echo $$(( $(BUILDNUMBER)+1 ))))
# Rewrite $(build_file).
	@if ! test -f $@; then touch $@; fi
	@echo "# Automatically incremented." > $@
	@echo "BUILDNUMBER="$(BUILDNUMBER) >> $@

# Rewrite $(version_header) file if required.
$(version_header): $(version_file) $(build_file)
	@if ! test -f $@; then touch $@; fi
	@echo -e "/* Generated by make, do not modify. */\n" > $@
	@echo -e "#ifndef _H_VERSION_" >> $@
	@echo -e "#define _H_VERSION_\n" >> $@
	@echo -e "#define PROGNAME\t\""$(PROGNAME)"\"" >> $@
	@echo -e "#define VERSION\t\t\""$(VERSION)"\"" >> $@
	@echo -e "#define CODENAME\t\""$(CODENAME)"\"" >> $@
	@echo -e "#define BUILD\t\t"$(BUILDNUMBER) >> $@
	@echo -e "\n#endif" >> $@

# List of all special targets (always out-of-date).
.PHONY: all build clean tags
