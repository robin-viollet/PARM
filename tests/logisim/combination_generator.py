import itertools
lst = list(itertools.product([0, 1], repeat=6))
for item in lst:
    combination = ""
    for subitem in item:
        combination += str(subitem)
    print(combination)