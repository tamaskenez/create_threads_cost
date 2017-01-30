#include <thread>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <string>
#include <atomic>
#include <numeric>
#include <climits>
#include <cmath>
#include <mutex>

#include "chrono.h"


// this program creates a few threads that running continuously, only to load the CPU/mem
// meanwhile very small threads will be created and detached during the test. These threads do very little work then exit quickly.

const int n_blocks = 16; //one thread will operate on this many memory blocks to provide some CPU load
const int c_block_size = 65536;
const int c_test_seconds = 10; // measurement duration
const double thread_create_freq = 300; //this many extra threads (multiplied by the n_threads_created_together) will be created per second
const int n_threads_created_together = 6;

namespace this_thread = std::this_thread;

using std::vector;
using std::atomic;
using std::chrono::seconds;
using std::string;
using std::to_string;
using hrc = high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

atomic<bool> running{true};
atomic<int64_t> total_counter{0};
atomic<int> empty_thread_counter{0};

class Statistics
{
public:
    void reset();

    void add(double d)
    {
	std::lock_guard<std::mutex> lock(mx);
        ++count_;
        sum_ += d;
        sum2_ += d*d;
        if ((std::isnan(lower_)))
            lower_ = upper_ = d;
        else if ((
                     d <
                     lower_))  // note:bad performance for decreasing sequences
            lower_ = d;
        else if ((
                     d >
                     upper_))  // note:bad performance for increasing sequences
            upper_ = d;
        bDirty = true;
    }

    double mean() const
    {
        update_if_needed();
        return mean_;
    }

    double std() const  // normalized with N
    {
        update_if_needed();
        return std_;
    }
    double std_sample() const  // normalized with N-1
    {
        update_if_needed();
        return std_sample_;
    }

    double var() const  // normalized with N
    {
        update_if_needed();
        return var_;
    }
    double var_sample() const  // normalized with N-1
    {
        update_if_needed();
        return var_sample_;
    }

    int count() const { return (int)count_; }
    int64_t count64() const { return count_; }
    double sum() const { return sum_; }
    double sum2() const { return sum2_; }
    double upper() const { return upper_; }
    double lower() const { return lower_; }
    void update() const;

private:
    void update_if_needed() const
    {
        if (bDirty)
            update();
    }
    double sum_{0.0}, sum2_{0.0};
    int64_t count_{0};

    mutable atomic<bool> bDirty{false};
    mutable double mean_ = NAN;
    mutable double lower_ = NAN, upper_ = NAN;
    mutable double std_, std_sample_, var_, var_sample_;

    std::mutex mx;
};

Statistics stet, stt;

void empty_thread_fn(hrc::time_point t0) {
	auto d = duration<double>(hrc::now() - t0).count();
	stet.add(d*1000000.0);
}

void busy_thread_fn(int n_blocks) {
	vector<int> v(n_blocks * c_block_size);
	std::iota(v.begin(), v.end(), 0);
	int64_t counter = n_blocks;
	while(running) {
		for(size_t i = 2; i < v.size(); ++i)
			v[i] = v[i-1] + v[i-2];
		counter += n_blocks;
	}
	total_counter += counter;
}

int main() {
	int hc = std::thread::hardware_concurrency();
	printf("Cores: %d\n", hc);
	vector<std::thread> busy_threads;
	int n_busy_threads = hc - 1;
	printf("Launching %d threads with vectors of %s bytes.\n", n_busy_threads, to_string(n_blocks * c_block_size).c_str());
	auto t0 = hrc::now();

	for(int i = 0; i < n_busy_threads; ++i) {
		busy_threads.emplace_back(&busy_thread_fn, n_blocks);
	}

	if(n_threads_created_together > 0) {
		printf("Creating %d empty threads at %f Hz...\n", n_threads_created_together, thread_create_freq);
		auto frame_time = 1.0 / thread_create_freq;
		int n_frames = (int)(c_test_seconds / frame_time+0.5);
		printf("There will be %d frames = %d empty threads\n", n_frames, n_frames * n_threads_created_together);
		auto dur_frame_time = duration_cast<hrc::duration>(duration<double>(frame_time));
		for(int f = 0; f < n_frames; ++f) {
			for(int i = 0; i < n_threads_created_together; ++i) {
				auto n = hrc::now();
				auto t = std::thread(&empty_thread_fn, n);
				t.detach();
				auto d = duration<double>(hrc::now() - n).count();
				stt.add(d*1000000.0);
			}
			this_thread::sleep_for(dur_frame_time);
		}
		printf("Created %d empty threads\n", stet.count());
	} else {
		printf("Not creating any new threads, sleeping...\n");
		this_thread::sleep_for(seconds(c_test_seconds));
	}


	running = false;
	printf("Joining...\n");
	for(auto &t: busy_threads)
		if(t.joinable())
			t.join();
	auto t1 = hrc::now();
	double elapsed = duration<double>(t1 - t0).count();
	printf("Elapsed: %.f sec\n", elapsed);
	printf("Performance: %fGB/sec\n", (double)(total_counter.load())*c_block_size/elapsed/1024/1024/1024);
	printf("Create thread delay min-avg-max: %f %f %f usec, std: %f usec\n", stet.lower(), stet.mean(), stet.upper(), stet.std());
	printf("Create thread cost min-avg-max: %f %f %f usec, std: %f usec\n", stt.lower(), stt.mean(), stt.upper(), stt.std());
	return EXIT_SUCCESS;
}

void Statistics::reset()
{
    count_ = 0;
    sum_ = sum2_ = 0.0;
    bDirty = false;
    lower_ = upper_ = NAN;
}

inline double square(double x){return x*x; }

void Statistics::update() const
{
    if (count_ > 0) {
        mean_ = sum_ / count_;
        var_ = ((double)count_ * sum2_ - square(sum_)) / (square((double)count_));
        std_ = sqrt(var_);
        if (count_ > 1) {
            var_sample_ =
                (count_ * sum2_ - square(sum_)) / (count_ * (count_ - 1));
            std_sample_ = sqrt(var_sample_);
        } else
            std_sample_ = var_sample_ = NAN;
    } else
        mean_ = std_ = std_sample_ = var_ = var_sample_ = NAN;

    bDirty = false;
}

