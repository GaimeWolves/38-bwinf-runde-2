from typing import List
from copy import deepcopy
from queue import Queue
from inspect import isclass
import heapq
import math
import sys
import threading
import time

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

        if op1 == 1 and type(self.operands[0]) == Factorial:
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

def findSolution(num, digit):
    workers = PriorityQueue()
    eq = Number([Digit(digit)])
    workers.push(eq, abs(num - eq.calc()))

    equations = []

    while not workers.empty():
        worker = workers.pop()

        if worker in equations:
            continue

        operations = getAllOperations(worker)
        numDigit = sum(1 for op in operations if type(op) == Digit)
        result = worker.calc()

        try:
            float(result)
        except OverflowError:
            continue

        if math.isnan(result):
            continue

        if worker.calc() == num:
            return worker

        c = 1

        print("Current: ", worker, "=", result)

        for i, operation in enumerate(operations):
            if type(operation) == Number:
                newDigit = deepcopy(operations)
                newDigit[i].operands.append(Digit(digit))
                newDigit[i].operands[-1].parent = newDigit[i]
                workers.push(newDigit[0], abs(num - newDigit[0].calc()) * (c ** numDigit))

            if type(operation) != Digit:
                exp = replaceOperation(i, deepcopy(operations), Exponentiation, digit)
                add = replaceOperation(i, deepcopy(operations), Addition, digit)
                sub = replaceOperation(i, deepcopy(operations), Subtraction, digit)
                mul = replaceOperation(i, deepcopy(operations), Multiplication, digit)
                div = replaceOperation(i, deepcopy(operations), Division, digit)

                try:
                    if not math.isnan(add.calc()):
                        workers.push(add, abs(num - add.calc()) * (c ** numDigit))
                except:
                    pass

                try:
                    if not math.isnan(sub.calc()):
                        workers.push(sub, abs(num - sub.calc()) * (c ** numDigit))
                except:
                    pass

                try:
                    if not math.isnan(mul.calc()):
                        workers.push(mul, abs(num - mul.calc()) * (c ** numDigit))
                except:
                    pass

                try:
                    if not math.isnan(div.calc()):
                        workers.push(div, abs(num - div.calc()) * (c ** numDigit))
                except:
                    pass

                try:
                    if not math.isnan(exp.calc()):
                        workers.push(exp, abs(num - exp.calc()) * (c ** numDigit))
                except:
                    pass

                fac = replaceOperation(i, deepcopy(operations), Factorial, digit, 1)

                try:
                    if not math.isnan(fac.calc()):
                        workers.push(fac, abs(num - fac.calc()) * (c ** numDigit))
                except:
                    pass

        equations.append(worker)

    return None

num = 0
digit = 0
threads = []
best = None

class EquationThread(threading.Thread):
   def __init__(self, eq):
      threading.Thread.__init__(self)
      self.eq = eq
   def run(self):
      solveForHelper(self.eq)

def solveForHelper(eq):
    global num
    global digit
    global threads
    global best

    print(eq)

    res = eq.calc()

    try:
        if math.isnan(res):
            return
    except:
        return

    ops = getAllOperations(eq)
    numDigits = sum(1 for x in ops if type(x) == Digit)

    if res == num and numDigits < best[1] - 1:
        best = [eq, numDigits]
        return

    for i, op in enumerate(ops):
        if numDigits < best[1] and type(op) == Number:
            newDigit = deepcopy(ops)
            newDigit[i].operands.append(Digit(digit))
            newDigit[i].operands[-1].parent = newDigit[i]
            thread = EquationThread(newDigit[0])
            threads.append(thread)
            thread.start()

        if type(op) != Digit:
            for opType in [Exponentiation, Addition, Subtraction, Multiplication, Division, Factorial]:
                if opType == Factorial or numDigits < best[1] - 1:
                    newOp = replaceOperation(i, deepcopy(ops), opType, digit, 1 if opType == Factorial else 2)
                    thread = EquationThread(newOp)
                    threads.append(thread)
                    thread.start()

def solveFor(_num, _digit, _best):
    eq = Number([Digit(_digit)])

    global num
    global digit
    global threads
    global best

    if _num == 0:
        return None

    threads = []
    best = _best
    num = _num
    digit = _digit

    threads.append(EquationThread(eq))
    threads[0].start()

    for t in threads:
        t.join()

#sol = findSolution(2019, 3)
#print(sol, '=', sol.calc())
#ops = getAllOperations(sol)
#numDigits = sum(1 for x in ops if type(x) == Digit)
#solveFor(2019, 3, [sol, numDigits])
#print(best[0], '=', best[0].calc())

for i in range(1, 10):
    sol = solveFor(2019, i, [None, 4])
    #print(sol, '=', sol.calc())

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

