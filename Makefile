#Comet
#MKLPATH=/opt/intel/Compiler/11.1/072/mkl
MKLPATH=$MKL_ROOT
CC=icpc
MPICC = mpicc
OPENFLAGS= -fopenmp
MOIFLAGS = -parallel
PTFLAGS = -lpthread
CFLAGS = -I$(MKLPATH)/include $(OPENFLAGS)
MKLFLAGS=  -I$MKL_ROOT/include ${MKL_ROOT}/lib/intel64/libmkl_scalapack_lp64.a -Wl,--start-group ${MKL_ROOT}/lib/intel64/libmkl_intel_lp64.a ${MKL_ROOT}/lib/intel64/libmkl_core.a ${MKL_ROOT}/lib/intel64/libmkl_sequential.a -Wl,--end-group ${MKL_ROOT}/lib/intel64/libmkl_blacs_intelmpi_lp64.a 
LDFLAGS = $(MKLFLAGS) -lpthread -lm $(OPENFLAGS)

all:	benchmark-naive benchmark-blocked benchmark-blas benchmark-tp
benchmark-naive: benchmark.o dgemm-naive.o
	$(CC) -o $@ $^ $(LDFLAGS)
benchmark-tp: benchmark-tp.o
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f benchmark-naive *.o benchmark-tp *.o
