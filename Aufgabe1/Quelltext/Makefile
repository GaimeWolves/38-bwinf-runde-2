C_SRC := $(wildcard *.cpp)

debug:
	g++ -o stromralley_d ${C_SRC} -Og -g

profile:	
	g++ -pg -o stromralley_p ${C_SRC} -O0 -g
	./stromralley_p ../Beispiele/generated/5x5_35.txt
	sleep 1
	gprof stromralley_p > prof.txt
	gprof stromralley_p | ./gprof2dot -s | dot -Tpng -o new.png

release:
	g++ -o stromralley ${C_SRC} -O3
