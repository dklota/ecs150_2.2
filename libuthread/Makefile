# target library
lib := libuthread.a

# object files to include in library
objs := queue.o

CC := gcc
CFLAGS := -Wall -Werror -g

all: $(lib)

# create static library, as provided in the link in the project information
$(lib): $(objs)
	ar rcs $@ $^

# compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(objs) $(lib)
