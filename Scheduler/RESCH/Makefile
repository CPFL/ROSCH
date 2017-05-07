obj = core lib

.PHONY: all
all: $(obj)

$(obj)::
	@$(MAKE) -C $@

.PHONY: clean
clean:
	@( for d in $(obj) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d clean ; fi ; \
        done ) ;
	rm -fr ./include/{resch-config.h,*~}

.PHONY: install
install:
	@( for d in $(obj) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d install ; fi ; \
        done ) ;

.PHONY: uninstall
uninstall:
	@( for d in $(obj) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d uninstall ; fi ; \
        done ) ;
