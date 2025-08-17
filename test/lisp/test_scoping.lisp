
(def x 1)
(def y 2)

; Expected: 1
(print-debug x)
; Expected: 2
(print-debug y)


(let ((x 10)) ; Shadow global x
  ; Expected: 10
  (print-debug x)
  ; Expected: 2
  (print-debug y)

  (let ((y 20) (x 100)) ; Shadow outer x and global y
    ; Expected: 120
    (print-debug (+ x y))
  )

  ; Expected: 10
  (print-debug x)
)

; Expected: 1
(print-debug x)