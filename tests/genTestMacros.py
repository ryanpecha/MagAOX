#!/bin/env python3

# Generates a C++ header of XWCTEST_IF_<NAME>(line) macros from a plain text list of
# XWCTEST_<NAME> fault injection names.
#
# Each generated macro has two forms. When a test translation unit defines the matching
# XWCTEST_<NAME> before it includes the production header, the macro expands to the given
# line inside a block, marked with LCOV_EXCL_LINE so coverage ignores it. Otherwise the
# macro expands to an empty `do {} while(0)`. Production builds never define these names.
# So the macro call sites in production code are always inert there.
#
# Usage:
#   genTestMacros.py --names <names-file> --out <output-header>
#
# The names file is plain text with one XWCTEST_<NAME> per line. Blank lines and lines
# starting with '#' are ignored. For each name the text up to and including the first '_'
# is stripped off. For example XWCTEST_FOO_BAR becomes the callable XWCTEST_IF_FOO_BAR,
# guarded by #ifdef XWCTEST_FOO_BAR.
#
# The header is rendered from xwcTestMacroTemplate.jinja2 in this directory.

import os
import sys
import getopt
import jinja2

def versionAsNumber(major, minor):
    return (major * 1000 + minor)

def usage():
    print("usage: genTestMacros.py --names <names-file> --out <output-header>")

def readNames(namesPath):
    # Read the names file. Skip blank lines and '#' comment lines.
    names = []
    with open(namesPath) as infile:
        for line in infile:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            names.append(line)
    return names

def main():
    if versionAsNumber(sys.version_info[0], sys.version_info[1]) < versionAsNumber(3, 9):
        print("Error: Python version must be >= 3.9")
        sys.exit(1)

    try:
        opts, _ = getopt.getopt(sys.argv[1:], "", ["names=", "out=", "help"])
    except getopt.GetoptError as e:
        print(f"Error: {e}")
        usage()
        sys.exit(1)

    namesPath = None
    outPath = None
    for opt, val in opts:
        if opt == "--names":
            namesPath = val
        elif opt == "--out":
            outPath = val
        elif opt == "--help":
            usage()
            sys.exit(0)

    if namesPath is None or outPath is None:
        usage()
        sys.exit(1)

    gXwcTests = readNames(namesPath)

    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(searchpath=os.path.dirname(__file__))
    )
    # Drop the newline after each block tag and the whitespace before it.
    # This keeps the generated header free of stray blank lines from the template.
    env.trim_blocks = True
    env.lstrip_blocks = True

    template = env.get_template("xwcTestMacroTemplate.jinja2")

    # Strip the leading XWCTEST_ prefix. The template adds the XWCTEST_ and XWCTEST_IF_ prefixes back.
    templateInfo = dict()
    templateInfo["xwcTestNames"] = [x[(x.find("_") + 1):] for x in gXwcTests]

    renderedHeader = template.render(templateInfo)

    with open(outPath, "w") as outfile:
        print(renderedHeader, file=outfile)

if __name__ == "__main__":
    main()
