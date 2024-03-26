use std::cmp::Ordering;
use std::thread;
use std::sync::{Arc, Mutex};
use std::sync::mpsc::{channel, Sender, Receiver};

const BATCH_SIZE: usize = 1 << 10;

fn sqrt(to_test: u64) -> u64 {
    let mut start = 1u64;
    let mut end = to_test >> 1u64;
    let mut ans = 0u64;
    loop {
        let mid = (start + end) >> 1u64;
        let sqr = mid * mid;
        match sqr.cmp(&to_test) {
            Ordering::Equal => return mid,
            Ordering::Less => {
                start = mid + 1;
                ans = mid;
            },
            Ordering::Greater => end = mid - 1,
        }
        if start > end {
            return ans;
        }
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

fn is_multiple_parallel(p: u64, next_prime_index: usize, highest_index: usize, known_primes: &Vec<u64>, sender: Sender<bool>) {
    let batch_size = BATCH_SIZE;
    let max_lcv = (highest_index - next_prime_index) / BATCH_SIZE;
    for i in 0..max_lcv {
        let j = i * BATCH_SIZE + next_prime_index;
        let sender_clone = sender.clone();
        thread::spawn(move || {
            for k in 0..batch_size {
                if p % known_primes[j + k] == 0 {
                    sender_clone.send(true).unwrap();
                    return;
                }
            }
            sender_clone.send(false).unwrap();
        });
    }
}

fn is_multiple(p: u64, next_index: usize, known_primes: &Vec<u64>) -> bool {
    let sqrt_p = sqrt(p);
    let highest_index = known_primes.iter().position(|&x| x > sqrt_p).unwrap();
    let diff = highest_index - next_index;
    if highest_index > next_index && (diff >> 1) > BATCH_SIZE {
        let (sender, receiver) = channel();
        is_multiple_parallel(p, next_index, highest_index, known_primes, sender);
        for _ in 0..max_lcv {
            if receiver.recv().unwrap() {
                return true;
            }
        }
    }
    let next_index = diff % BATCH_SIZE;
    for i in next_index..highest_index {
        if p % known_primes[i] == 0 {
            return true;
        }
    }
    false
}

fn is_multiple(p: u64, known_primes: &Vec<u64>) -> bool {
    for &prime in known_primes {
        if p % prime == 0 {
            return true;
        }
    }
    false
}

fn wheel_inc(primes: Vec<u64>, limit: u64) -> Vec<bool> {
    let mut radius = 1u64;
    for &i in &primes {
        radius *= i;
    }
    if limit < radius {
        radius = limit;
    }
    let prime = primes.last().unwrap();
    let mut output = Vec::new();
    for i in 1..=radius {
        if !is_multiple(i, &primes) {
            output.push(i % prime == 0);
        }
    }
    output
}

fn get_wheel_increment(inc_seqs: &mut Vec<Vec<bool>>) -> usize {
    let mut wheel_increment = 0usize;
    let mut is_wheel_multiple = false;
    loop {
        for wheel in inc_seqs.iter_mut() {
            is_wheel_multiple = wheel[0];
            wheel.remove(0);
            if is_wheel_multiple {
                wheel.push(true);
                break;
            }
        }
        wheel_increment += 1;
        if !is_wheel_multiple {
            break;
        }
    }
    wheel_increment
}

fn get_wheel5_increment(wheel5: &mut u32) -> usize {
    let mut wheel_increment = 0usize;
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

fn make_not_multiple(n: u64) -> u64 {
    let mut n = n | 1;
    if n % 3 == 0 {
        n -= 2;
    }
    n
}

fn trial_division(n: u64) -> Vec<u64> {
    let mut known_primes = vec![2, 3];
    if n < 2 {
        return Vec::new();
    }
    if n < known_primes.last().unwrap() + 2 {
        let highest_prime_it = known_primes.iter().position(|&x| x > n).unwrap();
        return known_primes[..highest_prime_it].to_vec();
    }
    let mut wheel_primes = vec![2, 3];
    let mut inc_seqs = Vec::new();
    let wheel_limit = 17;
    let mut o = 1;
    loop {
        o += get_wheel_increment(&mut inc_seqs);
        let p = forward(o);
        if p > n {
            break;
        }
        if is_multiple(p, wheel_primes.len(), &known_primes) {
            continue;
        }
        known_primes.push(p);
        if p <= wheel_limit {
            wheel_primes.push(p as usize);
            inc_seqs.push(wheel_inc(known_primes.clone(), n));
            let wheel = &mut inc_seqs.last_mut().unwrap();
            wheel.remove(0);
            wheel.push(true);
        }
    }
    known_primes
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
    known_primes.reserve((n.log() - 2f64.log()).expint() - (2f64.log()).expint());
    let mut thread_limit = 26;
    let cardinality = backward5(n) as usize;
    let not_prime = Arc::new(Mutex::new(vec![false; cardinality + 1]));
    let (sender, receiver) = channel();
    let mut wheel5 = (1 << 7) | 1;
    let mut o = 1;
    loop {
        o += get_wheel5_increment(&mut wheel5);
        let p = forward(o);
        if p * p > n {
            break;
        }
        if thread_limit < p {
            let not_prime_clone = not_prime.clone();
            thread::spawn(move || {
                let mut not_prime = not_prime_clone.lock().unwrap();
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
                        if (wheel30 >> j) & 1 != 0 {
                            not_prime[backward5(i) as usize] = true;
                        }
                        i += p4;
                        if i > n {
                            return false;
                        }
                        if (wheel30 >> (j + 1)) & 1 != 0 {
                            not_prime[backward5(i) as usize] = true;
                        }
                        i += p2;
                        if i > n {
                            return false;
                        }
                    }
                }
                false
            });
        }
        let not_prime = not_prime.clone();
        thread::spawn(move || {
            let not_prime = not_prime.lock().unwrap();
            let mut i = backward5(p) as usize;
            loop {
                if not_prime[i] {
                    known_primes.push(forward(i as u64));
                }
                i += p as usize;
                if i > cardinality {
                    break;
                }
            }
        });
        if p > thread_limit {
            thread_limit *= thread_limit;
            let not_prime = not_prime.clone();
            thread::spawn(move || {
                let mut not_prime = not_prime.lock().unwrap();
                for i in (0..=cardinality).step_by(p as usize) {
                    not_prime[i] = true;
                }
            });
        }
    }
    for i in 0..=cardinality {
        if !not_prime.lock().unwrap()[i] {
            known_primes.push(forward(i as u64));
        }
    }
    for _ in 0..known_primes.len() {
        receiver.recv().unwrap();
    }
    known_primes
}

fn segmented_sieve_of_eratosthenes(n: u64, limit: usize) -> Vec<u64> {
    if backward(n) <= limit as u64 {
        return sieve_of_eratosthenes(n);
    }
    let mut low = make_not_multiple(limit as u64);
    let mut high = make_not_multiple((limit << 1) as u64);
    let mut known_primes = sieve_of_eratosthenes(limit as u64);
    known_primes.reserve((n.log() - 2f64.log()).expint() - (2f64.log()).expint());
    let mut thread_limit = 26;
    let cardinality = backward5(n) as usize;
    let not_prime = Arc::new(Mutex::new(vec![false; cardinality + 1]));
    while low < n {
        if high >= n {
            high = make_not_multiple(n);
        }
        let b_low = backward(low);
        let cardinality = backward(high) - b_low;
        let not_prime = not_prime.clone();
        for &p in &known_primes {
            thread::spawn(move || {
                let mut not_prime = not_prime.lock().unwrap();
                let p2 = p << 1;
                let p4 = p << 2;
                let mut i = ((low + p - 1) / p) * p;
                if i & 1 == 0 {
                    i += p;
                }
                if i % 3 == 0 {
                    i += p2;
                }
                if i % 3 == 2 {
                    let q = backward5(i) as usize;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q] = true;
                    i += p2;
                }
                loop {
                    let q = backward5(i) as usize;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q] = true;
                    i += p4;
                    let q = backward5(i) as usize;
                    if q > cardinality {
                        return false;
                    }
                    not_prime[q] = true;
                    i += p2;
                }
                false
            });
        }
        for i in 0..=cardinality {
            if !not_prime.lock().unwrap()[i] {
                known_primes.push(forward(i as u64 + b_low));
            }
        }
        low = make_not_multiple(low + limit as u64);
        high = make_not_multiple(high + limit as u64);
    }
    known_primes
}

fn main() {
    let mut n = 1000000000u64;
    println!("Primes up to number: ");
    std::io::stdin().read_line(&mut n).unwrap();
    let n: u64 = n.trim().parse().unwrap();
    println!("Following are the prime numbers smaller than or equal to {}:", n);
    let primes = segmented_sieve_of_eratosthenes(n, BATCH_SIZE);
    for p in primes {
        print!("{} ", p);
    }
    println!();
}



