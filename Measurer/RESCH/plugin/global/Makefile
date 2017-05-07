plugins = fp fp-us fp-zl edf edf-us edf-zl

.PHONY: all
.PHONY: clean

all: $(plugins)
$(plugins)::
	@$(MAKE) -C $@

clean:
	@( for d in $(plugins) _ ; do \
                if [ "$$d" != "_" ] ; then $(MAKE) -C $$d clean ; fi ; \
        done ) ;

