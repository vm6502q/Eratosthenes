use std::sync::{Arc, Mutex, Condvar};
use std::thread;
use std::collections::VecDeque;
use std::thread::available_parallelism;

type DispatchFn = dyn Fn() -> bool + Send + 'static;

struct DispatchQueue {
    threads: Vec<thread::JoinHandle<()>>,
    queue: Arc<(Mutex<VecDeque<Box<DispatchFn>>>, Condvar)>,
    quit: Arc<(Mutex<bool>, Condvar)>,
    is_finished: Arc<(Mutex<bool>, Condvar)>,
    is_started: Arc<(Mutex<bool>, Condvar)>,
    result: Arc<(Mutex<bool>, Condvar)>,
}

impl DispatchQueue {
    fn new(n: usize) -> Self {
        let queue = Arc::new((Mutex::new(VecDeque::new()), Condvar::new()));
        let quit = Arc::new((Mutex::new(false), Condvar::new()));
        let is_finished = Arc::new((Mutex::new(true), Condvar::new()));
        let is_started = Arc::new((Mutex::new(false), Condvar::new()));
        let result = Arc::new((Mutex::new(false), Condvar::new()));

        let mut threads = Vec::new();
        for _ in 0..n {
            let queue = Arc::clone(&queue);
            let quit = Arc::clone(&quit);
            let is_finished = Arc::clone(&is_finished);
            let is_started = Arc::clone(&is_started);
            let result = Arc::clone(&result);

            let handle = thread::spawn(move || {
                DispatchQueue::dispatch_thread_handler(queue, quit, is_finished, is_started, result);
            });

            threads.push(handle);
        }

        DispatchQueue {
            threads,
            queue,
            quit,
            is_finished,
            is_started,
            result,
        }
    }

    fn reset_result(&self) {
        let (lock, _) = &*self.result;
        let mut result = lock.lock().unwrap();
        *result = false;
    }

    fn dispatch(&self, op: Box<DispatchFn>) {
        let (lock, cvar) = &*self.queue;
        let mut queue = lock.lock().unwrap();
        queue.push_back(op);

        let (lock, _cvar) = &*self.is_finished;
        let mut is_finished = lock.lock().unwrap();
        *is_finished = false;

        let (lock, _cvar) = &*self.is_started;
        let mut is_started = lock.lock().unwrap();
        if !*is_started {
            *is_started = true;
            cvar.notify_all();
        }
    }

    fn finish(&self) -> bool {
        let (lock, _cvar) = &*self.quit;
        let quit = lock.lock().unwrap();
        if *quit {
            return false;
        }

        let (lock, _cvar) = &*self.is_finished;
        let is_finished = lock.lock().unwrap();
        if *is_finished {
            return false;
        }

        let (lock, cvar) = &*self.is_finished;
        let mut is_finished = lock.lock().unwrap();
        while !*is_finished {
            is_finished = cvar.wait(is_finished).unwrap();
        }

        let (lock, _cvar) = &*self.result;
        let result = lock.lock().unwrap();
        *result
    }

    fn dump(&self) {
        let (lock, _cvar) = &*self.quit;
        let quit = lock.lock().unwrap();
        if *quit {
            return;
        }

        let (lock, _cvar) = &*self.is_finished;
        let is_finished = lock.lock().unwrap();
        if *is_finished {
            return;
        }

        let (lock, _cvar) = &*self.queue;
        let mut queue = lock.lock().unwrap();
        queue.clear();

        let (lock, cvar) = &*self.is_finished;
        let mut is_finished = lock.lock().unwrap();
        *is_finished = true;
        cvar.notify_all();
    }

    fn dispatch_thread_handler(
        queue: Arc<(Mutex<VecDeque<Box<DispatchFn>>>, Condvar)>,
        quit: Arc<(Mutex<bool>, Condvar)>,
        is_finished: Arc<(Mutex<bool>, Condvar)>,
        is_started: Arc<(Mutex<bool>, Condvar)>,
        result: Arc<(Mutex<bool>, Condvar)>
    ) {
        let (lock, cvar) = &*queue;
        let (lock_quit, _cvar_quit) = &*quit;
        let (lock_is_finished, cvar_is_finished) = &*is_finished;
        let (lock_is_started, _cvar_is_started) = &*is_started;
        let (lock_result, _cvar_result) = &*result;

        loop {
            let mut queue = lock.lock().unwrap();
            while queue.is_empty() {
                let is_started = lock_is_started.lock().unwrap();
                if !*is_started {
                    queue = cvar.wait(queue).unwrap();
                } else {
                    let quit = lock_quit.lock().unwrap();
                    if *quit {
                        return;
                    }
                    queue = cvar.wait(queue).unwrap();
                }
            }

            let op = queue.pop_front().unwrap();

            drop(queue);

            let mut result = lock_result.lock().unwrap();
            *result |= op();

            let mut quit = lock_quit.lock().unwrap();
            *quit |= *result;

            let mut is_finished = lock_is_finished.lock().unwrap();
            if queue.is_empty() {
                *is_finished = true;
                cvar_is_finished.notify_all();
            }
        }
    }
}

fn backward(n: u64) -> u64 {
    ((!(!n | 1)) / 3) + 1
}

fn forward(p: usize) -> u64 {
    ((p << 1) + ((!(!p | 1)) - 1)) as u64
}

fn backward5(n: u64) -> usize {
    let n = ((n + 1) << 2) / 5;
    let n = ((n + 1) << 1) / 3;
    ((n + 1) >> 1) as usize
}

fn get_wheel5_increment(wheel5: &mut u32) -> usize {
    let mut wheel_increment = 0;
    let mut is_wheel_multiple;
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

fn sieve_of_eratosthenes(n: u64) -> Vec<u64> {
    let mut known_primes = vec![2, 3, 5];
    if n < 2 {
        return Vec::new();
    }
    if n < known_primes.last().unwrap() + 2 {
        let highest_prime_it = known_primes.iter().position(|&x| x > n).unwrap();
        return known_primes[..highest_prime_it].to_vec();
    }

    let cardinality = backward5(n);

    let not_prime = Arc::new(Mutex::new(vec![false; cardinality + 1]));

    let mut thread_boundary = 36;
    let mut wheel5 = (1 << 7) | 1;
    let mut o = 1;

    let dispatch = DispatchQueue::new(available_parallelism().unwrap().get());

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
        let not_prime = Arc::clone(&not_prime);
        let p_clone = p;
        dispatch.dispatch(Box::new(move || {
            let p2 = p_clone << 1;
            let p4 = p_clone << 2;
            let mut i = p_clone * p_clone;

            if p_clone % 3 == 2 {
                let mut not_prime = not_prime.lock().unwrap();
                not_prime[backward5(i)] = true;
                i += p2;
                if i > n {
                    return false;
                }
            }

            loop {
                let mut not_prime = not_prime.lock().unwrap();
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
        }));
    }

    dispatch.finish();

    loop {
        let p = forward(o);
        if p > n {
            break;
        }
        o += get_wheel5_increment(&mut wheel5);
        let not_prime = not_prime.lock().unwrap();
        if !not_prime[backward5(p)] {
            known_primes.push(p);
        }
    }

    known_primes
}

fn main() {
    let n; // = 1000000000;
    println!("Count primes up to number: ");
    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();
    n = input.trim().parse().unwrap();
    println!("Following is the count of prime numbers smaller than or equal to {}:", n);
    println!("{}", sieve_of_eratosthenes(n).len());
}
