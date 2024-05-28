/***************************************************************************
 *   Copyright (C) 2009-2024 by Veselin Georgiev, Slavomir Kaslev,         *
 *                              Deyan Hadzhiev et al                       *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @File threading.cpp
 * @Brief useful classes and high-level primitives for multi-threading
 */

#include "threading.h"
#include <thread>
#include <mutex>
#include <condition_variable>

struct WorkerSyncStruct {
	int threadIdx;
	int state;
	std::condition_variable cv;
	std::mutex m;
	void wait(int desiredState);
	void signal(int newState);
};

void WorkerSyncStruct::wait(int desiredState)
{
	std::unique_lock lk(m);
	cv.wait(lk, [this, desiredState]() { return this->state == desiredState; });
}

void WorkerSyncStruct::signal(int newState)
{
	m.lock();
	state = newState;
	m.unlock();
	cv.notify_one();
}

void workerBoilerplate(ThreadPool* pool, WorkerSyncStruct* sync)
{
	while (1) {
		sync->signal(0);
		sync->wait(1);
		if (pool->m_exitRequired) break;
		pool->m_workFunc(sync->threadIdx, int(pool->m_workers.size()) + 1);
	}
}

ThreadPool::ThreadPool(int n)
{
	n = std::max(1, n);
	m_workers.resize(n - 1);
	m_sync.resize(n - 1);
	m_exitRequired = false;
	for (int i = 0; i < n - 1; i++) {
		m_sync[i] = new WorkerSyncStruct;
		m_sync[i]->state = -1;
		m_sync[i]->threadIdx = i + 1;
		m_workers[i] = std::thread(workerBoilerplate, this, m_sync[i]);
		m_sync[i]->wait(0);
	}
}

ThreadPool::~ThreadPool()
{
	m_exitRequired = true;
	for (int i = 0; i < int(m_workers.size()); i++) {
		m_sync[i]->wait(0);
		m_sync[i]->signal(1);
		m_workers[i].join();
		delete m_sync[i];
	}
}

void ThreadPool::run(std::function<void(int, int)> f)
{
	m_workFunc = f;
	int n = int(m_workers.size());
	for (int i = 0; i < n; i++) m_sync[i]->signal(1);
	m_workFunc(0, n + 1);
	for (int i = 0; i < n; i++) m_sync[i]->wait(0);
}
