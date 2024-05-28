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
 * @File threading.h
 * @Brief useful classes and high-level primitives for multi-threading
 */

#pragma once

#include <vector>
#include <thread>
#include <functional>

/**
 * A thread pool class with blocking run semantics
 *
 * Usage:
 * ThreadPool tp(8); // run on 8 threads
 * tp.run([] (int threadIdx, int threadCnt) { work A here ... });
 * tp.run([] (int threadIdx, int threadCnt) { work B here ... });
 *
 * This arrangement ensures all of work A is completed before work B commences.
 * In the degenerate case of thread count = 1, no threads are ever created - tp.run()
 * simply calls the lambda.
 */
struct WorkerSyncStruct;
class ThreadPool {
	std::vector<std::thread> m_workers;
	std::vector<WorkerSyncStruct*> m_sync;
	std::function<void(int, int)> m_workFunc;
	volatile bool m_exitRequired;

	friend void workerBoilerplate(ThreadPool*, WorkerSyncStruct*);
public:
	ThreadPool(int threadCount);
	~ThreadPool();
	void run(std::function<void(int, int)> worker);
};
