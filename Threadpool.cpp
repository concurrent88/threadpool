#include "Threadpool.h"

using namespace raj;

Threadpool::Threadpool(uint32_t numberOfThreads)
{
	_workers.reserve(numberOfThreads);
	for(size_t i = 0; i < numberOfThreads; ++i)
	{
		_workers.emplace_back(std::thread(
		[this]()
		{
			while(true)
			{
				Job job;
				{
					std::unique_lock<std::mutex> uniqueLock(_lock);
					_condition.wait(uniqueLock, [this]() { return !_jobs.empty(); });
					job = _jobs.front();
					_jobs.pop_front();
				}

				if (job == nullptr)
				{
					return;
				}
				else
				{
					try
					{
						job();
					}
					catch (...)
					{
						std::lock_guard<std::mutex> guard{ _lock };
						_exceptions.emplace_back(std::current_exception());
					}
				}
			}
		}));
	}
}

void Threadpool::enque(Job job, bool front)
{
	{
		std::lock_guard<std::mutex> guard{ _lock };
		if (front)
			_jobs.emplace_front(job);
		else
			_jobs.emplace_back(job);
	}
	_condition.notify_one();
}
void Threadpool::stop(bool drain)
{
	if (_workers.empty())
		return;

	{
		std::lock_guard<std::mutex> guard{ _lock };
		if (drain)
		{
			_jobs.clear();
		}
		for (size_t i = 0; i < _workers.size(); ++i)
		{
			_jobs.emplace_front(nullptr);
		}
	}

	_condition.notify_all();
	for (auto& worker : _workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}
}

Threadpool::~Threadpool()
{
	stop();
}

void Threadpool::exception()
{
	std::lock_guard<std::mutex> guard{ _lock };
	auto excep = _exceptions.front();
	_exceptions.pop_front();

	std::rethrow_exception(excep);
}