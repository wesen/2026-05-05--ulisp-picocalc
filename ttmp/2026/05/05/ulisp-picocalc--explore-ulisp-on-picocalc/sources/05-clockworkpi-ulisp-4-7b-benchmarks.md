I’ve done some [benchmarks](https://dev.metacircular.net/2025/04/first-light) with the PicoCalc running uLisp 4.7b:

|  | ARM @ 150MHz | ARM @ 200MHz | RISC-V @ 150MHz | RISC-V @ 200MHz |
| --- | --- | --- | --- | --- |
| tak | 4.6s | 3.4s | 6.6s | 4.8s |
| fib | 3.2s | 2.4s | 4.9s | 3.6s |
| q | 6.2s | 4.6s | 9.5s | 6.7s |
| q2 | 11.3s | 8.4s | 16.7s | 12.0s |
| factor | 1.6s | 1.2s | 2.2s | 1.6s |
| sieve | 18.1s | 13.6s | 23.6 | 17.1s |

I also have some [images built](https://dev.metacircular.net/picocalc-uf2) for anyone that wants to try it.

I sent off my patches to the uLisp author, who is trying to add support for the PicoCalc in uLisp right now.