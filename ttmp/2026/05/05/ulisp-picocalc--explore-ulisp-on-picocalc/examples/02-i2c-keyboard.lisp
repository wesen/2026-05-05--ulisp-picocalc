;;; 02-i2c-keyboard.lisp
;;; I2C keyboard register access from uLisp on PicoCalc
;;; Tested on PicoCalc RP2040, uLisp 4.8f, keyboard BIOS 1.2
;;;
;;; IMPORTANT: The keyboard is on Wire1 (I2C1, GP6/GP7).
;;; uLisp routes address >= 128 to Wire1, so use #x9F (0x1F + 128).
;;; DO NOT read register 0x08 (RST) — it crashes the keyboard MCU.

;;; --- Read a single keyboard register ---

(defun kbd-reg (reg)
  (with-i2c (str #x9F)
    (write-byte reg str)
    (restart-i2c str 1)
    (read-byte str)))

;;; --- Write a single keyboard register ---

(defun kbd-write (reg val)
  (with-i2c (str #x9F)
    (write-byte (+ reg 128) str)
    (write-byte val str)))

;;; --- Register map ---

;;; 0x01 VER  - Firmware version (0 = BIOS 1.2, 0x14 = BIOS 1.4, 0x16 = BIOS 1.6)
;;; 0x02 CFG  - Configuration flags
;;; 0x03 INT  - Interrupt status
;;; 0x04 KEY  - Key count + lock states
;;; 0x05 BKL  - Display backlight (0-255)
;;; 0x06 DEB  - Debounce config
;;; 0x07 FRQ  - Poll frequency config
;;; 0x08 RST  - Reset (** DO NOT READ THIS — CRASHES KEYBOARD **)
;;; 0x09 FIF  - Key FIFO (read key events)
;;; 0x0A BK2  - Keyboard backlight (0-255)
;;; 0x0B BAT  - Battery status
;;; 0x0E OFF  - Power off (BIOS 1.4+ only)

;;; --- Display backlight control ---

(defun kbd-backlight (val)
  (kbd-write #x05 val))

;;; --- Keyboard backlight control ---

(defun kbd-key-backlight (val)
  (kbd-write #x0A val))

;;; --- Dump all safe registers ---

(defun kbd-dump ()
  (dolist (r '(#x01 #x02 #x03 #x04 #x05 #x06 #x07 #x09 #x0A #x0B))
    (format t "~2d: ~d~%" r (kbd-reg r))))

;;; --- Detect BIOS version ---

(defun kbd-bios-version ()
  (let ((v (kbd-reg #x01)))
    (cond
      ((= v 0) "BIOS 1.2 or earlier")
      ((= v #x14) "BIOS 1.4")
      ((= v #x16) "BIOS 1.6")
      (t (format nil "Unknown (0x~2,'0x)" v)))))

;;; --- Usage examples ---

;; Read firmware version
;; (kbd-reg #x01)
;; => 0  (BIOS 1.2)

;; Detect BIOS version string
;; (kbd-bios-version)
;; => "BIOS 1.2 or earlier"

;; Set display backlight to max
;; (kbd-backlight 255)

;; Set keyboard backlight to half
;; (kbd-key-backlight 128)

;; Dump all safe registers
;; (kbd-dump)

;; Read battery status
;; (kbd-reg #x0B)
;; => 11
