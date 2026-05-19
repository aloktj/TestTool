CC ?= gcc

CFLAGS ?= -Wall -Wextra -O2
CFLAGS += -I. -Isrc

LDFLAGS ?=
LDFLAGS += -lm

ifeq ($(OS),Windows_NT)
LDFLAGS += -lws2_32
endif

TARGET := webbuilder_api_demo

SRCS := \
	main.c \
	mongoose.c \
	src/routes.c \
	src/auth.c \
	src/api_public.c \
	src/api_auth.c \
	src/api_data.c \
	src/api_control.c \
	src/api_settings.c \
	src/websocket.c \
	src/demo_data.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
