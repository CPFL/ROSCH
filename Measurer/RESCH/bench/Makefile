benchmarks = schedbench resbench

.PHONY: all
.PHONY: clean
.PHONY: distclean

all: $(benchmarks)
$(benchmarks)::
	@$(MAKE) -C $@

clean:
	@( for d in $(benchmarks) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d clean ; fi ; \
        done ) ;
	rm -f ./include/config.h

distclean:
	@( for d in $(benchmarks) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d distclean ; fi ; \
        done ) ;
	rm -f ./include/config.h
