#!/usr/bin/env python3
"""TortureLang bench 对照版：循环和 fib(20)。"""
import sys
import time


def fib(n: int) -> int:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)


def main() -> int:
    print("Python bench: loop sum 0..999999")
    t0 = time.perf_counter()
    i = 0
    s = 0
    limit = 1000000
    while i < limit:
        s += i
        i += 1
    t1 = time.perf_counter()
    print(s)
    print(f"loop_ms={(t1 - t0) * 1000:.3f}")

    print("Python bench: fib(20)")
    t0 = time.perf_counter()
    f = fib(20)
    t1 = time.perf_counter()
    print(f)
    print(f"fib_ms={(t1 - t0) * 1000:.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
