src = $(wildcard *.c)
obj = $(patsubst %.c, %.o, $(src))
target = app
CC = gcc
CFLAGS = -c -Wall
$(target) : $(obj)
	$(CC) $^ -o $@

%.o : %.c 
	$(CC) $(CFLAGS) $< -o $@

.PHONY : clean test
clean:
	-rm $(target) $(obj) -f

test:
	@echo $(src)
	@echo $(obj)
