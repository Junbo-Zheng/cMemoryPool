cxx = clang

prom = main
deps = $(shell find ./ -name "*.h")
src = $(shell find ./src -name "*.c")
src += main.c

obj = $(src:%.c=%.o)
LIBS:= -std=gnu11
CFLAGS:=-g -Wall -fsanitize=address -fno-omit-frame-pointer -O0 -ggdb

$(prom): $(obj)
	$(cxx) -o $(prom) $(obj) $(LIBS) $(CFLAGS) $(LIBS)

%.o: %.c $(deps)
	$(cxx) $(CFLAGS) -c $< -o $@ $(LIBS)

clean:
	rm -rf *.o $(prom) *.data
# .phony:install install_remote
# remote:
# 	# @sshpass -p "root" scp -P 22 $(prom) root@10.110.24.222:/opt/fls
# 	# @echo copy $(prom) to remote server ok...
