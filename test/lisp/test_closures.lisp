;; Test 1
;; A simple function with two parameters
;; Expected output: 15
(def (add a b) (+ a b))
(print-debug (add 7 8))


;; Test 2
;; Recursive Function (Factorial)
;; Expected output: 120
(def (factorial n)
  (if (= n 0)
      1
      (* n (factorial (- n 1)))))
(print-debug (factorial 5))


;; Test 3
;; Nested Closures & Lexical Scoping 
;; A function defined inside a let-binding.
;; The inner lambda captures 'x' from its lexical environment.
;; Expected output: 30
(let ((x 10))
  (def closure (lambda (y) (+ x y)))
  (print-debug (closure 20)))


;; Test 4 
;; Shadowing
;; The 'x' in the inner let shadows the outer 'x'.
;; The closure should capture the inner 'x' (value 50).
;; Expected output: 65
(let ((x 10))
  (let ((x 50))
    (def shadow-closure (lambda (y) (+ x y))))
  (print-debug (shadow-closure 15)))


;; Test 5
;; Functions as Return Values (Closure Factory) 
;; 'make-adder' is a function that returns another function.
;; The returned function "remembers" the 'x' it was created with.
;; Expected output: 105
;; Expected output: 210
(def (make-adder x)
  (lambda (y) (+ x y)))

(def add5 (make-adder 5))
(print-debug (add5 100))

(def add10 (make-adder 10))
(print-debug (add10 200))


;; Test 6 
;; First-Class Functions (Passing as Arguments) 
;; The 'apply' function takes another function 'f' as an argument.
;; Expected output: 25
(def (apply f a b) (f a b))
(print-debug (apply add 12 13))


;; Test 7 
;; Closure Capturing Multiple Free Variables
;; The lambda captures both 'prefix' and 'suffix' from the let block.
;; Expected output: 130  (100 + 20 + 10)
(let ((prefix 20) (suffix 10))
  (def multi-capture (lambda (n) (+ n prefix suffix)))
  (print-debug (multi-capture 100)))

;; Test 8 
;; Mutual Recursion (via shared environment)
;; is-even and is-odd call each other. This is tricky. It will require a 2 pass parser.
;; It works because when is-odd is defined, is-even is already in the
;; global scope, so is-odd can capture it as a free variable.
;; Expected output: 1 (#t)
;; Expected output: 0 (#f)
;; (def (is-even n)
;;   (if (= n 0)
;;       #t
;;       (is-odd (- n 1))))

;; (def (is-odd n)
;;   (if (= n 0)
;;       #f
;;       (is-even (- n 1))))

;; (print-debug (is-even 10))
;; (print-debug (is-even 7))