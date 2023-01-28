pp = &H378
OPEN "romout.txt" FOR OUTPUT AS 1
OUT pp + 1, 16
inic:
a = (INP(pp + 1))
IF a = 223 THEN GOTO NO
a = (INP(pp + 0))
PRINT a
PRINT #1, a
IF INKEY$ <> "" THEN END
NO:
GOTO inic

