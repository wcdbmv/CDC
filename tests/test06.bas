'
SUB Main
  CALL Print1
  CALL Print2 3.1415
  CALL Print3 777, "Ok!"
END SUB

SUB Print1
  PRINT "Hi!"
END SUB

SUB Print2(x)
  PRINT x
END SUB

SUB Print3(x, y$)
  PRINT x
  PRINT y$
END SUB
