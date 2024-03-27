use std::sync::{Arc, Mutex, Condvar};
use std::thread;
use std::collections::VecDeque;
use std::thread::available_parallelism;

type DispatchFn = dyn Fn() -> bool + Send + 'static;

struct DispatchQueue {
    threads: Vec<thread::JoinHandle<()>>,
    q: Arc<Mutex<VecDeque<Box<DispatchFn>>>>,
    cv: Arc<Condvar>,
    cv_finished: Arc<Condvar>,
    quit: Arc<Mutex<bool>>,
    is_finished: Arc<Mutex<bool>>,
    is_started: Arc<Mutex<bool>>,
    result: Arc<Mutex<bool>>,
}

impl DispatchQueue {
    fn new(n: usize) -> Self {
        let q = Arc::new(Mutex::new(VecDeque::new()));
        let cv = Arc::new(Condvar::new());
        let cv_finished = Arc::new(Condvar::new());
        let quit = Arc::new(Mutex::new(false));
        let is_finished = Arc::new(Mutex::new(true));
        let is_started = Arc::new(Mutex::new(false));
        let result = Arc::new(Mutex::new(false));

        let mut threads = Vec::new();
        for _ in 0..n {
            let q = Arc::clone(&q);
            let cv = Arc::clone(&cv);
            let cv_finished = Arc::clone(&cv_finished);
            let quit = Arc::clone(&quit);
            let is_finished = Arc::clone(&is_finished);
            let is_started = Arc::clone(&is_started);
            let result = Arc::clone(&result);

            let handle = thread::spawn(move || {
                Self::dispatch_thread_handler(q, cv, cv_finished, quit, is_finished, is_started, result);
            });

            threads.push(handle);
        }

        Self {
            threads,
            q,
            cv,
            cv_finished,
            quit,
            is_finished,
            is_started,
            result,
        }
    }

    fn reset_result(&self) {
        *self.result.lock().unwrap() = false;
    }

    fn dispatch(&self, op: Box<DispatchFn>) {
        let mut q = self.q.lock().unwrap();
        q.push_back(op);
        *self.is_finished.lock().unwrap() = false;
        if !*self.is_started.lock().unwrap() {
            *self.is_started.lock().unwrap() = true;
            self.cv.notify_one();
        }
    }

    fn finish(&self) -> bool {
        let mut is_finished = self.is_finished.lock().unwrap();
        let mut quit = self.quit.lock().unwrap();
        if *quit || !*self.is_started.lock().unwrap() {
            return false;
        }
        is_finished = self.cv_finished.wait_while(is_finished, |is_finished| !*is_finished || *quit).unwrap();
        *is_finished
    }

    fn dump(&self) {
        let mut q = self.q.lock().unwrap();
        q.clear();
        *self.is_finished.lock().unwrap() = true;
        self.cv_finished.notify_all();
    }

    fn dispatch_thread_handler(
        q: Arc<Mutex<VecDeque<Box<DispatchFn>>>>,
        cv: Arc<Condvar>,
        cv_finished: Arc<Condvar>,
        quit: Arc<Mutex<bool>>,
        is_finished: Arc<Mutex<bool>>,
        is_started: Arc<Mutex<bool>>,
        result: Arc<Mutex<bool>>,
    ) {
        loop {
            let mut q = q.lock().unwrap();
            let mut quit = quit.lock().unwrap();
            let mut result = result.lock().unwrap();
            let mut is_finished = is_finished.lock().unwrap();

            while q.is_empty() && !*quit {
                q = cv.wait(q).unwrap();
            }

            if *quit {
                continue;
            }

            let op = q.pop_front().unwrap();

            drop(q);
            *result |= op();
            *quit |= *result;

            let mut is_finished = is_finished.lock().unwrap();
            if q.is_empty() {
                *is_finished = true;
                cv_finished.notify_all();
            }
        }
    }
}

fn backward(n: u64) -> usize {
    ((!(n | 1) / 3) + 1) as usize
}

fn forward(p: usize) -> u64 {
    ((p << 1) + (!(p | 1)) - 1) as u64
}

fn backward5(n: u64) -> usize {
    let n = ((n + 1) << 2) / 5;
    let n = ((n + 1) << 1) / 3;
    ((n + 1) >> 1) as usize
}

fn get_wheel5_increment(wheel5: &mut u32) -> usize {
    let mut wheel_increment = 0;
    let mut is_wheel_multiple = false;
    loop {
        is_wheel_multiple = *wheel5 & 1 != 0;
        *wheel5 >>= 1;
        if is_wheel_multiple {
            *wheel5 |= 1 << 9;
        }
        wheel_increment += 1;
        if !is_wheel_multiple {
            break;
        }
    }
    wheel_increment
}

fn sieve_of_eratosthenes(n: u64) -> Vec<u64> {
    let mut known_primes = vec![2, 3, 5];
    if n < 2 {
        return Vec::new();
    }
    if n < known_primes.last().unwrap() + 2 {
        let highest_prime_it = known_primes.iter().position(|&p| p > n).unwrap();
        return known_primes[..highest_prime_it].to_vec();
    }

    let cardinality = backward5(n);

    let mut not_prime = vec![false; cardinality + 1];

    let mut thread_boundary = 36;

    let dispatch = DispatchQueue::new(available_parallelism().unwrap().get());
    let mut wheel5 = (1 << 7) | 1;
    let mut o = 1;
    loop {
        o += get_wheel5_increment(&mut wheel5);
        let p = forward(o);
        if p * p > n {
            break;
        }
        if thread_boundary < p {
            dispatch.finish();
            thread_boundary *= thread_boundary;
        }
        if not_prime[backward5(p)] {
            continue;
        }
        known_primes.push(p);
        dispatch.dispatch(Box::new(move || {
            let p2 = p << 1;
            let p4 = p << 2;
            let mut i = p * p;

            if p % 3 == 2 {
                not_prime[backward5(i)] = true;
                i += p2;
                if i > n {
                    return false;
                }
            }

            loop {
                if i % 5 != 0 {
                    not_prime[backward5(i)] = true;
                }
                i += p4;
                if i > n {
                    return false;
                }
                if i % 5 != 0 {
                    not_prime[backward5(i)] = true;
                }
                i += p2;
                if i > n {
                    return false;
                }
            }
            false
        }));
    }
    dispatch.finish();
    loop {
        let p = forward(o);
        if p > n {
            break;
        }
        o += get_wheel5_increment(&mut wheel5);
        if not_prime[backward5(p)] {
            continue;
        }
        known_primes.push(p);
    }
    known_primes
}

fn main() {
    let mut n = 1000000000;
    println!("Primes up to number: ");
    std::io::stdin().read_line(&mut n).unwrap();
    let n: u64 = n.trim().parse().unwrap();
    println!("Following are the prime numbers smaller than or equal to {}:", n);

    let primes = sieve_of_eratosthenes(n);
    for p in primes {
        print!("{} ", p);
    }
    println!();
}
