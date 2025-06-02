CC = riscv64-unknown-linux-gnu-gcc
CPP = riscv64-unknown-linux-gnu-g++

OFLAGS := -O3 -DRISCV -march=rv64imafd -DUSE_PUBLIC_MALLOC_WRAPPERS
CFLAGS:= -O3 -DRISCV -march=rv64imafd

CFLAGS_CM:= -O3 -DRISCV -march=rv64imafd -Wl,--allow-multiple-definition
CFLAGS_MS:= -fpic -std=c++17

#CC = gcc
#CFLAGS := -O3 -std=gnu99
initialisation_none: initialisation_none.c
	${CC} ${CFLAGS} -o initialisation_none.riscv initialisation_none.c -lpthread

initialisation_pmc: initialisation_pmc.c
	${CC} ${CFLAGS} -o initialisation_pmc.riscv initialisation_pmc.c -lpthread

initialisation_sanitiser: initialisation_sanitiser.c
	${CC} ${CFLAGS} -o initialisation_sanitiser.riscv initialisation_sanitiser.c -lpthread

initialisation_minesweeper: initialisation_minesweeper.c
	${CC} ${CFLAGS} -o initialisation_minesweeper.riscv initialisation_minesweeper.c -lpthread

initialisation_ss: initialisation_ss.cpp
	${CPP} ${CFLAGS} -o initialisation_ss.riscv initialisation_ss.cpp -lpthread

initialisation_ss_mc: initialisation_ss_mc.cpp
	${CPP} ${CFLAGS} -o initialisation_ss_mc.riscv initialisation_ss_mc.cpp -lpthread



malloc: malloc.c
	${CC} ${OFLAGS} -c malloc.c

gc_main_none: gc_main_none.c
	${CC} ${CFLAGS_CM} -c gc_main_none.c -lpthread

gc_main_pmc: gc_main_pmc.c
	${CC} ${CFLAGS_CM} -c gc_main_pmc.c -lpthread

gc_main_sanitiser: gc_main_sanitiser.c
	${CC} ${CFLAGS_CM} -c gc_main_sanitiser.c -lpthread

gc_main_ss: gc_main_ss.cpp
	${CPP} ${CFLAGS_CM} -c gc_main_ss.cpp -lpthread

gc_main_ss_mc: gc_main_ss_mc.c
	${CC} ${CFLAGS_CM} -c gc_main_ss_mc.c -lpthread

gc_main_minesweeper: gc_main_minesweeper.cpp
	${CPP} ${CFLAGS_CM} ${CFLAGS_MS} -c gc_main_minesweeper.cpp -lpthread 

# checker_ss: checker_ss.cpp
#	${CPP} ${CFLAGS} -o checker_ss.riscv checker_ss.cpp -lpthread

################################################
# Some test casese
################################################
tc_pmc: tc_pmc.c
	${CC} ${CFLAGS} -o tc_pmc.riscv tc_pmc.c -lpthread

tc_sanitiser: tc_sanitiser.c
	${CC} ${CFLAGS_CM} -o tc_sanitiser.riscv tc_sanitiser.c malloc.o -lpthread

tc_shadowstack-s: tc_shadowstack-s.c
	${CC} ${CFLAGS} -o tc_shadowstack-s.riscv tc_shadowstack-s.c -lpthread

tc_shadowstack-m: tc_shadowstack-m.c
	${CC} ${CFLAGS} -o tc_shadowstack-m.riscv tc_shadowstack-m.c -lpthread

test: test.cpp
	${CPP} ${CFLAGS} -o test.riscv test.cpp -lpthread

clean:
	rm -f *.o *.riscv *.so

