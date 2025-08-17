(def a 10)
(def b 20)

; Expected to print 100
(print-debug (if #t 100 999))

; Expected to print 200
(print-debug (if #f 999 200))

; Expected to print 300
(print-debug (if 1 300 999))

; Expected to print #f
(print-debug (if () 999 400))


; Expected to print 30
(print-debug (if a (+ a b) (- a b)))