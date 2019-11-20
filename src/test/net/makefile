ALL:target

TOPDIR=$(shell pwd);
#INCLUDEDIR=../../
#LIBPATH=../lib/

#becare:受制于C的单遍编译模型，为了减少错误，该项必须位于最后
LINKARCHIVE=-limnet

CC=gcc
XX=g++
DEBUG=-ggdb -O0
FLAG=-std=c++11 -Wall -I$(INCLUDEDIR) -L$(LIBPATH)  

#REALEASE=0
#设置发行版本，RELEASE为0时(默认)发布debug
ifeq ($(RELEASE),)
	RELEASE=0
endif

ifeq ($(BINPATH),)
	BINPATH=../../bin
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

TestTarget:=httpd_server_test dreamn

httpd_server_test:httpd_server_test.cc
	$(XX) $(FLAG) -o httpd_server_test.ef $^ $(LINKARCHIVE) -lpthread

dreamn:dreamn.cc
	$(XX) $(FLAG) -I/mnt/share/Dreamn/DreamSE/src/query -I/mnt/share/Dreamn/DreamSE/src/utils\
		-I/mnt/share/Dreamn/DreamSE/src/index -L/mnt/share/Dreamn/DreamSE/src/lib \
		-o dreamn.ef $^ $(LINKARCHIVE) -ldream-search -lpthread

target: $(TestTarget)
	mv *.ef $(BINPATH)/
	@echo Done!

.PHONY :clean

clean:
	@rm -f *.o *.ef
	@echo Done!


