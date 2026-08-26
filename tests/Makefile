############################################################
#                makefile for MagAOX tests                 #
#                                                          #
# Add tests to tests.list and build with `make`            #
#   Use `make -jN` to build the tests in parallel.         #
#                                                          #
# notes:                                                   #
#   -- you do not need to edit anything else in this file  #
#   -- if the test requires setuid, add a conditional to   #
#      Makefile.one                                        #
############################################################

# Read tests.list once at parse time instead of with a shell backtick in each recipe.
# Blank lines are dropped so they do not become empty goals.
# The sort removes duplicates and gives a stable build order.
TESTOBJS := $(sort $(shell grep -v '^[[:space:]]*$$' tests.list))

##################################################
## Should not need to edit from here on:
##################################################

# One phony goal per test. This lets `make -jN` build the tests concurrently.
# The old version ran a serial shell loop, which could not be parallelized.
build_goals := $(addprefix build@,$(TESTOBJS))

.PHONY: all prep $(build_goals)
all: $(build_goals)

# Build the shared testMain.o once, before any per-test build starts.
# Without this, concurrent jobs would race to compile the same file.
# The git version header has a similar race. It is made safe in Makefile.one.
prep:
	@$(MAKE) --no-print-directory -f Makefile.one testMain.o

# Build one test through Makefile.one. The order-only dependency on prep waits for testMain.o.
# The leading dash and the echo turn a failed build into a SKIP message and let the other
# tests continue. The old shell loop also did not stop on a failed test build, so this keeps
# that behavior.
$(build_goals): build@%: | prep
	-@$(MAKE) --no-print-directory -f Makefile.one t=$* || echo ">> SKIP (build failed): $*"

# Same loop as before on one line. It still stops at the first failing test.
.PHONY: test
test: all
	@for test in $(TESTOBJS); do ./$$test || exit 1; done

# Same loop as before on one line.
.PHONY: clean
clean:
	@echo cleaning . . .
	@for test in $(TESTOBJS); do $(MAKE) --no-print-directory -f Makefile.one t=$$test clean; done

.PHONY: realclean
realclean: clean
	@rm -f testMain.o
	@rm -f test_stderr.txt

# $(MAKE) replaces ${MAKE} here and in coverage_clean. They are the same variable.
# This is only for one consistent style in the file.
.PHONY: coverage
coverage: coverage_clean
	$(MAKE) all COVERAGE=1

coverage_clean:
	find .. -path '*/tests/*.gcno' -delete
	find .. -path '*/tests/*.gcda' -delete
	find .. -path '*/tests/*.gcov' -delete
	@rm -f testMain.gcno
	@rm -f testMain.gcda
	$(MAKE) realclean COVERAGE=1
