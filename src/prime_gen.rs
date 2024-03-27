use std::f64::consts::E;
use std::sync::{Arc, Mutex, Condvar};
use std::thread;
use std::vec::Vec;
use std::collections::VecDeque;
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread::available_parallelism;

type DispatchFn = dyn Fn() -> bool + Send + 'static;

struct DispatchQueue {
    threads: Vec<thread::JoinHandle<()>>,
    q: Arc<(Mutex<VecDeque<Box<DispatchFn>>>, Condvar)>,
    quit: Arc<AtomicBool>,
    is_finished: Arc<AtomicBool>,
    is_started: Arc<AtomicBool>,
    result: Arc<AtomicBool>,
}

impl DispatchQueue {
    fn new(n: usize) -> Self {
        let q = Arc::new((Mutex::new(VecDeque::new()), Condvar::new()));
        let quit = Arc::new(AtomicBool::new(false));
        let is_finished = Arc::new(AtomicBool::new(true));
        let is_started = Arc::new(AtomicBool::new(false));
        let result = Arc::new(AtomicBool::new(false));

        let mut threads = Vec::new();
        for _ in 0..n {
            let q = Arc::clone(&q);
            let quit = Arc::clone(&quit);
            let is_finished = Arc::clone(&is_finished);
            let is_started = Arc::clone(&is_started);
            let result = Arc::clone(&result);

            let handle = thread::spawn(move || {
                Self::dispatch_thread_handler(q, quit, is_finished, is_started, result);
            });

            threads.push(handle);
        }

        Self {
            threads,
            q,
            quit,
            is_finished,
            is_started,
            result,
        }
    }

    fn reset_result(&self) {
        self.result.store(false, Ordering::Relaxed);
    }

    fn dispatch(&self, op: Box<DispatchFn>) {
        let (lock, cvar) = &*self.q;
        let mut q = lock.lock().unwrap();
        q.push_back(op);
        self.is_finished.store(false, Ordering::Relaxed);
        if !self.is_started.load(Ordering::Relaxed) {
            self.is_started.store(true, Ordering::Relaxed);
            cvar.notify_all();
        }
    }

    fn finish(&self) -> bool {
        let (lock, cvar) = &*self.q;
        let mut q = lock.lock().unwrap();
        if self.quit.load(Ordering::Relaxed) || !self.is_started.load(Ordering::Relaxed) {
            return false;
        }
        cvar.wait_while(lock, || !self.is_finished.load(Ordering::Relaxed) && !self.quit.load(Ordering::Relaxed)).unwrap();
        self.result.load(Ordering::Relaxed)
    }

    fn dump(&self) {
        let (lock, cvar) = &*self.q;
        let mut q = lock.lock().unwrap();
        if self.quit.load(Ordering::Relaxed) || !self.is_started.load(Ordering::Relaxed) {
            return;
        }
        q.clear();
        self.is_finished.store(true, Ordering::Relaxed);
        cvar.notify_all();
    }

    fn dispatch_thread_handler(
        q: Arc<(Mutex<VecDeque<Box<DispatchFn>>>, Condvar)>,
        quit: Arc<AtomicBool>,
        is_finished: Arc<AtomicBool>,
        is_started: Arc<AtomicBool>,
        result: Arc<AtomicBool>,
    ) {
        let (lock, cvar) = &*q;
        let mut q = lock.lock().unwrap();
        loop {
            q = cvar.wait_while(q, || q.is_empty() && !quit.load(Ordering::Relaxed)).unwrap();
            if quit.load(Ordering::Relaxed) {
                continue;
            }
            let op = q.pop_front().unwrap();
            drop(q);
            let res = op();
            result.fetch_or(res, Ordering::Relaxed);
            quit.fetch_or(res, Ordering::Relaxed);
            let (lock, cvar) = &*q;
            let mut q = lock.lock().unwrap();
            if q.is_empty() {
                is_finished.store(true, Ordering::Relaxed);
                cvar.notify_all();
            }
        }
    }
}

impl Drop for DispatchQueue {
    fn drop(&mut self) {
        let (lock, cvar) = &*self.q;
        let mut q = lock.lock().unwrap();
        if !self.is_started.load(Ordering::Relaxed) {
            return;
        }
        q.clear();
        self.quit.store(true, Ordering::Relaxed);
        drop(q);
        cvar.notify_all();

        for handle in self.threads.drain(..) {
            handle.join().unwrap();
        }
        self.is_finished.store(true, Ordering::Relaxed);
        cvar.notify_all();
    }
}




fn backward(n: u64) -> u64 {
    ((!(!n | 1)) / 3) + 1
}

fn forward(p: u64) -> u64 {
    (p << 1) + (!(!p | 1)) - 1
}

fn backward5(mut n: u64) -> u64 {
    n = ((n + 1) << 2) / 5;
    n = ((n + 1) << 1) / 3;
    (n + 1) >> 1
}

fn get_wheel5_increment(wheel5: &mut u32) -> u64 {
    let mut wheel_increment = 0;
    let mut is_wheel_multiple = false;
    loop {
        is_wheel_multiple = *wheel5 & 1 == 1;
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

fn make_not_multiple(n: u64) -> u64 {
    let mut n = n | 1;
    if n % 3 == 0 {
        n -= 2;
    }
    n
}

fn sieve_of_eratosthenes(n: u64) -> Vec<u64> {
    let mut known_primes = vec![2, 3, 5];
    if n < 2 {
        return vec![];
    }
    if n < known_primes.last().unwrap() + 2 {
        let highest_prime_it = known_primes.iter().position(|&x| x > n).unwrap();
        return known_primes[..highest_prime_it].to_vec();
    }
    known_primes.reserve((E.powf((n as f64).ln()) - E.powf(2f64.ln())) as usize);
    let mut thread_limit = 25;

    let cardinality = backward5(n) as usize;

    let mut not_prime = vec![false; cardinality + 1];

    let mut dispatch = DispatchQueue::new(available_parallelism().unwrap().get());
    let mut wheel5 = (1 << 7) | 1 as u32;
    let mut o = 1 as u64;
    loop {
        o += get_wheel5_increment(&mut wheel5);
        let p = forward(o);
        if p * p > n {
            break;
        }
        if thread_limit < p {
            dispatch.finish();
            thread_limit *= thread_limit;
        }
        if not_prime[backward5(p) as usize] {
            continue;
        }

        dispatch.dispatch(move || {
            let p2 = p << 1;
            let p4 = p << 2;
            let mut i = p * p;

            if p % 3 == 2 {
                not_prime[backward5(i) as usize] = true;
                i += p2;
                if i > n {
                    return false;
                }
            }
            let mut wheel30 = 0;
            for j in (0..30).step_by(2) {
                if i % 5 != 0 {
                    wheel30 |= 1 << j;
                    not_prime[backward5(i) as usize] = true;
                }
                i += p4;
                if i > n {
                    return false;
                }
                if i % 5 != 0 {
                    wheel30 |= 1 << (j + 1);
                    not_prime[backward5(i) as usize] = true;
                }
                i += p2;
                if i > n {
                    return false;
                }
            }
            loop {
                for j in (0..30).step_by(2) {
                    if (wheel30 >> j) & 1 == 1 {
                        not_prime[backward5(i) as usize] = true;
                    }
                    i += p4;
                    if i > n {
                        return false;
                    }
                    if (wheel30 >> (j + 1)) & 1 == 1 {
                        not_prime[backward5(i) as usize] = true;
                    }
                    i += p2;
                    if i > n {
                        return false;
                    }
                }
            }
        });
    }
    dispatch.finish();
    wheel5 = (1 << 7) | 1;
    o = 1;
    loop {
        o += get_wheel5_increment(&mut wheel5);
        let p = forward(o);
        if p > n {
            break;
        }
        if not_prime[backward5(p) as usize] {
            continue;
        }
        known_primes.push(p);
    }
    known_primes
}

fn segmented_sieve_of_eratosthenes(n: u64, limit: usize) -> Vec<u64> {
    if backward5(n) <= limit as u64 {
        return sieve_of_eratosthenes(n);
    }

    let mut low = make_not_multiple(limit as u64);
    let mut high = make_not_multiple((limit << 1) as u64);

    let mut known_primes = sieve_of_eratosthenes(limit as u64);
    known_primes.reserve((E.powf((n as f64).ln()) - E.powf(2f64.ln())) as usize);
    let mut dispatch = DispatchQueue::new(available_parallelism().unwrap().get());

    while low < n {
        if high >= n {
            high = make_not_multiple(n);
        }

        let b_low = backward5(low);
        let cardinality = backward5(high) - b_low;
        let mut not_prime = vec![false; (cardinality + 1) as usize];

        for k in 3..known_primes.len() {
            let p = known_primes[k];
            dispatch.dispatch(move || {
                let p2 = p << 1;
                let p4 = p << 2;
                let mut i = ((low + p - 1) / p) * p;
                if i & 1 == 0 {
                    i += p;
                }
                while i % 3 == 0 || i % 5 == 0 {
                    i += p2;
                }

                if i % 3 == 2 {
                    let q = backward5(i) - b_low;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q as usize] = true;
                    i += p2;
                }
                let mut wheel30 = 0;
                for j in (0..30).step_by(2) {
                    let q = backward5(i) - b_low;
                    if q > cardinality {
                        return false;
                    }
                    if i % 5 != 0 {
                        wheel30 |= 1 << j;
                        not_prime[q as usize] = true;
                    }
                    i += p4;
                    let q = backward5(i) - b_low;
                    if q > cardinality {
                        return false;
                    }
                    if i % 5 != 0 {
                        wheel30 |= 1 << (j + 1);
                        not_prime[q as usize] = true;
                    }
                    i += p2;
                }
                loop {
                    for j in (0..30).step_by(2) {
                        let q = backward5(i) - b_low;
                        if q > cardinality {
                            return false;
                        }
                        if (wheel30 >> j) & 1 == 1 {
                            not_prime[q as usize] = true;
                        }
                        i += p4;
                        let q = backward5(i) - b_low;
                        if q > cardinality {
                            return false;
                        }
                        if (wheel30 >> (j + 1)) & 1 == 1 {
                            not_prime[q as usize] = true;
                        }
                        i += p2;
                    }
                }
                loop {
                    let q = backward5(i) - b_low;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q as usize] = true;
                    i += p4;
                    let q = backward5(i) - b_low;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q as usize] = true;
                    i += p2;
                }
                false
            });
        }
        dispatch.finish();

        let mut q = 0;
        for o in 0.. {
            let p = forward(o);
            if p > cardinality {
                break;
            }
            if p % 5 == 0 {
                continue;
            }
            if not_prime[q as usize] {
                known_primes.push(forward(q + b_low));
            }
            q += 1;
        }

        low = make_not_multiple(low + limit as u64);
        high = make_not_multiple(high + limit as u64);
    }
    known_primes
}

fn main() {
    let mut n = 1000000000;
    println!("Primes up to number: ");
    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();
    n = input.trim().parse().unwrap();
    println!("Following are the prime numbers smaller than or equal to {}:", n);

    let primes = sieve_of_eratosthenes(n);
    for p in primes {
        print!("{} ", p);
    }
    println!();
}


