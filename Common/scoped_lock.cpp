#include "scoped_lock.h"

scoped_lock::scoped_lock(std::mutex& lock)
: lock_(lock)
{
	lock_.lock();
}

scoped_lock::~scoped_lock()
{
	lock_.unlock();
}

