SUBDIRS := src tests/UnitTests examples/qsort examples/imageProcessing

define submake
	for d in $(SUBDIRS); do \
		$(MAKE) -j9 $(1) --directory=$$d; \
	done
endef

define genmake
	for d in $(SUBDIRS); do \
		qmake $$d -o $$d/Makefile; \
	done
endef

define cleanImages
	for d in $(SUBDIRS); do \
		rm -f $$d/*.png; \
	done; \
	rm -rf doc/html;
endef

all:
	doxygen doc/res/Doxyfile
	$(call genmake)
	$(call submake, $@)
	
clean:
	$(call submake, $@)
	$(call cleanImages)
	
distclean:
	$(call submake, $@)
	$(call cleanImages)
