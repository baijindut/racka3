PREFIX = /usr/local

CC = g++

OPTS = -O6 -ffast-math -funroll-loops -Wall -Wno-psabi -fPIC -DPIC
#OPTS = -g -DDEBUG 

_LDFLAGS = -shared 
STRIP = strip

-include defines.make

CFLAGS = $(OPTS) $(_CFLAGS)
LDFLAGS = $(_LDFLAGS) $(CFLAGS)

VERSION = 0.9.16
PLUG = caps

SOURCES = $(wildcard *.cc) $(wildcard dsp/*.cc)
OBJECTS	= $(SOURCES:.cc=.o) 
HEADERS = $(wildcard *.h) $(wildcard dsp/*.h) $(wildcard util/*.h) $(wildcard dsp/tonestack/*.h)

PDF = releases/caps-$(VERSION).pdf

DEST = $(PREFIX)/lib/ladspa
RDFDEST = $(PREFIX)/share/ladspa/rdf

# targets following -------------------------------------------------------------

all: depend $(PLUG).so tags

run: all
	@#python bin/enhance_dp_wsop.py
	@#python -i bin/rack.py White AutoFilter cream.Audio.Meter Pan
	@#python -i bin/rack.py White AutoFilter Pan
	@#python -i bin/rack.py Click Plate
	@#python -i bin/rack.py PhaserII 
	@#~/cream/gdb-python html/graph.py Eq4p,a.f=100,a.gain=30.png
	@#~/cream/gdb-python bin/eqtest.py
	@#~/cream/gdb-python bin/fractalstest.py
	@python bin/sinsweep.py

rdf: $(PLUG).rdf
$(PLUG).rdf: all tools/make-rdf.py
	python tools/make-rdf.py > $(PLUG).rdf

$(PLUG).so: $(OBJECTS)
	$(CC) $(ARCH) $(LDFLAGS) -o $@ $(OBJECTS)

.cc.s: 
	$(CC) $(ARCH) $(CFLAGS) -S $<

.cc.o: depend 
	$(CC) $(ARCH) $(CFLAGS) -o $@ -c $<

tags: $(SOURCES) $(HEADERS)
	@-if [ -x /usr/bin/ctags ]; then ctags $(SOURCES) $(HEADERS) >/dev/null 2>&1 ; fi

install: all
	@$(STRIP) $(PLUG).so > /dev/null
	install -d $(DEST)
	install -m 644 $(PLUG).so $(DEST)
	install -d $(RDFDEST)
	install -m 644 $(PLUG).rdf $(RDFDEST)

fake-install: all
	-rm $(DEST)/$(PLUG).so
	ln -s `pwd`/$(PLUG).so $(DEST)/$(PLUG).so
	-rm $(RDFDEST)/$(PLUG).rdf
	ln -s `pwd`/$(PLUG).rdf $(RDFDEST)/$(PLUG).rdf

rdf-install:
	install -d $(RDFDEST)
	install -m 644 $(PLUG).rdf $(RDFDEST)

uninstall:
	-rm $(DEST)/$(PLUG).so
	-rm $(DEST)/$(PLUG)-ng.so

clean:
	rm -f $(OBJECTS) $(PLUG).so *.s depend

version.h:
	@VERSION=$(VERSION) python tools/make-version.h.py

dist: all $(PLUG).rdf version.h
	-rm doc/*.html 
	tools/make-dist.py caps $(VERSION) $(CFLAGS)

depend: $(SOURCES) $(HEADERS)
	$(CC) -MM $(CFLAGS) $(DEFINES) $(SOURCES) > depend

-include depend
