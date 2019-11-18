ALL:target
#ALL:testexport

TOPDIR=$(shell pwd)
SUBDIR=

#以下变量期待从上层makefile传入
#INCLUDEDIR=../
#LIBPATH=./lib/
#RELEASE=0

LIBNAME=libimnet.a

CC=gcc
XX=g++
DEBUG=-ggdb -O0
FLAG+=-std=c++11 -Wall -I$(INCLUDEDIR)


#设置发行版本，RELEASE为0时(默认)发布debug
ifeq ($(RELEASE),)
	RELEASE=0
endif

ifeq ($(BINPATH),)
	BINPATH=$(TOPDIR)/../bin/
endif

ifeq ($(INCLUDEDIR),)
	INCLUDEDIR=../
endif

ifeq ($(LIBPATH),)
	LIBPATH=../lib/
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

testexport:
	@echo $(BINPATH)
	@echo $(INCLUDEDIR)
	@echo $(LIBPATH)
	@echo $(RELEASE)
	@echo done

showall:
	@echo $(OBJECTS)
	@echo $(FLAG)
	@echo $(RELEASE)

target:$(OBJECTS) $(SUBDIR) 
	@echo file list:
	@echo $(OBJECTS)
	@(mv *.o $(LIBPATH) && cd $(LIBPATH) && ar crs $(LIBNAME) *.o) || echo none obj
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
