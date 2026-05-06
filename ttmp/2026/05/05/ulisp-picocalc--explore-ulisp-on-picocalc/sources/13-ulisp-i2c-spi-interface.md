## I2C and SPI serial interfaces

uLisp includes a simple, unified I2C and SPI interface using **read-byte** and **write-byte** to transmit data via the appropriate interface, and streams to identify which interface is being used. For examples of their use see [I2C clock](http://www.ulisp.com/show?1DO6) and [Data logging](http://www.ulisp.com/show?1FCN).

### I2C interface

The I2C interface allows a master device to communicate with one or more slave devices over two wires. It is also known as the two-wire interface.

For details of which pins are used for the I2C interface on different processors see [Language reference: with-i2c](http://www.ulisp.com/show?3L#withi2c).

To set up an interface to an I2C device you use the **with-i2c** form. This takes a variable, which will be bound to the stream within the form, the address of the I2C device, and the direction of the transfer.

#### Port scanning

If the start operation fails the stream variable is bound to **nil**. This allows you to write a very simple port scanner to find the address of each of the I2C devices on the bus:

```
(defun scan () 
  (dotimes (p 127)
    (with-i2c (str p)
      (when str (print p)))))
```

If your board has a second I2C port you can scan that with:

```
(defun scan () 
  (dotimes (p 127)
    (with-i2c (str 1 p)
      (when str (print p)))))
```

#### Writing to a slave

For example, to write the three bytes 0, 1, and 2 to a slave device with address #x68 you would use:

```
(with-i2c (str #x68) 
  (write-byte 0 str) 
  (write-byte 1 str) 
  (write-byte 2 str))
```

Within the form the first parameter to `with-i2c`, for example `str`, is bound to a stream object. For an I2C stream, printing this variable will show the address of the device; for example:

<i2c-stream #x68>

#### Reading from a slave

When the master device is reading from a slave it has to terminate each read operation with an ACK apart from the last one, which is terminated by a NAK. This tells the slave not to send any further bytes.

The uLisp I2C interface allows you to identify the last `read-byte` in either of two ways:

You can specify the total number of bytes you are going to read, as the third parameter of the `with-i2c` form. With this approach `read-byte` will automatically terminate the last call with a NAK:

```
(defun get ()
  (with-i2c (str #x68 3) 
    (list
     (read-byte str)
     (read-byte str)
     (read-byte str))))
```

Alternatively you can just specify the third parameter of the **with-i2c** form as **t** (to indicate read), and explicitly identify the last **read-byte** command by specifying an additional **t** parameter:

```
(defun get ()
  (with-i2c (str #x68 t) 
    (list
     (read-byte str)
     (read-byte str)
     (read-byte str t))))
```

#### Issuing a restart

If you want to send a value to a slave device, and then immediately read bytes back, you should issue a **restart-i2c** command to switch between write mode and read mode. This ensures that the master retains control of the I2C bus between the write and the read.

The **restart-i2c** command takes two parameters; the stream, and the direction of the transfer. For example:

```
(restart-i2c str t)
```

As in the **with-i2c** form the third parameter should be **nil**, or omitted, for a write, and **t**, or the number of bytes, for a read.

A typical application is reading bytes from an I2C EEPROM. The master would first write the address, then send a restart, and read the data from the EEPROM starting at that address.

### SPI interface

The SPI interface allows a master device to communicate with one or more slave devices using three common wires, and a fourth select wire per slave device.

For details of which pins are used for the SPI interface on different processors see [Language reference: with-spi](http://www.ulisp.com/show?3L#withspi).

In addition, each slave needs an I/O pin to be used for the select line.

To set up an interface to an SPI device you use the **with-spi** form. This takes a variable, which will be bound to the stream within the form, and the Arduino pin number used to select the device. Additional optional arguments allow you to specify the speed, bit order, and SPI mode; for full details see [Language reference](http://www.ulisp.com/show?3L#withspi).

#### Writing to a slave

For example, to write the three bytes 0, 1, and 2 to a slave device using pin 7 as select you would use:

```
(with-spi (str 7) 
  (write-byte 0 str) 
  (write-byte 1 str) 
  (write-byte 2 str))
```

Within the form the first parameter to **with-spi**, for example **str**, is bound to a stream object. For an SPI stream, printing this variable will show the select pin number of the device; for example:

<spi-stream #x07>

#### Reading from a slave

Reading is similar; for example:

```
(with-spi (str 7) 
  (list
   (read-byte str) 
   (read-byte str) 
   (read-byte str)))
```

### Nesting calls

It is illegal to nest calls to **with-i2c** or **with-spi** because it's not possible to have communication with two devices at once in either protocol. However, you can nest a **with-spi** and a **with-i2c** form. For example, you could do this to read bytes from one interface and write them to the other:

```
(with-i2c (i2c #x77)
  (with-spi (spi 9)
    (write-byte (read-byte spi) i2c)))
```

### Implementation

The SPI interface uses the standard Arduino SPI library.

For the I2C interface I decided against using the standard Arduino Wire library on the AVR platforms because it doesn't fit well with uLisp. Instead I incorporated my own I2C routines <sup><a href="http://www.ulisp.com/show?1EK4#cite_note1">[1]</a></sup>. The other platforms use the Arduino Wire library.

---

1. [^](http://www.ulisp.com/show?1EK4#cite_ref1) [TinyI2C Library](https://github.com/technoblogy/tiny-i2c) on Github.

---

Previous: [SD card interface](http://www.ulisp.com/show?207M)

Next: [Lisp Library](http://www.ulisp.com/show?27OV)