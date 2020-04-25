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
        return type(self) == type(other) and str(self.calc()) == str(other.calc())


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

class WorkThread(threading.Thread):
   def __init__(self, queue, equations, best, approx, num, digit, maxLen):
      threading.Thread.__init__(self)
      self.queue = queue
      self.equations = equations
      self.best = best
      self.approx = approx
      self.num = num
      self.digit = digit
      self.maxLen = maxLen
      self.isIdle = False

   def run(self):
      workQueue(self)

exitFlag = 0
queueLock = threading.Lock()
equationLock = threading.Lock()
bestLock = threading.Lock()
approxLock = threading.Lock()

def workQueue(thread):
    while not exitFlag:
        queueLock.acquire()
        if not thread.queue.empty():
            thread.isIdle = False
            worker = thread.queue.get()
            queueLock.release()

            equationLock.acquire()
            if worker in thread.equations:
                equationLock.release()
                continue
            equationLock.release()

            operations = getAllOperations(worker)
            numDigit = sum(1 for op in operations if type(op) == Digit)

            if thread.best is not None:
                if thread.best[1] > 0 and numDigit >= thread.best[1]:
                    continue

            result = worker.calc()
            try:
                float(result)
            except OverflowError:
                continue

            if math.isnan(result):
                continue

            approxLock.acquire()
            if abs(thread.num - result) < abs(thread.num - thread.approx[1]):
                thread.approx = [worker, result, numDigit]
            elif abs(thread.num - result) == abs(thread.num - thread.approx[1]):
                if numDigit < thread.approx[2]:
                    thread.approx = [worker, result, numDigit]
            approxLock.release()

            bestLock.acquire()
            if worker.calc() == thread.num and (thread.best is None or numDigit < thread.best[1]):
                thread.best = [worker, numDigit]
                print("Record =", thread.best[1], "Current: ", worker, "=", result)
                bestLock.release()
                continue
            bestLock.release()

            print("Current: ", worker, "=", result)

            queueLock.acquire()
            for i, operation in enumerate(operations):
                if type(operation) == Number and numDigit < thread.maxLen:
                    newDigit = deepcopy(operations)
                    newDigit[i].operands.append(Digit(thread.digit))
                    newDigit[i].operands[-1].parent = newDigit[i]
                    thread.queue.put(newDigit[0], abs(thread.num - newDigit[0].calc()))

                if type(operation) != Digit:
                    if numDigit < thread.maxLen and (thread.best is None or numDigit < thread.best[1] - 1):
                        exp = replaceOperation(i, deepcopy(operations), Exponentiation, thread.digit)
                        thread.queue.put(exp)
                        
                        add = replaceOperation(i, deepcopy(operations), Addition, thread.digit)
                        thread.queue.put(add)
                        
                        sub = replaceOperation(i, deepcopy(operations), Subtraction, thread.digit)
                        thread.queue.put(sub)
                        
                        mul = replaceOperation(i, deepcopy(operations), Multiplication, thread.digit)
                        thread.queue.put(mul)
                        
                        div = replaceOperation(i, deepcopy(operations), Division, thread.digit)
                        thread.queue.put(div)
                
                if operation.calc() > 2:
                    fac = replaceOperation(i, deepcopy(operations), Factorial, thread.digit, 1)
                    thread.queue.put(fac)
            queueLock.release()
        
            equationLock.acquire()
            thread.equations.append(worker)
            equationLock.release()
        else:
            queueLock.release()
            thread.isIdle = True

def solveFor(num, digit, maxLen):
    global exitFlag

    eq = Number([Digit(digit)])
    equations = []
    best = None
    approx = [eq, digit, 1]

    queue = Queue()

    threads = []
    for _ in range(16):
        thread = WorkThread(queue, equations, best, approx, num, digit, maxLen)
        thread.start()
        threads.append(thread)

    queueLock.acquire()
    queue.put(eq)
    queueLock.release()

    time.sleep(0.1)

    while all(not t.isIdle for t in threads) or not queue.empty():
        pass

    exitFlag = 1
    for t in threads:
        t.join()
    exitFlag = 0

    return [best, approx]

solutions = []

for i in range(1, 10):
    solutions.append(solveFor(2019, i, 3))

for s in solutions:
    if s[0] == None:
        print(s[1][0], "=", s[1][1])
    else:
        print(s[0][0], "=", s[0][0].calc())