CC      = cc
CFLAGS  = -O2 -Wall

PYTHON  = python3
PYINC   = $(shell $(PYTHON) -c "import sysconfig; print(sysconfig.get_path('include'))")
PYEXT   = $(shell $(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

.PHONY: all module bench clean

all: module bench

# Python extension — built in-place as cpuspeed<EXT_SUFFIX>
module:
	$(CC) $(CFLAGS) -shared -fPIC -I$(PYINC) \
	    -o cpuspeed$(PYEXT) cpuspeed.c -lpthread
	@echo "Built cpuspeed$(PYEXT)"

# Standalone C benchmark
bench: bench.c
	$(CC) $(CFLAGS) -o bench bench.c -lpthread
	@echo "Built ./bench"

clean:
	rm -f bench cpuspeed*.so cpuspeed*.pyd
	rm -rf build __pycache__
