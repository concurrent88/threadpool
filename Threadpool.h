#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <exception>
#include <functional>

namespace raj
{
	using Job = std::function<void()>;

	class Threadpool
	{
	public:
		Threadpool(uint32_t numberOfThreads = std::thread::hardware_concurrency() * 2);
		~Threadpool();

		void enque(Job job, bool front = false);
		void stop(bool drain = false);

		void exception();

	private:
		std::deque<Job> _jobs;
		std::vector<std::thread> _workers;

		std::deque<std::exception_ptr> _exceptions;

		std::mutex _lock;
		std::condition_variable _condition;
	};
}