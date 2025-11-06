mkdir ../bin
cp -r ../src/shaders ../bin/

./fonts/atlas ./fonts/Ovo-Regular.ttf ../bin/fonts/ovo_small.cmfont 64 > /dev/null
./fonts/atlas ./fonts/Ovo-Regular.ttf ../bin/fonts/ovo_large.cmfont 108 > /dev/null

g++ -g -o ../bin/tattile\
	../src/game/main.cpp ../src/window/xlib/xlib_window.cpp ../src/time/unix/unix_time.cpp ../src/renderer/opengl/opengl.cpp \
	../src/renderer/opengl/GL/gl3w.c \
	-I ../src/ \
	-I ../dependencies/freetype-2.14.1/out/include/freetype2/ \
	-L ../dependencies/freetype-2.14.1/out/lib/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes -lfreetype
