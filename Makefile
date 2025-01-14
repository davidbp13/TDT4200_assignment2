
DEPS 	= bitmap.h

all: main bonus1 bonus2

main : main.o bitmap.o
	mpicc main.o bitmap.o -o main 
	
bonus1 : bonus1.o bitmap.o
	mpicc bonus1.o bitmap.o -o bonus1 
	
bonus2 : bonus2.o bitmap.o
	mpicc bonus2.o bitmap.o -o bonus2

# % matches anything, $< refers to the left hand side contents and $@ refers to the LHS
#Common header as dependency to force recompilation if it is modified
%.o : %.c $(DEPS)
	mpicc -c $< -o $@

phony : clean
clean: 
	rm -f *.o
