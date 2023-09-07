import sys
import json


class Value:
   def __init__(self, instr, var2num):
      self.op = instr["op"]
      self.args = []
      self.isValue = "dest" in instr
      if instr["op"] == "const":
        self.val = instr["value"]
      else:
        if instr["args"]:
          self.args = [var2num[x] for x in instr["args"]]
        else:
          self.isValue = False
      self.args.sort()

   def __eq__(self, other):
      if self.op != other.op:
         return False
      return self.args == other.args

   def __hash__(self):
      if self.args:
        return hash((self.op, *self.args))
      return hash((self.op, self.val))

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


def lvn(function):
    valId = 1
    table = Table()
    var2num = {}

    removeInstrs = set()

    redef = [False] * len(function["instrs"])
    redefed = set()
    for i, instr in reversed(enumerate(function["instrs"])):
       if "dest" in instr and instr["dest"] in redefed:
          redef[i] = True
       if "dest" in instr:
          redefed.add(instr["dest"])

    for i, instr in enumerate(function["instrs"]):
        value = Value(instr, var2num)
        if value.isValue:
          if table.inTable(value):
              num, var = table.get(value)
              removeInstrs.add(i)
          else:
             num = valId

             if redef[i]:
               dest = "var" + valId
               instr["dest"] = dest
             else:
               dest = instr["dest"]

             valId += 1

             table.insert(num, value, dest)

             if "args" in instr and instr["op"] != "const":
              instr["args"] = [table.getFromRow(
                  var2num[a]) for a in instr["args"]]

          var2num[instr["dest"]] = num
        else:
           if "args" in instr:
              instr["args"] = [table.getFromRow(
                  var2num[a]) for a in instr["args"]]

    newInstrs = []
    for i, instr in enumerate(function["instrs"]):
       if i not in removeInstrs:
          newInstrs.append(instr)
    function["instrs"] = newInstrs


if __name__ == "__main__":
    data = json.load(sys.stdin)

    for function in data["functions"]:
        lvn(function)

    json.dump(data, sys.stdout, indent=2)
