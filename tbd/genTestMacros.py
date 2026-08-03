#!/bin/env python3

import os
import sys
import glob
import re
import pathlib
import string
import random
import getopt
import jinja2

gXwcTests = [
    "XWCTEST_INDI_CALLBACK_VALIDATION",
    "XWCTEST_TELEMETER_LOGSTART",
    "XWCTEST_DOX_REF",
    "XWC_DMTIMINGS"
]

def versionAsNumber(major, minor):
    return (major * 1000 + minor)

def main():
    # check python version >= 3.9
    if (versionAsNumber(sys.version_info[0], sys.version_info[1]) < versionAsNumber(3,9)):
        print("Error: Python version must be >= 3.9")
        exit(0)


    # load template
    env = jinja2.Environment(
        loader = jinja2.FileSystemLoader(searchpath=os.path.dirname(__file__))
    )
    env.trim_blocks = True
    env.lstrip_blocks = True

    catchTemplate = env.get_template("appDevTestMacroTemplate.jinja2")

    templateInfo = dict()
    templateInfo["xwcTestNames"] = [ x[(x.find("_") + 1):] for x in gXwcTests ]

    # render
    renderedHeader = catchTemplate.render(templateInfo)

    # write generated file
    outPath = os.path.join(os.path.dirname(__file__), "testMacros.hpp")
    with open(outPath,"w") as outfile:
        print(renderedHeader,file=outfile)

if __name__ == "__main__":
    main()