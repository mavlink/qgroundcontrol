#-- set this to the man directory you would like to use
MANPATH:=/usr/share/man

#-- uncomment this to enable debugging
#DEBUG:=-g -DDEBUG


###### YOU SHOULD NOT CHANGE BELOW THIS LINE ######

VERSION:=1.4.1
SHELL:=/bin/bash
SRCS:=api.c
MANS:=man3/libxbee.3 \
      man3/xbee_con.3 \
      man3/xbee_end.3 \
      man3/xbee_endcon.3 \
      man3/xbee_flushcon.3 \
      man3/xbee_purgecon.3 \
      man3/xbee_getanalog.3 \
      man3/xbee_getdigital.3 \
      man3/xbee_getpacket.3 \
      man3/xbee_hasanalog.3 \
      man3/xbee_hasdigital.3 \
      man3/xbee_logit.3 \
      man3/xbee_newcon.3 \
      man3/xbee_nsenddata.3 \
      man3/xbee_pkt.3 \
      man3/xbee_senddata.3 \
      man3/xbee_setup.3 \
      man3/xbee_setupAPI.3 \
      man3/xbee_setuplog.3 \
      man3/xbee_setuplogAPI.3 \
      man3/xbee_vsenddata.3
MANPATHS:=$(foreach dir,$(shell ls man -ln | grep ^d | tr -s ' ' | cut -d ' ' -f 9),${MANPATH}/$(dir))

PDFS:=${SRCS} ${SRCS:.c=.h} makefile main.c xbee.h

CC:=gcc
CFLAGS:=-Wall -Wstrict-prototypes -Wno-variadic-macros -pedantic -c -fPIC ${DEBUG}
CLINKS:=-lpthread -lrt ${DEBUG}
DEFINES:=

ifeq ($(strip $(wildcard ${MANPATH}/man3/libxbee.3.bz2)),)
FIRSTTIME:=TRUE
else
FIRSTTIME:=FALSE
endif

ENSCRIPT:=-MA4 --color -f Courier8 -C --margins=15:15:0:20
ifneq ($(strip $(wildcard /usr/share/enscript/mine-web.hdr)),)
  ENSCRIPT+= --fancy-header=mine-web
else
  ENSCRIPT+= --fancy-header=a2ps
endif

SRCS:=${sort ${SRCS}}
PDFS:=${sort ${PDFS}}

.PHONY: FORCE
.PHONY: all run new clean cleanpdfs main pdfs html
.PHONY: install install_su install_man
.PHONY: uninstall uninstall_su uninstall_man/


# all - do everything (default) #
all: ./lib/libxbee.so.$(VERSION)
	@echo "*** Done! ***"


# run - remake main and then run #
run: all main
	./bin/main


# new - clean and do everything again #
new: clean all


# clean - remove any compiled files and PDFs #
clean:
	rm -f ./*~
	rm -f ./sample/*~
	rm -f ./obj/*.o
	rm -f ./lib/libxbee.so*
	rm -f ./bin/main

cleanpdfs:
	rm -f ./pdf/*.pdf


# install - installs library #
install: ./lib/libxbee.so.$(VERSION)
	@echo
	@echo
ifneq ($(shell echo $$USER),root)
	@echo "#######################################################################################"
	@echo "### To Install this library I need the root password please!"
	@echo "#######################################################################################"
endif
	su -c "make install_su --no-print-directory"
	@echo
ifeq (${FIRSTTIME},TRUE)
	@echo "#######################################################################################"
	@echo
	@pr -h "" -o 3 -w 86 -tT ./README
	@echo
	@echo "#######################################################################################"
endif

install_su: /usr/lib/libxbee.so.$(VERSION) /usr/include/xbee.h install_man

/usr/lib/libxbee.so.$(VERSION): ./lib/libxbee.so.$(VERSION)
	cp ./lib/libxbee.so.$(VERSION) /usr/lib/libxbee.so.$(VERSION) -f
	@chmod 755 /usr/lib/libxbee.so.$(VERSION)
	@chown root:root /usr/lib/libxbee.so.$(VERSION)
	ln ./libxbee.so.$(VERSION) /usr/lib/libxbee.so.1 -sf
	@chown root:root /usr/lib/libxbee.so.1
	ln ./libxbee.so.$(VERSION) /usr/lib/libxbee.so -sf
	@chown root:root /usr/lib/libxbee.so

/usr/include/xbee.h: ./xbee.h
	cp ./xbee.h /usr/include/xbee.h -f
	@chmod 644 /usr/include/xbee.h
	@chown root:root /usr/include/xbee.h

install_man: ${MANPATH} ${MANPATHS} ${addsuffix .bz2,${addprefix ${MANPATH}/,${MANS}}}

${MANPATH} ${MANPATHS}:
	@echo "#######################################################################################"
	@echo "### $@ does not exist... cannot install man files here!"
	@echo "### Please check the directory and the MANPATH variable in the makefile"
	@echo "#######################################################################################"
	@false

${MANPATH}/%.bz2: ./man/%
	@echo "cat $< | bzip2 -z > $@"
	@cat $< | bzip2 -z > $@ || ( \
	  echo "#######################################################################################"; \
	  echo "### Installing man page '$*' to '$@' failed..."; \
	  echo "#######################################################################################"; )
	@chmod 644 $@
	@chown root:root $@

./doc/:
	mkdir ./doc/

html: ./doc/ ./man/
	cd ./doc/; mkdir -p `find ../man/ -type d -not -path *.svn* | cut -b 2-`;
	find ./man/ -type f -not -path *.svn* | cut -d / -f 3- | sort > .html_todo
	for item in `cat .html_todo`; do \
	  man2html -r ./man/$$item | tail -n +3 > ./doc/man/$$item.html; \
	  done 2> /dev/null
	rm .html_todo

uninstall:
	@echo
	@echo
ifneq ($(shell echo $$USER),root)
	@echo "#######################################################################################"
	@echo "### To Uninstall this library I need the root password please!"
	@echo "#######################################################################################"
endif
	su -c "make uninstall_su --no-print-directory"
	@echo
	@echo

uninstall_su: ${addprefix uninstall_man/,${MANS}}
	rm /usr/lib/libxbee.so.$(VERSION) -f
	rm /usr/lib/libxbee.so.1 -f
	rm /usr/lib/libxbee.so -f	
	rm /usr/include/xbee.h -f

uninstall_man/%:
	rm ${MANPATH}/$*.bz2 -f

# main - compile & link objects #
main: ./bin/main

./bin/main: ./obj/api.o ./bin/ ./main.c
	${CC} ${CLINKS} ./main.c ./obj/api.o -o ./bin/main ${DEBUG}

./bin/:
	mkdir ./bin/

./lib/libxbee.so.$(VERSION): ./lib/ ${addprefix ./obj/,${SRCS:.c=.o}} ./xbee.h
	gcc -shared -Wl,-soname,libxbee.so.1 $(CLINKS) -o ./lib/libxbee.so.$(VERSION) ./obj/*.o
	ln ./libxbee.so.$(VERSION) ./lib/libxbee.so.1 -sf
	ln ./libxbee.so.$(VERSION) ./lib/libxbee.so -sf

./lib/:
	mkdir ./lib/

./obj/:
	mkdir ./obj/

./obj/%.o: ./obj/ %.c %.h xbee.h
	${CC} ${CFLAGS} ${DEFINES} ${DEBUG} $*.c -o $@

./obj/%.o: ./obj/ %.c xbee.h
	${CC} ${CFLAGS} ${DEFINES} ${DEBUG} $*.c -o $@


# pdfs - generate PDFs for each source file #
ifneq ($(strip $(wildcard /usr/bin/ps2pdf)),)
ifneq ($(strip $(wildcard /usr/bin/enscript)),)
pdfs: ./pdf/ ${addprefix ./pdf/,${addsuffix .pdf,${PDFS}}}

./pdf/:
	mkdir ./pdf/

./pdf/makefile.pdf: ./makefile
	enscript ${ENSCRIPT} -Emakefile $< -p - | ps2pdf - $@

./pdf/%.pdf: %
	enscript ${ENSCRIPT} -Ec $< -p - | ps2pdf - $@

./pdf/%.pdf:
	@echo "*** Cannot make $@ - '$*' does not exist ***"
else
pdfs:
	@echo "WARNING: enscript is not installed - cannot generate PDF files"
endif
else
pdfs:
	@echo "WARNING: ps2pdf is not installed - cannot generate PDF files"
endif
