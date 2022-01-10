This repo relizes parsing csv file in python calling C function or fork C subprocess.

You can specify quote character, escape character and eol.
Both ways of parsing csv is faster than python build-in functions. 

test result of split.py:

```
split file in csv:
Processed 40000000 lines.
cost time: 27.77s

split file in open readlines:
Processed 40000000 lines.
read time: 8.2s
calcute len time: 0.0s
cost time: 8.2s

split file in c function:
Processed 40000000 lines.
total cost time: 7.11s

split in fork c sub process
processed chunk: 14653
total cost time: 3.7s
```