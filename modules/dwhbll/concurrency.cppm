module;

#include <dwhbll/concurrency/threading.h>
#include <dwhbll/concurrency/spinlock.h>
#include <dwhbll/concurrency/owning_spinlock.h>
#include <dwhbll/concurrency/recycling_concurrent_stack.h>

#include <dwhbll/concurrency/backoff/backoff_policy.h>
#include <dwhbll/concurrency/backoff/policy_pause.h>
#include <dwhbll/concurrency/backoff/policy_linear.h>
#include <dwhbll/concurrency/backoff/policy_exponential.h>

#include <dwhbll/concurrency/queues/bounded_spsc_queue.h>
#include <dwhbll/concurrency/queues/bounded_mpsc_queue.h>

export module dwhbll.concurrency;

export namespace dwhbll::concurrency {
    using dwhbll::concurrency::pin_thread_to_core;
    using dwhbll::concurrency::spinlock;
    using dwhbll::concurrency::owning_spinlock;
    using dwhbll::concurrency::RecyclingConcurrentStack;

    namespace backoff {
        using dwhbll::concurrency::backoff::BackoffPolicy;
        using dwhbll::concurrency::backoff::PolicyPause;
        using dwhbll::concurrency::backoff::PolicyLinear;
        using dwhbll::concurrency::backoff::PolicyExponential;
    }

    namespace queues {
        using dwhbll::concurrency::queues::BoundedSPSCQueue;
        using dwhbll::concurrency::queues::BoundedMPSCQueue;
    }
}
