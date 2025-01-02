import time
from eratosthenes import segmented_count


start = time.perf_counter()
print(segmented_count(1000000000))
print(time.perf_counter() - start)
