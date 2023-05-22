# CC     := gcc
# CFLAGS := -Wall -Werror 

# SRCS   := client.c \
# 	server.c 

# OBJS   := ${SRCS:c=o}
# PROGS  := ${SRCS:.c=}

# .PHONY: all
# all: ${PROGS}

# ${PROGS} : % : %.o Makefile
# 	${CC} $< -o $@ udp.c

# clean:
# 	rm -f ${PROGS} ${OBJS}

# %.o: %.c Makefile
# 	${CC} ${CFLAGS} -c $<

CC		:= gcc
# CFLAGS	:= -Wall -Werror -g
CFLAGS	:= -Werror -g
LDFLAGS	:= -L. -lmfs

LIB		:= mfs.c

DEPS	:= udp.c

.PHONY: all
all: libmfs.so server

server: server.o ${DEPS}
	${CC} ${CFLAGS} -o server server.o ${DEPS}

client: client.o libmfs.so
	${CC} ${CFLAGS} -o client client.o ${LDFLAGS}

libmfs.so : mfs.o ${DEPS}
	${CC} ${CFLAGS} -fPIC -shared -Wl,-soname,libmfs.so -o libmfs.so mfs.o udp.c -lc

clean:
	rm -f ./client ./server *.o libmfs.so

mfs.o : ${LIB} Makefile
	${CC} ${CFLAGS} -c -fPIC ${LIB}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<