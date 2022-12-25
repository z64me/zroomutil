mkdir -p bin/

gcc -o bin/zroomutil -Wall -Wextra -std=c99 -pedantic -Og -g src/*.c -lm \
	-Wno-unused-parameter -Wno-unused-function
