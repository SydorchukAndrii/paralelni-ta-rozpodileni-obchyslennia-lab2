#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <cmath>
#include <string>
#include <sstream>

using namespace std;

// -----------------------------------------------------------------------------
// Auxiliary mathematical functions
// -----------------------------------------------------------------------------
bool is_prime(uint64_t num)
{
  if (num < 2)
    return false;
  if (num == 2 || num == 3)
    return true;
  if (num % 2 == 0 || num % 3 == 0)
    return false;
  for (uint64_t i = 5; i * i <= num; i += 6)
  {
    if (num % i == 0 || num % (i + 2) == 0)
      return false;
  }
  return true;
}

uint64_t find_nth_prime(int n)
{
  if (n <= 0)
    return 0;
  int count = 0;
  uint64_t num = 1;
  while (count < n)
  {
    ++num;
    if (is_prime(num))
    {
      ++count;
    }
  }
  return num;
}

namespace Task1
{
  queue<int> data_queue;
  mutex mtx;
  bool data_ready = false;

  void DataPreparation()
  {
    cout << "[DataPreparation] Enter integers (enter 'end' or any letter to finish):\n";
    int val;
    while (cin >> val)
    {
      lock_guard<mutex> lk(mtx);
      data_queue.push(val);
    }
    cin.clear();
    string dummy;
    cin >> dummy; // clear remaining input

    {
      lock_guard<mutex> lk(mtx);
      data_ready = true;
    }
    cout << "[DataPreparation] Input completed. Flag set.\n";
  }

  void DataProcessing()
  {
    unique_lock<mutex> lk(mtx);
    while (!data_ready)
    {
      lk.unlock();
      this_thread::sleep_for(chrono::milliseconds(100));
      lk.lock();
    }

    cout << "[DataProcessing] Prime numbers from the queue: ";
    bool found = false;
    while (!data_queue.empty())
    {
      int num = data_queue.front();
      data_queue.pop();
      if (num > 1 && is_prime(num))
      {
        cout << num << " ";
        found = true;
      }
    }
    if (!found)
      cout << "none";
    cout << "\n";
  }

  void run()
  {
    data_ready = false;
    while (!data_queue.empty())
      data_queue.pop();

    thread prep_thread(DataPreparation);
    prep_thread.detach(); // detached according to task requirements

    thread proc_thread(DataProcessing);
    proc_thread.join(); // main thread waits for processing to finish
  }
}

namespace Task2
{
  mutex mtx;
  condition_variable cv;
  int i = 0;

  void Waits(int id)
  {
    unique_lock<mutex> lk(mtx);
    cout << "[Waits #" << id << "] Entering wait state...\n";
    cv.wait(lk, []
            { return i == 1; });
    cout << "[Waits #" << id << "] Waiting completed! (i == " << i << ")\n";
  }

  void Awake()
  {
    this_thread::sleep_for(chrono::milliseconds(100));
    {
      lock_guard<mutex> lk(mtx);
      cout << "[Awake] Notifying all threads when i = " << i << " (condition is false)...\n";
    }
    cv.notify_all();

    this_thread::sleep_for(chrono::milliseconds(100));

    {
      lock_guard<mutex> lk(mtx);
      i = 1;
      cout << "[Awake] Assigned i = 1. Re-notifying all threads!\n";
    }
    cv.notify_all();
  }

  void run()
  {
    i = 0;
    thread t1(Waits, 1);
    thread t2(Waits, 2);
    thread t3(Waits, 3);
    thread t_awake(Awake);

    t_awake.join();
    t1.join();
    t2.join();
    t3.join();
  }
}

namespace Task3
{
  mutex mtx;
  condition_variable cv;
  int i = 0;

  void Worker(int id)
  {
    unique_lock<mutex> lk(mtx);
    cv.wait(lk, []
            { return i == 1; });
    cout << "Notification from thread " << id << "\n";
  }

  void Notify()
  {
    this_thread::sleep_for(chrono::milliseconds(100));
    {
      lock_guard<mutex> lk(mtx);
      i = 1;
      cout << "[Notify] Set i = 1. Calling notify_one()...\n";
    }
    cv.notify_one();
  }

  void run()
  {
    i = 0;
    thread t1(Worker, 1);
    thread t2(Worker, 2);
    thread t3(Worker, 3);
    thread t_notify(Notify);

    t_notify.join();

    // Give a little time for the single thread to wake up
    this_thread::sleep_for(chrono::milliseconds(100));

    // Allow the remaining threads to terminate cleanly to prevent crash/deadlock
    cv.notify_all();

    t1.join();
    t2.join();
    t3.join();
  }
}

namespace Task4
{
  queue<int> data_queue;
  mutex mtx;
  condition_variable cv;
  bool finished = false;

  void DataPreparation()
  {
    cout << "[DataPreparation] Enter numbers (enter 'end' or any letter to finish):\n";
    int val;
    while (cin >> val)
    {
      {
        lock_guard<mutex> lk(mtx);
        data_queue.push(val);
      }
      cv.notify_one();
    }
    cin.clear();
    string dummy;
    cin >> dummy;

    {
      lock_guard<mutex> lk(mtx);
      finished = true;
    }
    cv.notify_all();
  }

  void DataProcessing()
  {
    unique_lock<mutex> lk(mtx);
    cout << "[DataProcessing] Prime numbers: ";
    while (true)
    {
      // Wait condition: queue is not empty or input is finished
      cv.wait(lk, []
              { return !data_queue.empty() || finished; });

      while (!data_queue.empty())
      {
        int num = data_queue.front();
        data_queue.pop();
        if (num > 1 && is_prime(num))
        {
          cout << num << " ";
        }
      }

      if (finished && data_queue.empty())
      {
        break;
      }
    }
    cout << "\n";
  }

  void run()
  {
    finished = false;
    while (!data_queue.empty())
      data_queue.pop();

    thread prep(DataPreparation);
    prep.detach();

    thread proc(DataProcessing);
    proc.join();
  }
}

namespace Task5
{
  void execute_test(int n, launch policy, const string &policy_name)
  {
    cout << "\n--- Testing mode: " << policy_name << " ---\n";

    auto start_async = chrono::high_resolution_clock::now();
    // Launching asynchronous task
    future<uint64_t> fut = async(policy, find_nth_prime, n);

    cout << "Select a function for number n = " << n << ":\n";
    cout << "1. Square root\n2. Sine\n3. Natural logarithm\nChoice: ";
    int choice;
    cin >> choice;

    double math_res = 0.0;
    if (choice == 1)
      math_res = sqrt(n);
    else if (choice == 2)
      math_res = sin(n);
    else if (choice == 3)
      math_res = log(n);
    else
      math_res = sqrt(n);

    cout << "Result of mathematical function: " << math_res << "\n";
    cout << "Waiting for prime calculation result (calling get())...\n";

    auto start_wait = chrono::high_resolution_clock::now();
    uint64_t prime_num = fut.get();
    auto end_wait = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> wait_duration = end_wait - start_wait;
    chrono::duration<double, milli> total_duration = end_wait - start_async;

    cout << n << "th prime number: " << prime_num << "\n";
    cout << "Blocking time in .get(): " << wait_duration.count() << " ms\n";
    cout << "Total time since creation: " << total_duration.count() << " ms\n";
  }

  void run()
  {
    cout << "Enter prime number index n (e.g. 100000 or 200000): ";
    int n;
    cin >> n;

    execute_test(n, launch::deferred, "std::launch::deferred");
    execute_test(n, launch::async, "std::launch::async");
  }
}

namespace Task6
{
  deque<packaged_task<uint64_t()>> task_queue;
  deque<int> indices_queue;
  mutex deque_mtx;
  condition_variable cv;
  bool stop_worker = false;

  void WorkerThread()
  {
    while (true)
    {
      packaged_task<uint64_t()> task;
      int n = 0;
      {
        unique_lock<mutex> lk(deque_mtx);
        cv.wait(lk, []
                { return !task_queue.empty() || stop_worker; });

        if (stop_worker && task_queue.empty())
        {
          break;
        }

        task = move(task_queue.front());
        task_queue.pop_front();
        n = indices_queue.front();
        indices_queue.pop_front();
      }

      // Executing task outside of the mutex
      task();
    }
  }

  void run()
  {
    stop_worker = false;
    task_queue.clear();
    indices_queue.clear();

    thread worker(WorkerThread);

    vector<pair<int, future<uint64_t>>> futures;

    cout << "Enter values of n to find the nth prime number.\n";
    cout << "Enter 'stop' or any non-numeric character to finish:\n";

    int n;
    while (cin >> n)
    {
      packaged_task<uint64_t()> task([n]()
                                     { return find_nth_prime(n); });
      futures.push_back({n, task.get_future()});

      {
        lock_guard<mutex> lk(deque_mtx);
        task_queue.push_back(move(task));
        indices_queue.push_back(n);
      }
      cv.notify_one();
    }
    cin.clear();
    string dummy;
    cin >> dummy;

    // Notify the worker that task submission is finished
    {
      lock_guard<mutex> lk(deque_mtx);
      stop_worker = true;
    }
    cv.notify_all();

    // Wait for the worker thread to finish
    worker.join();

    // Retrieving and printing results
    cout << "\n--- Calculation Results ---\n";
    for (auto &item : futures)
    {
      cout << item.first << "th prime number = " << item.second.get() << "\n";
    }
  }
}

namespace Task7
{
  void FirstThread(int n,
                   promise<uint64_t> prom_n,
                   promise<bool> prom_signal,
                   promise<uint64_t> prom_10n)
  {
    // Calculation of the nth prime number
    uint64_t p_n = find_nth_prime(n);
    prom_n.set_value(p_n);

    // Signal to the second thread
    prom_signal.set_value(true);

    // Calculation of the (n * 10)th prime number
    uint64_t p_10n = find_nth_prime(n * 10);
    prom_10n.set_value(p_10n);
  }

  void SecondThread(int n, future<bool> fut_signal)
  {
    // Waiting for signal from the first thread
    if (fut_signal.get())
    {
      this_thread::sleep_for(chrono::seconds(2));
      cout << "\n[Thread 2] Square root of n (" << n << ") = "
           << sqrt(n) << "\n"
           << flush;
    }
  }

  void run()
  {
    cout << "Enter n (e.g., 1000): ";
    int n;
    cin >> n;

    promise<uint64_t> prom_n;
    promise<bool> prom_signal;
    promise<uint64_t> prom_10n;

    future<uint64_t> fut_n = prom_n.get_future();
    future<bool> fut_signal = prom_signal.get_future();
    future<uint64_t> fut_10n = prom_10n.get_future();

    // Launch two independent threads
    thread t1(FirstThread, n, move(prom_n), move(prom_signal), move(prom_10n));
    thread t2(SecondThread, n, move(fut_signal));

    // Main thread receives values via future
    cout << "[Main Thread] " << n << "th prime number = " << fut_n.get() << "\n";
    cout << "[Main Thread] " << (n * 10) << "th prime number = " << fut_10n.get() << "\n";

    t1.join();
    t2.join();
  }
}

int main()
{
  int choice = 0;
  while (true)
  {
    cout << "1. Task 1.2.1\n";
    cout << "2. Task 1.2.2\n";
    cout << "3. Task 1.2.3\n";
    cout << "4. Task 1.2.4\n";
    cout << "5. Task 1.2.5\n";
    cout << "6. Task 1.2.6\n";
    cout << "7. Task 1.2.7\n";
    cout << "0. Exit\n";
    cout << "Select a menu item: ";

    if (!(cin >> choice))
    {
      break;
    }

    if (choice == 0)
      break;

    switch (choice)
    {
    case 1:
      Task1::run();
      break;
    case 2:
      Task2::run();
      break;
    case 3:
      Task3::run();
      break;
    case 4:
      Task4::run();
      break;
    case 5:
      Task5::run();
      break;
    case 6:
      Task6::run();
      break;
    case 7:
      Task7::run();
      break;
    default:
      cout << "Invalid choice! Please try again.\n";
      break;
    }
  }

  cout << "Program finished execution.\n";
  return 0;
}