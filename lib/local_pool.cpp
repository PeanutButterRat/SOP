#include "local_pool.hpp"
// static bool isPrinted = false;
// static bool isPrintedLast = false;
local_pool::local_pool(int thread_count)
{
    locks = std::vector<spin_lock>(thread_count);
    pools = std::vector<std::deque<std::deque<path_node>>>(thread_count);
    depths = std::vector<int>(thread_count);
    this->thread_count = thread_count;
}

// void local_pool::print_top_sequence_sizes_end(int thread_total)
// {
//     std::vector<int> results;
//     results.reserve(thread_total);

//     if (!isPrintedLast)
//     {
//         isPrintedLast = true;
//         for (int i = 0; i < thread_total; i++)
//         {
//             locks[i].lock(); // Lock the pool to ensure thread safety

//             if (pools[i].size() > 0)
//             {
//                 while (pools[i].front().empty() && pools[i].size() > 1)
//                 {
//                     pools[i].pop_front();
//                 }
//                 if (pools[i].size() <= 1 && pools[i].front().empty())
//                 {
//                     results.push_back(-2); // Top deque is empty
//                 }
//                 else
//                 {
//                     // Access the top deque and get the first element in it
//                     const path_node &top_node = pools[i].front().front();
//                     // Add the size of the sequence to results
//                     results.push_back(top_node.sequence.size());
//                 }
//             }
//             else
//             {
//                 results.push_back(-1); // Pool is empty
//             }

//             locks[i].unlock(); // Unlock the pool after accessing
//         }

//         // Print all collected results as a comma-separated string
//         for (size_t i = 0; i < results.size(); ++i)
//         {
//             std::cout << results[i];
//             if (i < results.size() - 1)
//                 std::cout << ",";
//         }
//         std::cout << std::endl;
//     }
// }

// void local_pool::print_top_sequence_sizes(int thread_total)
// {
//     std::vector<int> results;
//     results.reserve(thread_total);

//     if (!isPrinted)
//     {
//         isPrinted = true;
//         for (int i = 0; i < thread_total; i++)
//         {
//             locks[i].lock(); // Lock the pool to ensure thread safety

//             if (pools[i].size() > 0)
//             {
//                 while (pools[i].front().empty() && pools[i].size() > 1)
//                 {
//                     pools[i].pop_front();
//                 }
//                 if (pools[i].size() <= 1 && pools[i].front().empty())
//                 {
//                     results.push_back(-2); // Top deque is empty
//                 }
//                 else
//                 {
//                     // Access the top deque and get the first element in it
//                     const path_node &top_node = pools[i].front().front();
//                     // Add the size of the sequence to results
//                     results.push_back(top_node.sequence.size());
//                 }
//             }
//             else
//             {
//                 results.push_back(-1); // Pool is empty
//             }

//             locks[i].unlock(); // Unlock the pool after accessing
//         }

//         // Print all collected results as a comma-separated string
//         for (size_t i = 0; i < results.size(); ++i)
//         {
//             std::cout << results[i];
//             if (i < results.size() - 1)
//                 std::cout << ",";
//         }
//         std::cout << std::endl;
//     }
// }

bool local_pool::pop_from_zero_list(int thread_number, path_node &result_node, int stealing_thread)
{
    if (pools[thread_number].size() <= 1)
    {
        return false;
    }
    locks[thread_number].lock();

    while (pools[thread_number].front().empty() && pools[thread_number].size() > 1)
    {
        pools[thread_number].pop_front();
        depths[thread_number]++;
    }

    if (pools[thread_number].size() <= 1)
    {
        locks[thread_number].unlock();
        return false;
    }

    result_node = pools[thread_number].front().back();
    pools[thread_number].front().pop_back();
    depths[stealing_thread] = depths[thread_number] + 1;

    if (pools[thread_number].front().empty())
    {
        pools[thread_number].pop_front();
        depths[thread_number]++;
    }

    locks[thread_number].unlock();
    return true;
};

bool local_pool::pop_from_active_list(int thread_number, path_node &result_node)
{

    if (pools[thread_number].size() == 0)
        return false;

    if (pools[thread_number].size() == 0 || pools[thread_number].back().empty())
    {
        if (pools[thread_number].size() == 1)
        {
            locks[thread_number].unlock();
        }
        return false;
    }

    result_node = pools[thread_number].back().back();
    pools[thread_number].back().pop_back();

    return true;
};

void local_pool::push_list(int thread_number, std::deque<path_node> list)
{
    locks[thread_number].lock();

    pools[thread_number].push_back(list);

    locks[thread_number].unlock();
};

void local_pool::pop_active_list(int thread_number)
{
    locks[thread_number].lock();
    if (pools[thread_number].size() > 0)
        pools[thread_number].pop_back();
    locks[thread_number].unlock();
};

bool local_pool::out_of_work(int thread_number)
{
    return pools[thread_number].size() == 0;
};

int local_pool::choose_victim(int thread_number, int stolen_from) {
    int lowest_bound = INT_MAX;
    int victim = -1;
 
    for (int thread = 0; thread < thread_count; thread++) {
        if (thread == thread_number || (stolen_from & (1 << thread)) != 0) {
            continue;
        }

        locks[thread].lock();
    
        for (auto& depth : pools[thread]) {
            for (auto& node : depth) {
                if (node.lower_bound < lowest_bound) {
                    lowest_bound = node.lower_bound;
                    victim = thread;
                } 
            }
        }

        locks[thread].unlock();
    }

    return victim;
}


int local_pool::active_pool_size(int thread_number)
{ // TODO: this is not strictly necessary
    return pools[thread_number].back().size();
}

void local_pool::print()
{
    for (int i = 0; i < pools.size(); i++)
    {
        std::cout << pools[i].size() << ", ";
    }
    std::cout << std::endl;
}

void local_pool::set_pool_depth(int thread_id, int depth)
{
    depths[thread_id] = depth;
}