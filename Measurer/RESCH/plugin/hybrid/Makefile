plugins = fp-pm edf-wm

.PHONY: all
.PHONY: clean

all: $(plugins)
$(plugins)::
	@$(MAKE) -C $@

clean:
	@( for d in $(plugins) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d clean ; fi ; \
        done ) ;

