import sys
import json


class Value:
    @classmethod
    def constructor1(cls, instr, var2num):
        value = Value()
        value.op = instr["op"]
        value.args = []
        value.isValue = "dest" in instr
        if instr["op"] == "const":
            value.val = instr["value"]
            value.type = instr["type"]
        elif instr["op"] == "id":
            value.val = instr["args"][0]
            value.type = instr["type"]
        else:
            if "args" in instr:
                value.args = [var2num[x] for x in instr["args"]]
            else:
                value.isValue = False
        value.args.sort()
        return value

    @classmethod
    def constructor2(cls, name):
        value = Value()
        value.op = "id"
        value.val = name
        value.type = "dummy"
        return value
    
    @classmethod
    def constructor3(cls, name, type):
        value = Value()
        value.op = "id"
        value.val = name
        value.type = type
        return value

    def __init__(self):
        self.op = None
        self.args = []
        self.isValue = True

    def __eq__(self, other):
        if self.op != other.op:
            return False
        return self.args == other.args

    def __hash__(self):
        if self.args:
            return hash((self.op, *self.args))
        return hash((self.op, self.val, self.type))

    def __repr__(self):
        res = "(" + self.op
        if self.args:
            for a in self.args:
                res += ", " + str(a)
        else:
            res += " " + str(self.val)
        return res + ")"


class Table():
    def __init__(self):
        self.valToRow = {}
        self.rowToCanon = {}

    def insert(self, row, value, canon):
        self.valToRow[value] = row
        self.rowToCanon[row] = canon

    def get(self, value):
        return (self.valToRow[value], self.rowToCanon[self.valToRow[value]])

    def getFromRow(self, value):
        return self.rowToCanon[value]

    def inTable(self, value):
        return value in self.valToRow
    
    def remove(self, value):
        row = self.valToRow[value]
        del self.rowToCanon[row]
        del self.valToRow[value]


def lvn(block, args): 
    valId = 1
    table = Table()
    var2num = {}

    for arg in args:
        var2num[arg["name"]] = valId 
        table.insert(valId, Value.constructor2(arg["name"]), arg["name"])
        valId += 1

    removeInstrs = set()

    redef = [False] * len(block)
    redefed = set()
    for i, instr in reversed(list(enumerate(block))):
        if "dest" in instr and instr["dest"] in redefed:
            redef[i] = True
        if "dest" in instr:
            redefed.add(instr["dest"])

    for i, instr in enumerate(block):
        if "label" in instr:
            continue
        
        if "args" in instr:
            for arg in instr["args"]:
                if arg not in var2num:
                    var2num[arg] = valId 
                    table.insert(valId, Value.constructor2(arg), arg)
                    valId += 1
        if "args" in instr and instr["op"] != "const":
            instr["args"] = [table.getFromRow(
                var2num[a]) for a in instr["args"]]
                
        value = Value.constructor1(instr, var2num)
        if value.isValue:
            if instr["op"] == "call":
                continue
            if table.inTable(value):
                num, var = table.get(value)
                removeInstrs.add(i)
            else:
                num = valId
                if redef[i]:
                    dest = "var" + str(valId)
                    var2num[instr["dest"]] = valId
                    instr["dest"] = dest
                else:
                    dest = instr["dest"]

                valId += 1
                
                destVal = Value.constructor3(dest, instr["type"])
                if table.inTable(destVal):
                    table.remove(destVal)
                table.insert(num, value, dest)

            var2num[instr["dest"]] = num
        else:
            if "args" in instr:
                instr["args"] = [table.getFromRow(
                    var2num[a]) for a in instr["args"]]

    newInstrs = []
    for i, instr in enumerate(block):
        if i not in removeInstrs:
            newInstrs.append(instr)
    block = newInstrs


def formBBs(instrs):
    def is_term(instr):
        return instr["op"] == "jmp" or instr["op"] == "br"

    res = []
    cur = []

    for instr in instrs:
        if "label" in instr:
            res.append(cur)
            cur = [instr]
            continue
        cur.append(instr)
        if is_term(instr):
            res.append(cur)
            cur = []
    if cur:
        res.append(cur)
    return res


if __name__ == "__main__":
    data = json.load(sys.stdin)

    for function in data["functions"]:
        blocks = formBBs(function["instrs"])
        for block in blocks:
            if "args" in function:
                lvn(block, function["args"])
            else:
                lvn(block, [])
        function["instrs"] = [x for sublist in blocks for x in sublist]

    json.dump(data, sys.stdout, indent=2)
