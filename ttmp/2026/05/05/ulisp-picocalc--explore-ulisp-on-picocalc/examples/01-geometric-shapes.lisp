;;; 01-geometric-shapes.lisp
;;; Geometric drawing examples for uLisp on PicoCalc
;;; Tested on PicoCalc RP2040, uLisp 4.8f
;;;
;;; Color reference (RGB565):
;;;   0      = black
;;;   31     = blue
;;;   2016   = green
;;;   2047   = cyan
;;;   63488  = red
;;;   64512  = magenta
;;;   65504  = yellow
;;;   65535  = white

;;; --- Utility: draw a rectangle border ---

(defun draw-rect-border (x y w h col)
  (draw-line x y (+ x w) y col)
  (draw-line (+ x w) y (+ x w) (+ y h) col)
  (draw-line (+ x w) (+ y h) x (+ y h) col)
  (draw-line x (+ y h) x y col))

;;; --- Utility: draw a circle (midpoint algorithm) ---

(defun draw-circle (cx cy r col)
  (let ((x 0) (y r) (d (- 3 (* 2 r))))
    (loop
      (draw-pixel (+ cx x) (+ cy y) col)
      (draw-pixel (+ cx x) (- cy y) col)
      (draw-pixel (- cx x) (+ cy y) col)
      (draw-pixel (- cx x) (- cy y) col)
      (draw-pixel (+ cx y) (+ cy x) col)
      (draw-pixel (+ cx y) (- cy x) col)
      (draw-pixel (- cx y) (+ cy x) col)
      (draw-pixel (- cx y) (- cy x) col)
      (if (< d 0)
        (setq d (+ d (* 4 x) 6))
        (progn (setq d (+ d (* 4 (- x y)) 10)) (setq y (1- y))))
      (setq x (1+ x))
      (when (> x y) (return)))))

;;; --- Utility: draw a diamond (rotated square) ---

(defun draw-diamond (cx cy size col)
  (draw-line cx (- cy size) (+ cx size) cy col)
  (draw-line (+ cx size) cy cx (+ cy size) col)
  (draw-line cx (+ cy size) (- cx size) cy col)
  (draw-line (- cx size) cy cx (- cy size) col))

;;; --- Utility: draw a spirograph / geometric rose ---

(defun spirograph (cx cy r1 r2 offset col)
  (let ((a 0))
    (loop
      (let* ((r (+ r1 (* r2 (cos (/ a 57.2958)))))
             (x (+ cx (truncate (* r (cos (/ (* a offset) 57.2958))))))
             (y (+ cy (truncate (* r (sin (/ (* a offset) 57.2958)))))))
        (draw-pixel x y col))
      (incf a)
      (when (> a 3600) (return)))))

;;; ============================================================
;;; DEMO 1: Concentric rectangles
;;; ============================================================

(defun demo-rectangles ()
  (fill-screen 0)
  (draw-rect-border 10 10 300 300 65504)   ; yellow outer
  (draw-rect-border 30 30 260 260 31)      ; blue
  (draw-rect-border 50 50 220 220 63488)   ; red
  (draw-rect-border 70 70 180 180 2016)    ; green
  (draw-rect-border 90 90 140 140 65535)   ; white
  (draw-rect-border 110 110 100 100 64512) ; magenta
  (draw-rect-border 130 130 60 60 2047)    ; cyan
  (draw-rect-border 150 150 20 20 31)      ; blue inner
  (get-key))  ; wait for keypress before returning

;;; ============================================================
;;; DEMO 2: Concentric circles
;;; ============================================================

(defun demo-circles ()
  (fill-screen 0)
  (draw-circle 160 160 150 65504)  ; yellow outer
  (draw-circle 160 160 120 31)     ; blue
  (draw-circle 160 160 90 63488)   ; red
  (draw-circle 160 160 60 2016)    ; green
  (draw-circle 160 160 30 65535)   ; white inner
  (get-key))

;;; ============================================================
;;; DEMO 3: Spirograph with diamond overlay
;;; ============================================================

(defun demo-spirograph ()
  (fill-screen 0)
  ; Red spirograph, 7-fold symmetry
  (spirograph 160 160 80 40 7 63488)
  ; Green spirograph, 5-fold symmetry
  (spirograph 160 160 60 30 5 2016)
  ; White diamond on top
  (draw-diamond 160 160 100 65535)
  (get-key))

;;; ============================================================
;;; DEMO 4: Grid of colored squares
;;; ============================================================

(defun demo-grid ()
  (fill-screen 0)
  (let ((colors '(63488 64512 65504 65535 2016 2047 31 63488))
        (size 32)
        (gap 8))
    (dotimes (row 8)
      (dotimes (col 8)
        (let ((x (+ 8 (* col (+ size gap))))
              (y (+ 8 (* row (+ size gap))))
              (c (nth (mod (+ row col) 8) colors)))
          (fill-rect x y size size c)))))
  (get-key))

;;; ============================================================
;;; Run all demos
;;; ============================================================

(defun demo-all ()
  (format t "~%Demo 1: Rectangles~%")
  (demo-rectangles)
  (format t "~%Demo 2: Circles~%")
  (demo-circles)
  (format t "~%Demo 3: Spirograph~%")
  (demo-spirograph)
  (format t "~%Demo 4: Grid~%")
  (demo-grid)
  (fill-screen 0)
  (format t "~%Done!~%"))
