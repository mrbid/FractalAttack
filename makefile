all:
	gcc main.c glad_gl.c -Ofast -lglfw -lm -o fat

install:
	cp fat $(DESTDIR)

uninstall:
	rm $(DESTDIR)/fat

clean:
	rm fat