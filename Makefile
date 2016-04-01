CFLAGS := -Wall -Werror -Wfatal-errors
CFLAGS += -std=gnu11
CFLAGS += -O0
CFLAGS += -ggdb3
CFLAGS += -MMD
CFLAGS += -I common

CLIENT := client
SERVER := server

TEMP := build

all: $(CLIENT).bin $(SERVER).bin

CLIENT_SRC := $(shell find $(CLIENT)/src/* -type f -name "*.c")
CLIENT_OBJ := $(CLIENT_SRC:%.c=$(TEMP)/%.o)
CLIENT_DEP := $(CLIENT_SRC:%.c=$(TEMP)/%.d)

SERVER_SRC := $(shell find $(SERVER)/src/* -type f -name "*.c")
SERVER_OBJ := $(SERVER_SRC:%.c=$(TEMP)/%.o)
SERVER_DEP := $(SERVER_SRC:%.c=$(TEMP)/%.d)

$(CLIENT).bin: $(CLIENT_OBJ) $(LIB_OBJ)
	@$(CC) $^ -lncurses -lpthread -o $@
	@echo +ld $^

$(SERVER).bin: $(SERVER_OBJ) $(LIB_OBJ)
	@$(CC) $^ -lpthread -lsqlite3 -o $@
	@echo +ld $^

$(TEMP)/%.o: %.c
	@mkdir -p $(TEMP)/$(dir $<)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo +cc $<

-include $(CLIENT_DEP)

-include $(SERVER_DEP)

.PHONY: clean

clean:
	-@rm -rf $(TEMP) 2> /dev/null
	-@rm -f $(CLIENT).bin 2> /dev/null
	-@rm -f $(SERVER).bin 2> /dev/null
