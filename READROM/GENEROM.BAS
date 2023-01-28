reg& = 1
c = 1
OPEN "romout.txt" FOR INPUT AS 1
OPEN "7000h.rom" FOR BINARY ACCESS WRITE AS 2
WHILE NOT EOF(1)
LINE INPUT #1, a$
a = VAL(a$)
c = c + 1
REM IF a <> 10 THEN PRINT a, c: END
PRINT a,
LINE INPUT #1, a$
a = VAL(a$)
PRINT a, c
a$ = CHR$(a)
PUT #2, reg&, a$
reg& = reg& + 1
WEND

