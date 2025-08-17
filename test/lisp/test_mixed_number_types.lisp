; int + int -> int
; Expected: 15
(print-debug (+ 10 5))

; float + float -> float
; Expected: 11.000000
(print-debug (+ 10.5 0.5))

; int + float -> float
; Expected: 15.500000
(print-debug (+ 10 5.5))

; float + int -> float
; Expected: 15.500000
(print-debug (+ 5.5 10))

; (- 10.0 (+ 2 3.5)) -> (- 10.0 5.5) -> 4.5
; Expected: 4.500000
(print-debug (- 10.0 (+ 2 3.5)))

(let ((my-int 100) (my-float 0.5))
  ; Expected: 99.500000
  (print-debug (- my-int my-float))
)