all:
	gcc main.c glad_gl.c -Ofast -lglfw -lm -o fractalattack

install:
	cp fractalattack $(DESTDIR)

uninstall:
	rm $(DESTDIR)/fractalattack

clean:
	rm fractalattack