from typing import List
from copy import deepcopy
from queue import Queue
from inspect import isclass
from collections import defaultdict
import heapq
import math
import sys

class Operation(object):
    def __init__(self, operands: List):
        self.parent = None
        self.operands = operands
        for operand in operands:
            if isinstance(operand, Operation):
                operand.parent = self
    
    def calc(self):
        pass

class Digit(Operation):
    def __init__(self, num):
        super().__init__([num])

    def calc(self):
        return self.operands[0]

    def __str__(self):
        return str(self.operands[0])

    def __eq__(self, other):
        return type(self) == type(other) and self.operands[0] == other.operands[0]

class Number(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self): 
        numStr = ""
        for op in self.operands:
            numStr += str(op.calc())
        return int(numStr)
        
    def __str__(self):
        numStr = ""
        for op in self.operands:
            numStr += str(op.calc())
        return numStr

    def __eq__(self, other):
        return type(self) == type(other) and self.calc() == other.calc()

class Addition(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()
        op2 = self.operands[1].calc()

        try:
            if math.isnan(op1) or math.isnan(op2):
                return float('nan')
        except OverflowError:
            return float('nan')

        result = op1 + op2

        if result > int(sys.float_info.max):
            return float('nan')

        return result

    def __str__(self):
        return "(" + str(self.operands[0]) + "+" + str(self.operands[1]) + ")"

    def __eq__(self, other):
        return type(self) == type(other) and ((self.operands[0] == other.operands[0] and self.operands[1] == other.operands[1]) or (self.operands[0] == other.operands[1] and self.operands[1] == other.operands[0]))

class Subtraction(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()
        op2 = self.operands[1].calc()

        try:
            if math.isnan(op1) or math.isnan(op2):
                return float('nan')
        except OverflowError:
            return float('nan')

        result = op1 - op2

        if result > int(sys.float_info.max):
            return float('nan')

        return result

    def __str__(self):
        return "(" + str(self.operands[0]) + "-" + str(self.operands[1]) + ")"

    def __eq__(self, other):
        return type(self) == type(other) and self.operands[0] == other.operands[0] and self.operands[1] == other.operands[1]

class Multiplication(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()
        op2 = self.operands[1].calc()

        try:
            if math.isnan(op1) or math.isnan(op2):
                return float('nan')
        except OverflowError:
            return float('nan')

        result = op1 * op2

        if result > int(sys.float_info.max):
            return float('nan')

        return result
        
    def __str__(self):
        return "(" + str(self.operands[0]) + "*" + str(self.operands[1]) + ")"

    def __eq__(self, other):
        return type(self) == type(other) and ((self.operands[0] == other.operands[0] and self.operands[1] == other.operands[1]) or (self.operands[0] == other.operands[1] and self.operands[1] == other.operands[0]))

class Division(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()
        op2 = self.operands[1].calc()

        try:
            if math.isnan(op1) or math.isnan(op2):
                return float('nan')
        except OverflowError:
            return float('nan')

        if op2 == 0:
            return float('nan')

        result = op1 / op2
        if not math.isclose(result, round(result)):
            return float('nan')

        if result > int(sys.float_info.max):
            return float('nan')

        return int(round(result))
        
    def __str__(self):
        return "(" + str(self.operands[0]) + "/" + str(self.operands[1]) + ")"

    def __eq__(self, other):
        return type(self) == type(other) and self.operands[0] == other.operands[0] and self.operands[1] == other.operands[1]

class Exponentiation(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()
        op2 = self.operands[1].calc()

        try:
            if math.isnan(op1) or math.isnan(op2):
                return float('nan')
        except OverflowError:
            return float('nan')

        if op2 > 2000 or (op1 == 0 and op2 < 0):
            return float('nan')

        try:
            result = op1 ** op2
        except OverflowError:
            return float('nan')

        if isinstance(result, complex):
            return float('nan')

        if result > int(sys.float_info.max):
            return float('nan')

        return result

    def __str__(self):
        return "(" + str(self.operands[0]) + "^" + str(self.operands[1]) + ")"

    def __eq__(self, other):
        return type(self) == type(other) and self.operands[0] == other.operands[0] and self.operands[1] == other.operands[1]

class Factorial(Operation):
    def __init__(self, operands: List[Operation]):
        super().__init__(operands)

    def calc(self):
        op1 = self.operands[0].calc()

        if op1 < 3 and type(self.operands[0]) == Factorial:
            return float('nan')

        try:
            if math.isnan(op1) or op1 > 100 or op1 < 0:
                return float('nan')
        except OverflowError:
            return float('nan')

        if not float(op1).is_integer():
            return float('nan')

        result = math.factorial(op1)

        if result > int(sys.float_info.max):
            return float('nan')
        return result

    def __str__(self):
        return "(" + str(self.operands[0]) + "!)"

    def __eq__(self, other):
        return type(self) == type(other) and self.operands[0] == other.operands[0]

def getAllOperations(operation):
    lst = [operation]

    if type(operation) == Digit:
        return lst

    for operand in operation.operands:
        lst.extend(getAllOperations(operand))

    return lst

def replaceOperation(index, operations, opType, digit, opCount = 2):
    parent = operations[index].parent

    if parent is not None:
        for i, op in enumerate(parent.operands):
            if op is operations[index]:
                operation = deepcopy(operations[index])
                lst = [operation]
                lst.extend([Number([Digit(digit)]) for x in range(opCount - 1)])
                parent.operands[i] = opType(lst)
                break
    else:
        lst = [operations[index]]
        lst.extend([Number([Digit(digit)]) for x in range(opCount - 1)])
        operations = [opType(lst)]

    return operations[0]

def tryMinConfig(num, digit, maxDigitCnt = -1, useAll = False):
    workers = Queue()
    eq = Number([Digit(digit)])
    workers.put(eq)

    equations = defaultdict(int)
    best = None
    bestDigitCnt = -1

    while not workers.empty():
        worker = workers.get()

        operations = getAllOperations(worker)

        numDigit = sum(1 for op in operations if type(op) == Digit)
        if bestDigitCnt > 0 and numDigit >= bestDigitCnt:
            continue

        result = worker.calc()

        if equations[result] != 0 and equations[result] <= numDigit:
            continue

        try:
            float(result)
        except OverflowError:
            continue

        if math.isnan(result):
            continue

        if worker.calc() == num and (best is None or numDigit < bestDigitCnt):
            best = worker
            bestDigitCnt = numDigit
            print("Record (Best) =", bestDigitCnt, worker)
            continue

        print("Record (Best) =", bestDigitCnt, worker)

        for i, operation in enumerate(operations):
            if type(operation) == Number and (maxDigitCnt == -1 or numDigit < maxDigitCnt):
                newDigit = deepcopy(operations)
                newDigit[i].operands.append(Digit(digit))
                newDigit[i].operands[-1].parent = newDigit[i]
                workers.put(newDigit[0])

            if type(operation) != Digit:
                if (maxDigitCnt == -1 or numDigit < maxDigitCnt) and (bestDigitCnt == -1 or numDigit < bestDigitCnt - 1):
                    if useAll:
                        exp = replaceOperation(i, deepcopy(operations), Exponentiation, digit)
                        workers.put(exp)
                    
                    add = replaceOperation(i, deepcopy(operations), Addition, digit)
                    workers.put(add)
                    
                    sub = replaceOperation(i, deepcopy(operations), Subtraction, digit)
                    workers.put(sub)
                    
                    mul = replaceOperation(i, deepcopy(operations), Multiplication, digit)
                    workers.put(mul)
                    
                    div = replaceOperation(i, deepcopy(operations), Division, digit)
                    workers.put(div)

                if useAll and operation.calc() > 2:
                    fac = replaceOperation(i, deepcopy(operations), Factorial, digit, 1)
                    workers.put(fac)

        equations[result] = numDigit

    return [best, bestDigitCnt]

sol = tryMinConfig(2019, 3, -1, True)
if sol[0] != None:
	print(sol[0], "=", sol[0].calc())