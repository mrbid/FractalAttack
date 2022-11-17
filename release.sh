clang main.c glad_gl.c -I inc -Ofast -lglfw -lm -o fat
i686-w64-mingw32-gcc main.c glad_gl.c -Ofast -Llib -lglfw3dll -lm -o fat.exe
upx fat
upx fat.exe