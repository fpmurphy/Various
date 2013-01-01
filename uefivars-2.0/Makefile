#
# Makefile   UEFIVARS
#

default: all

RELEASE_DATE   := "1-Jan-2012"
RELEASE_MAJOR  := 2
RELEASE_MINOR  := 0
RELEASE_NAME   := uefivars
RELEASE_STRING := $(RELEASE_NAME)-$(RELEASE_MAJOR).$(RELEASE_MINOR)

MODULES        := src
BINDIR         := /usr/sbin
EXTRA_CFLAGS   :=

CFLAGS = $(EXTRA_CFLAGS) -DUEFIVARS_VERSION=\"$(RELEASE_MAJOR).$(RELEASE_MINOR)\"

#------------------------------------------------------------------
# Generic Makefile stuff is below. You should not have to modify it
#------------------------------------------------------------------

#Included makefiles will add their deps for each stage in these vars:
INSTALLDEPS :=
CLEANDEPS :=
ALLDEPS :=
CLEANLIST :=

#Define the top-level build directory
BUILDDIR := $(shell pwd)

#Include make rules from each submodule (subdirectory)
include $(patsubst %,%/module.mk,$(MODULES))

.PHONY: all clean install_list install install_link post_install tarball echotree default

all:  $(ALLDEPS) 
clean: clean_list $(CLEANDEPS) 

clean_list:
	rm -f $(CLEANLIST)

install_list: echotree $(INSTALLDEPS) 

install: all 
	@make install_list | tools/install.pl copy

install_link: all
	@make install_list | tools/install.pl link

post_install: 

tarball: clean
	-rm $(RELEASE_NAME)*.tar.gz
	cp -a ../$(RELEASE_NAME) ../$(RELEASE_STRING)
	find ../$(RELEASE_STRING) -name CVS -type d -depth -exec rm -rf \{\} \;
	sync; sync; sync;
	cd ..; tar cvzf $(RELEASE_STRING).tar.gz --exclude=.git --exclude=\*~ $(RELEASE_STRING)
	mv ../$(RELEASE_STRING).tar.gz .
	rm -rf ../$(RELEASE_STRING)

doc_TARGETS += COPYING README INSTALL

echotree:
	@# making directory tree 
	@#RPM FORMAT:
	@# %defattr(-, user, group) 
	@# %attr(4755,user,group)  filename
	@# filename
