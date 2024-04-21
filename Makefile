# all:
# 	g++ \
# -I ../Libraries/SDL2-2.30.0/x86_64-w64-mingw32/include \
# -I ../Libraries/enet-1.3.18/include \
# -I include \
# -L ../Libraries/SDL2-2.30.0/x86_64-w64-mingw32/lib \
# -L ../Libraries/enet-1.3.18 \
# -o server src/server.cpp \
# -lmingw32 -lSDL2main -lSDL2 -lenet64 -lws2_32 -lwinmm

all:
	g++ \
-I ../Libraries/SDL2-2.30.0/x86_64-w64-mingw32/include \
-I ../Libraries/enet-1.3.18/include \
-I include \
-L ../Libraries/SDL2-2.30.0/x86_64-w64-mingw32/lib \
-L ../Libraries/enet-1.3.18 \
-o client src/client.cpp \
-lmingw32 -lSDL2main -lSDL2 -lenet64 -lws2_32 -lwinmm