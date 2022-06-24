SUB Main
  DIM A(100)
  INPUT "Array size: ", n
  PRINT "Input array"
  FOR i = 1 TO n + 1
    INPUT A(i)
  END FOR

  ' бубле сортинг
  FOR i = 0 TO n
    FOR j = 2 TO n - i + 1
      IF A(j) < A(j - 1) THEN
        LET tmp = A(j)
        LET A(j) = A(j - 1)
        LET A(j - 1) = tmp
      END IF
    END FOR
  END FOR

  FOR i = 1 TO n + 1
    PRINT A(i)
  END FOR
END SUB
