############################################################
#                makefile for MagAOX tests                 #
#                                                          #
# Add tests to tests.list and build with `make`            #
#   - `make -jN` builds the tests in parallel              #
#                                                          #
# notes:                                                   #
#   -- you do not need to edit anything else in this file  #
#   -- if the test requires setuid, add a conditional to   #
#      Makefile.one                                        #
############################################################

TESTOBJS := $(sort $(shell grep -v '^[[:space:]]*$$' tests.list))

##################################################
## Should not need to edit from here on:
##################################################

# One phony goal per test so `make -jN` builds them concurrently.
build_goals := $(addprefix build@,$(TESTOBJS))

.PHONY: all prep $(build_goals)
all: $(build_goals)

# Build the shared testMain.o once, before any parallel per-test build starts, so concurrent
# jobs don't race to compile it. (The git-version header is made race-safe in Makefile.one.)
prep:
	@$(MAKE) --no-print-directory -f Makefile.one testMain.o

$(build_goals): build@%: | prep
	-@$(MAKE) --no-print-directory -f Makefile.one t=$* || echo ">> SKIP (build failed): $*"

.PHONY: test
test: all
	@for test in $(TESTOBJS); do ./$$test || exit 1; done

.PHONY: clean
clean:
	@echo cleaning . . .
	@for test in $(TESTOBJS); do $(MAKE) --no-print-directory -f Makefile.one t=$$test clean; done

.PHONY: realclean
realclean: clean
	@rm -f testMain.o
	@rm -f test_stderr.txt

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
