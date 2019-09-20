ALL:target

TOPDIR=$(shell pwd);
INCLUDEDIR=../../
LIBPATH=../lib/

#becare:受制于C的单遍编译模型，为了减少错误，该项必须位于最后
LINKARCHIVE=-ldependent

CC=gcc
XX=g++

FLAG=-std=c++11 -Wall -I$(INCLUDEDIR) -L$(LIBPATH) 

REALEASE=0
ifeq ($(REALEASE),0)
	FLAG+= -ggdb -O0 
else
	FLAG+= -O3 -static 
endif

timezone_test:timezone_test.cc 
	$(XX) $(FLAG) -o timezone_test.ef $^ $(LINKARCHIVE) 

logger_test:logger_test.cc
	$(XX) $(FLAG) -o logger_test.ef $^ $(LINKARCHIVE) 

logfile_test:logfile_test.cc
	$(XX) $(FLAG) -o logfile_test.ef $^ $(LINKARCHIVE) -lpthread

target:timezone_test logger_test
	@echo Done!

.PHONY :clean

clean:
	@rm -f *.o *.ef
	@echo Done!


