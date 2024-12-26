import _eratosthenes

def count(n):
    return int(_eratosthenes._count(str(n)))

def segmented_count(n):
    return int(_eratosthenes._segmented_count(str(n)))

def sieve(n):
    v = _eratosthenes._sieve(str(n))
    l = []
    for p in v:
        l.append(int(p))

    return l

def segmented_sieve(n):
    v = _eratosthenes._segmented_sieve(str(n))
    l = []
    for p in v:
        l.append(int(p))

    return l