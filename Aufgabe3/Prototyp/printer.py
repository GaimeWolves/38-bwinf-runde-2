import os
from prettytable import from_csv

with open(os.sys.argv[1], newline='') as file:
    csv = from_csv(file)
    print(csv)