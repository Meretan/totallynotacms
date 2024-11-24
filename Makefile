
default:
	cd cms && $(MAKE)
	@echo Please find your binaries in the cms/ directory.

clean:
	cd cms && $(MAKE) clean
	cd lib && $(MAKE) clean
