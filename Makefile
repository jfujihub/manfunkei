CC	= gcc
CFLAGS	= `pkg-config --cflags gtk+-2.0`
LIBS	= `pkg-config --libs gtk+-2.0`
OBJS	= manfunkei.o


manfunkei: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

manfunkei.o: manfunkei.c icon.png
	$(CC) -c $< $(CFLAGS)

icon.png :
	convert -size 64x64 -background white -fill blue -gravity center -font /usr/share/fonts/truetype/fonts-japanese-gothic.ttf label:万分\\n計 icon.png


clean :
	rm -f $(OBJS) icon.png

