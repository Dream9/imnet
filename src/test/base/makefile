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

TestTarget:=timezone_test logger_test logfile_test asynclogger_test eventloop_test acceptor_test
TestTarget+=timerwork_queue_test tcp_server_test tcp_client_test

timezone_test:timezone_test.cc 
	$(XX) $(FLAG) -o timezone_test.ef $^ $(LINKARCHIVE) 

logger_test:logger_test.cc
	$(XX) $(FLAG) -o logger_test.ef $^ $(LINKARCHIVE) 

logfile_test:logfile_test.cc
	$(XX) $(FLAG) -o logfile_test.ef $^ $(LINKARCHIVE) -lpthread

asynclogger_test:asynclogger_test.cc
	$(XX) $(FLAG) -o asynclogger_test.ef $^ $(LINKARCHIVE) -lpthread

acceptor_test:acceptor_test.cc
	$(XX) $(FLAG) -o acceptor_test.ef $^ $(LINKARCHIVE)

eventloop_test:eventloop_test.cc
	$(XX) $(FLAG) -o eventloop_test.ef $^ $(LINKARCHIVE)

tcp_server_test:tcp_server_test.cc
	$(XX) $(FLAG) -o tcp_server_test.ef $^ $(LINKARCHIVE) -lpthread

tcp_client_test:tcp_client_test.cc
	$(XX) $(FLAG) -o tcp_client_test.ef $^ $(LINKARCHIVE) -lpthread

timerwork_queue_test:timerwork_queue_test.cc
	$(XX) $(FLAG) -o timerwork_queue_test.ef $^ $(LINKARCHIVE) -lpthread

target: $(TestTarget)
	mv *.ef $(BINPATH)/
	@echo Done!

.PHONY :clean

clean:
	@rm -f *.o *.ef
	@echo Done!


