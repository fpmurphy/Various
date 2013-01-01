INSTALLDEPS += bindir_LIST

MODULES := src/uefivars src/lib src/include
include $(patsubst %,%/module.mk,$(MODULES))

bindir_LIST:    
	@if [ ! -z "$(bindir_TARGETS)" ]; then \
	  echo "R-M-: %attr(0755,root,root) $(BINDIR)" ;\
	  for file in $(bindir_TARGETS) ;\
	  do                                              \
	    echo "-C--: $(BUILDDIR)/$$file $(BINDIR)/"                ;\
	    echo "R---: %attr(0755,root,root) $(BINDIR)/`basename $$file`" ;\
	  done ;\
	  echo  ;\
	fi
