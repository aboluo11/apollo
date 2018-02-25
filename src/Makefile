CC = gcc
CFLAGS = -g -O0
OBJS_DIR = ../build/
SRCS = buffer.c conn.c parse.c request.c response.c server.c string.c dict.c pool.c apl_timer.c
OBJS = $(addprefix $(OBJS_DIR), $(SRCS:.c=.o))

all:
	@mkdir -p $(OBJS_DIR)
	@make $(OBJS_DIR)apollo
	@make clean

$(OBJS_DIR)apollo: $(OBJS)
	$(CC) $(OBJS) -o $@

$(OBJS_DIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm $(OBJS_DIR)*.o