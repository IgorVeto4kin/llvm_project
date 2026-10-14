all: app

start.ll: start.c
	clang -emit-llvm -S $^
sim.ll: sim.c
	clang -emit-llvm -S $^
app.ll: app.c
	clang -emit-llvm -S $^
full.bc: start.ll sim.ll app.ll
	llvm-link $^ -o full.bc





app-opt.ll: app.c
	clang -emit-llvm -S $^ -o $@ -O2
app.ll: app.c
	clang -emit-llvm -S $^
app-opt.ll: app.c 
	clang -emit-llvm -S $^ -o $@ -O2



app: app.ll start.c sim.c
	clang $^ -lSDL2 -o $@
app-opt: app-opt.ll start.c sim.c
	clang $^ -lSDL2 -o $@ -O2


LLVM_FLAGS = $(shell llvm-config --cppflags --ldflags --libs)

ir_gen: sim.c IRGen/app_ir_gen.cpp
	clang++ $(LLVM_FLAGS) $^ -lSDL2 -o $@

asm2ir: sim.c IRGen/app_asm_IRgen_1.cpp
	clang++ $(LLVM_FLAGS) $^ -lSDL2 -o $@

asm2fullir: sim.c IRGen/app_asm_IRgen_2.cpp
	clang++ $(LLVM_FLAGS) $^ -lSDL2 -o $@

clean:
	rm app app-opt app2 app2-opt app3 app4 app4-opt *.ll full.bc ir_gen asm2ir asm2fullir

.PHONY: all clean
