#pragma once
#include <mutex>

class scoped_lock
{
public:
	scoped_lock(std::mutex& lock);
	~scoped_lock();

private:
	std::mutex& lock_;
};

