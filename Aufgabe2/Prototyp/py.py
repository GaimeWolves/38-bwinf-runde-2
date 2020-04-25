from typing import List
from copy import deepcopy
from queue import Queue
from inspect import isclass
import heapq
import math
import sys

class PriorityQueue:
    def __init__(self):
        self._queue = []
        self._index = 0
 
    def push(self, item, priority):
        heapq.heappush(self._queue, (priority, self._index, item))
        self._index += 1
 
    def pop(self):
        return heapq.heappop(self._queue)[-1]

    def empty(self):
        return len(self._queue) == 0

    def count(self):
        return len(self._queue)

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


def solveFor(num, digit, maxDigitCnt):
    workers = PriorityQueue()
    eq = Number([Digit(digit)])
    workers.push(eq, abs(num - eq.calc()))

    equations = []
    best = None
    approx = [eq, eq.calc(), 1]
    bestDigitCnt = -1

    while not workers.empty():
        worker = workers.pop()

        if worker in equations:
            continue

        operations = getAllOperations(worker)

        numDigit = sum(1 for op in operations if type(op) == Digit)
        if bestDigitCnt > 0 and numDigit >= bestDigitCnt:
            continue

        result = worker.calc()

        try:
            float(result)
        except OverflowError:
            continue

        if math.isnan(result):
            continue

        if abs(num - result) < abs(num - approx[1]):
            approx = [worker, result, numDigit]
        elif abs(num - result) == abs(num - approx[1]):
            if numDigit < approx[2]:
                approx = [worker, result, numDigit]

        if worker.calc() == num and (best is None or numDigit < bestDigitCnt):
            best = worker
            bestDigitCnt = numDigit
            print("Record =", bestDigitCnt, "Current: ", worker, "=", result)
            continue

        print("Record =", bestDigitCnt, "Current: ", worker, "=", result)

        for i, operation in enumerate(operations):
            if type(operation) == Number and numDigit < maxDigitCnt:
                newDigit = deepcopy(operations)
                newDigit[i].operands.append(Digit(digit))
                newDigit[i].operands[-1].parent = newDigit[i]
                workers.push(newDigit[0], abs(num - newDigit[0].calc()))

            if type(operation) != Digit:
                if numDigit < maxDigitCnt and (bestDigitCnt == -1 or numDigit < bestDigitCnt - 1):
                    exp = replaceOperation(i, deepcopy(operations), Exponentiation, digit)
                    add = replaceOperation(i, deepcopy(operations), Addition, digit)
                    sub = replaceOperation(i, deepcopy(operations), Subtraction, digit)
                    mul = replaceOperation(i, deepcopy(operations), Multiplication, digit)
                    div = replaceOperation(i, deepcopy(operations), Division, digit)

                    try:
                        if not math.isnan(add.calc()):
                            workers.push(add, abs(num - add.calc()))
                    except:
                        pass

                    try:
                        if not math.isnan(sub.calc()):
                            workers.push(sub, abs(num - sub.calc()))
                    except:
                        pass

                    try:
                        if not math.isnan(mul.calc()):
                            workers.push(mul, abs(num - mul.calc()))
                    except:
                        pass

                    try:
                        if not math.isnan(div.calc()):
                            workers.push(div, abs(num - div.calc()))
                    except:
                        pass

                    try:
                        if not math.isnan(exp.calc()):
                            workers.push(exp, abs(num - exp.calc()))
                    except:
                        pass

                if operation.calc() > 2:
                    fac = replaceOperation(i, deepcopy(operations), Factorial, digit, 1)
                    try:
                        if not math.isnan(fac.calc()):
                            workers.push(fac, abs(num - fac.calc()))
                    except:
                        pass

        equations.append(worker)

    return [best, approx[0]]

solutions = []

for i in range(1, 10):
    solutions.append(solveFor(2019, i, 3))

for s in solutions:
    if s[0] == None:
        print(s[1], "=", s[1].calc())
    else:
        print(s[0], "=", s[0].calc())

print(solveFor(2019, 3, 5)[0])

#for i in range(1, 10):
#    sol = solveFor(2020, i)
#    print(sol, '=', sol.calc())
#
#for i in range(1, 10):
#    sol = solveFor(2030, i)
#    print(sol, '=', sol.calc())
#
#for i in range(1, 10):
#    sol = solveFor(2080, i)
#    print(sol, '=', sol.calc())
#
#for i in range(1, 10):
#    sol = solveFor(2980, i)
#    print(sol, '=', sol.calc())

