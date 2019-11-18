ALL:target

TOPDIR=$(shell pwd)
SUBDIR=

#INCLUDEDIR=../../
#LIBPATH="../lib/"
LIBNAME="libimnet.a"

CC=gcc
XX=g++
DEBUG=-ggdb -O0
FLAG+=-std=c++11 -Wall -I$(INCLUDEDIR)

#RELEASE=0
ifeq ($(RELEASE),)
	RELEASE=0
endif

ifeq ($(BINPATH),)
	BINPATH=$(TOPDIR)/../../bin/
endif

ifeq ($(INCLUDEDIR),)
	INCLUDEDIR=../../
endif

ifeq ($(LIBPATH),)
	LIBPATH=../../lib/
endif

ifeq ($(RELEASE),0)
	FLAG+=$(DEBUG) 
else
	FLAG+=-O3 -static -DNDEBUG
endif


%.o:%.cc
	$(XX) $(FLAG) -c $< -o $@

SOURCES=$(wildcard *.c *.cc *.cpp)
OBJECTS=$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(SOURCES))))

showall:
	@echo $(OBJECTS)
	@echo $(FLAG)
	@echo $(RELEASE)

target:$(OBJECTS) 
	@echo file list:
	@echo $(OBJECTS)
	mv *.o $(LIBPATH) && cd $(LIBPATH) && ar crs $(LIBNAME) *.o
	@echo done!

$(SUBDIR):ECHO
	$(MAKE) -C $@

ECHO:
	@echo ...

.PHONY:clean

clean:
	@for dir in $(SUBDIR);\
		do $(MAKE) -C $$dir clean || exit 1;\
		done
	rm -f *.o
	@echo $(TOPDIR) object files has been removed!
