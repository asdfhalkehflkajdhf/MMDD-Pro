#---------------------------------------------------------------------
#   author: cenlizhong
#     date: 2012-9-29 
# describe: a versatile Makefile
#
#       ps: language type (c/cpp)
#       cc: compiler (gcc/g++)
#   CFLAGS: c/c++ language flags (for example: -g -wall -o2)
#     DEST: output file name 
#     LIBS: library name (for example: pthread)
# INCLUDES: header file directory (for example: ./include)
#  SUB_DIR: subdirectory (for example: dir1 dir1/aaa dir/bbb dir2) 
# DEST_DIR: prefix install directory   
#  INSTALL: install directory (actually install to $DEST_DIR/$INSTALL)  
#----------------------------------------------------------------------

#----------------------configure in this part
PS = c
CC = gcc 
CFLAGS = -g -Wall -D_7ZIP_ST
DEST := newiup
ROOT_DIR=$(shell pwd)
LIBS_DIR :=  /usr/lib64/mysql ./ext/lib
LIBS := pthread dl mysqlclient uchardet archive z
INCLUDES := include utils yaml  archive/Archive  archive/Lzma
SUB_DIR := utils yaml archive/Archive  archive/Lzma
DEST_DIR := ./ext/bin
DEST_LIB := ext/lib

#----------------------do nothing in this part
RM := rm -f
CFLAGS  += -MMD -MF $(patsubst ./%, %, $(patsubst %.o, %.d, $(dir $@).$(notdir $@))) $(addprefix -I, $(INCLUDES)) 
SRCS := $(wildcard *.$(PS) $(addsuffix /*.$(PS), $(SUB_DIR)))
OBJS := $(patsubst %.$(PS), %.o, $(SRCS))
DEPS := $(patsubst %.$(PS), %.d, $(foreach n,$(SRCS),$(patsubst ./%, %, $(dir $n).$(notdir $n))))
MISS := $(filter-out $(wildcard DEPS), $(DEPS))

all: $(DEST)

clean :
	@$(RM) $(OBJS) 
	@$(RM) $(DEPS) 
	@$(RM) $(DEST)
cleanall : clean
	@$(RM) $(DEST_DIR)/*

install:
	cp -f cron.sh $(DEST_DIR)/$(DEST)-cron.sh
	cp -f conf.yaml $(DEST_DIR)/$(DEST)-conf.yaml.template
	cp -f newiup2.0-install-reference.doc $(DEST_DIR)/$(DEST)-reference.doc
	cp -f $(DEST) $(DEST_DIR)/$(DEST)


ifneq ($(MISS),)
$(MISS):
	@$(RM) $(patsubst %.d, %.o, $(dir $@)$(patsubst .%,%, $(notdir $@)))
endif

-include $(DEPS)

$(DEST): $(OBJS)
	$(CC) -o $(DEST) $(OBJS) $(addprefix -L, $(LIBS_DIR)) $(addprefix -l,$(LIBS)) -Wl,-rpath=$(ROOT_DIR)/$(DEST_LIB)

