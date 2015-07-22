SUBDIRS=client server
PROGRAMS=client server

recursive:
	cd src; for i in $(SUBDIRS); do cd $$i; make $$i; if [ $$? -ne 0 ]; then exit 1; fi; cd -; done

install: 
	cd src; for i in $(SUBDIRS); do cd $$i; make $@; if [ $$? -ne 0 ]; then exit 1; fi; cd -; done

.PHONY:
clean:
	cd src; for i in $(SUBDIRS); do cd $$i; make clean; if [ $$? -ne 0 ]; then exit 1; fi; cd -; done
	cd bin; rm $(PROGRAMS)
