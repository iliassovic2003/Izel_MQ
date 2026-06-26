NAME    = izmq

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g

SRCS    = core/ialloc/lock_free_malloc.c \
          core/ialloc/big_pool.c \
          core/ialloc/small_pool.c \
          core/message/message.c \
          core/queue/queue.c \
          core/broker/broker.c \
          metrics/cpu.c \
          metrics/memory.c \
          metrics/network.c \
          main.c

OBJS    = $(SRCS:.c=.o)

all: $(NAME)

asan: CFLAGS += -fsanitize=address
asan: $(NAME)

tsan: CFLAGS += -fsanitize=thread
tsan: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -lpthread -lrt

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re asan tsan