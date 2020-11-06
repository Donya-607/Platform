#pragma once

#include <memory>
#include <mutex>
#include <thread>

struct Thread
{
public:
	class Result
	{
	private:
		bool finished  = false;
		bool succeeded = false;
		mutable std::mutex mutex;
	public:
		void WriteResult( bool wasSucceeded )
		{
			std::lock_guard<std::mutex> lock( mutex );
			finished  = true;
			succeeded = wasSucceeded;
		}
		bool Finished() const
		{
			std::lock_guard<std::mutex> lock( mutex );
			return finished;
		}
		bool Succeeded() const
		{
			std::lock_guard<std::mutex> lock( mutex );
			return succeeded;
		}
	};
public:
	Result result;
	std::unique_ptr<std::thread> pThread = nullptr;
public:
	void JoinThenRelease()
	{
		if ( !pThread ) { return; }
		// else

		if ( pThread->joinable() )
		{
			pThread->join();
		}

		pThread.reset();
	}
};
