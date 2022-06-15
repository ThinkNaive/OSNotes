import time, subprocess

REPEAT = 10

def run(nthread):
    st = time.time()
    subprocess.run(['./a.out', nthread], check=True)
    return time.time() - st

for nthread in '1 2 3 4 8 16 24 32'.split():
    times = sorted([run(nthread) for i in range(REPEAT)])
    print(nthread, times[REPEAT // 2])
