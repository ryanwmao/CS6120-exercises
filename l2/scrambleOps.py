import argparse
import sys
import json
import random

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=argparse.FileType("r"), default=sys.stdin)
    args = parser.parse_args()

    file = json.load(args.input)

    ops = ["add", "sub", "mul", "div"]

    for function in file["funcitons"]:
        for instruction in function["instrs"]:
            if "op" in instruction and instruction["op"] in ops:
                instruction["op"] = random.choice(ops)

    print(file)
