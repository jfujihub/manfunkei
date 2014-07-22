CC	= gcc
CFLAGS	= `pkg-config --cflags gtk+-2.0`
LIBS	= `pkg-config --libs gtk+-2.0`
OBJS	= manfunkei.o


manfunkei: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) -c $< $(CFLAGS)


clean :
	rm -f $(OBJS)
