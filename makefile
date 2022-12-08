cc = gcc
ccache = ccache

cflags = -DD_notify_events -DD_gcc -Wall -Wextra
#cflags = -DD_gcc
release_cflags = -DD_release -DD_quiet -O3 -march=native -pipe
debug_cflags = -DD_debug -Og -g -fsanitize=undefined
ldflags = -lX11
release_ldflags = -Wl,-O1,--as-needed,-z,relro,-z,now
debug_ldflags = -lubsan
source_directory = src

exec := glass-shard
dependencies := $(wildcard ./${source_directory}/*.c)
object_directory := obj
release_directory := rel
release_objects := $(dependencies:./${source_directory}/%.c=./${release_directory}/${object_directory}/%.o)
debug_directory := deb
debug_objects := $(dependencies:./${source_directory}/%.c=./${debug_directory}/${object_directory}/%.o)

default: debug-build
.PHONY: default

help:
	echo -e "valid commands:\n" \
	        "\tbuild\n" \
	        "\trun\n" \
	        "\tdebug-build\n" \
	        "\tdebug-run\n" \
	        "\tdebug\n" \
	        "\tclean\n"
.PHONY: help
.SILENT: help

build: ./${release_directory}/${exec}
.PHONY: build

debug-build: ./${debug_directory}/${exec}
.PHONY: debug-build
.SILENT: debug-build

run: ./${release_directory}/${exec}
	echo "[startx] $^"
	startx $^
.PHONY: run
.SILENT: run

debug-run: ./${debug_directory}/${exec}
	echo "[startx] $^"
	startx $^
.PHONY: debug-run
.SILENT: debug-run

debug: ./${debug_directory}/${exec}
	echo "[exec]   gdb -p \$$(pidof ${exec})"
	gdb -p $$(pidof ${exec})
.PHONY: debug
.SILENT: debug

clean:
	echo "[clean]  ./${release_directory}/${exec}"
	rm -f ./${release_directory}/${exec}
	echo "[clean]  ${release_objects}"
	rm -f ${release_objects}
	echo "[clean]  ./${debug_directory}/${exec}"
	rm -f ./${debug_directory}/${exec}
	echo "[clean]  ${debug_objects}"
	rm -f ${debug_objects}
	echo "[clean]  ./.dependencies.mk"
	rm -f ./.dependencies.mk
.PHONY: clean
.SILENT: clean

./${release_directory}/${exec}: ./${release_directory}/${object_directory} ./.dependencies.mk ${release_objects} ./makefile
	echo "[link]   ./$@"
	gcc ${release_objects} -o ./$@ ${ldflags} ${release_ldflags}
.SILENT: ./${release_directory}/${exec}

./${debug_directory}/${exec}: ./${debug_directory}/${object_directory} ./.dependencies.mk ${debug_objects} ./makefile
	echo "[link]   ./$@"
	gcc ${debug_objects} -o ./$@ ${ldflags} ${debug_ldflags}
.SILENT: ./${debug_directory}/${exec}

./.dependencies.mk: ./generate-dependencies.sh ./${source_directory}/*.c ./${source_directory}/*.h
	echo "[depgen] ./$@"
	./generate-dependencies.sh ./$@ ./${source_directory} ./${release_directory}/${object_directory} ./${debug_directory}/${object_directory}
.SILENT: ./.dependencies.mk

./${release_directory}/${object_directory}/%.o: ./${source_directory}/%.c ./makefile
	echo "[build]  ./$@"
	${ccache} ${cc} ./$< -c -o ./$@ ${cflags} ${release_cflags}
.SILENT: ${release_objects}

./${debug_directory}/${object_directory}/%.o: ./${source_directory}/%.c ./makefile
	echo "[build]  ./$@"
	${ccache} ${cc} ./$< -c -o ./$@ ${cflags} ${debug_cflags}
.SILENT: ${debug_objects}

./${release_directory}/${object_directory}:
	echo "[mkdir]  ./$@"
	mkdir -p ./$@
.SILENT: ./${release_directory}/${object_directory}

./${debug_directory}/${object_directory}:
	echo "[mkdir]  ./$@"
	mkdir -p ./$@
.SILENT: ./${debug_directory}/${object_directory}

include .dependencies.mk
