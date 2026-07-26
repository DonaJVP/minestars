/*
 M * * i*neStars - MultiCraft - Minetest/Luanti
 Copyright (C) 2025 Logiki, <donatto555@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class MultithreadQueue {
public:
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(_mutex); // Acquire lock
        _queue.push(item);
        //_cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(_mutex); // Acquire lock
        //_cv.wait(lock, [&]{ return !_queue.empty(); }); // Wait if queue is empty
        T item = _queue.front();
        _queue.pop();
        return item;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    std::queue<T> _queue;
    mutable std::mutex _mutex; // Mutable for const methods like empty()
    //std::condition_variable _cv;
};
