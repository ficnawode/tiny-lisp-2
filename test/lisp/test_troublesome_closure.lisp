;; Test 9 
;; Closure with many free variables (more than registers)
;; This tests the creation of a closure where the number of captured
;; free variables exceeds the number of registers available for the
;; variadic part of the 'lispvalue_create_closure' C function call.
;; The remaining free variables must be passed correctly on the stack.
;; Expected output: 136
(let ((v1 1) (v2 2) (v3 3) (v4 4) (v5 5) (v6 6) (v7 7) (v8 8))
  (def mega-closure (lambda (x) 
    (+ x (+ v1 (+ v2 (+ v3 (+ v4 (+ v5 (+ v6 (+ v7 v8))))))))))
  (print-debug (mega-closure 100)))
