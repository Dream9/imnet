ALL:target

TOPDIR=$(shell pwd)
SUBDIR=
INCLUDEDIR=../../
LIBPATH=../lib/
LIBNAME=libdependent.a

CC=gcc
XX=g++
DEBUG=-ggdb -O0
FLAG+=-std=c++11 -Wall -I$(INCLUDEDIR) -lpthread 

RELEASE=0

ifeq ($(RELEASE),0)
	FLAG+=$(DEBUG) 
else
	FLAG+=-O3 -static -DNDEBUG
endif

AUTODETACH=0
ifneq ($(AUTODETACH),0)
	FLAG+=D_THREADTASK_AUTO_DETACH
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
	@mv *.o $(LIBPATH) && cd $(LIBPATH) && ar crs $(LIBNAME) *.o
	@echo done!


.PHONY:clean

clean:
	@for dir in $(SUBDIR);\
		do $(MAKE) -C $$dir clean || exit 1;\
		done
	rm -f *.o
	@echo $(TOPDIR) object files has been removed!
