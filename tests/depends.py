#!/usr/bin/env python3
import toml
import sys

def prefix(x: str) -> str:
    return sys.argv[1] + x

with open(sys.argv[2]) as c:
    d = toml.load(c)
    if "testing" in d and "depends" in d["testing"]:
        d = d["testing"]["depends"]
        if type(d) == str:
            print(prefix(d), end="")
        elif type(d) == list:
            print(";".join(map(prefix, sorted(d))), end="")
        else:
            exit(1)
