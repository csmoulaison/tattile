mkdir ../bin
cp -r fonts ../bin/fonts
cp -r ../src/shaders ../bin/shaders

g++ -g -o ../bin/tattile\
	../src/game/main.cpp ../src/window/xlib/xlib_window.cpp ../src/time/unix/unix_time.cpp ../src/renderer/opengl/opengl.cpp \
	../src/renderer/opengl/GL/gl3w.c \
	-I ../src/ \
	-I ../dependencies/freetype-2.14.1/out/include/freetype2/ \
	-L ../dependencies/freetype-2.14.1/out/lib/ \
	-lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes -lfreetype
