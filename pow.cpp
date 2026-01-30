#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "sha1.h"

/**
 * Assumptions:
 * 0 <= difficulty <= 32 (Gets capped at 32)
 * Problem is always 16 bytes
 * Solution will always be 16 bytes
 */
std::string solve(const char *problem, unsigned difficulty) {
    int mask = difficulty >= 32 ? -1 : ((1 << difficulty) - 1) << (32 - difficulty);

    char solution[17];
    solution[16] = '\0';

    uint8_t arr[48];
    uint32_t hash[5];

    memcpy(arr, problem, 16);
    memset(arr + 16, '0', 16);
    memcpy(arr + 32, problem, 16);

    for (uint64_t i = 0; i < MAX; i++) {
        uint64_t tmp = i;
        int iter = 16;
        while (tmp > 0) {
            arr[16 + --iter] = (tmp % 10) + '0';
            tmp /= 10;
        }

        sha1_hash(arr, 48, hash);
        if (!(hash[0] & mask)) {
            memcpy(solution, arr + 16, 16);
            break;
        }
    }

    return std::string(solution);
}

std::string solveParallel(const char *problem, unsigned difficulty) {
    int mask = difficulty >= 32 ? -1 : ((1 << difficulty) - 1) << (32 - difficulty);

    char solution[17];
    solution[16] = '\0';

    unsigned numThreads = std::max<unsigned>(1, std::thread::hardware_concurrency());
    std::atomic<bool> found(false);
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (unsigned t = 0; t < numThreads; t++) {
        threads.emplace_back([t, numThreads, problem, mask, &found, &solution]() {
            uint8_t arr[48];
            uint32_t hash[5];

            memcpy(arr, problem, 16);
            memset(arr + 16, '0', 16);
            memcpy(arr + 32, problem, 16);

            for (uint64_t i = t; i < MAX && !found.load(std::memory_order_relaxed); i += numThreads) {
                uint64_t tmp = i;
                int iter = 16;
                while (tmp > 0) {
                    arr[16 + --iter] = (tmp % 10) + '0';
                    tmp /= 10;
                }

                sha1_hash(arr, 48, hash);
                if (!(hash[0] & mask)) {
                    bool expected = false;
                    if (found.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                        memcpy(solution, arr + 16, 16);
                    }
                    break;
                }
            }
        });
    }

    for (auto &th : threads) th.join();

    return std::string(solution);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <16-char problem> <difficulty>\n", argv[0]);
        return 1;
    }

    const char *problem = argv[1];
    if (strlen(problem) != 16) {
        printf("Error: Problem string must be exactly 16 characters long.\n");
        return 1;
    }

    int difficulty = atoi(argv[2]);
    if (difficulty < 0 || difficulty > 32) {
        printf("Error: Difficulty must be between 0 and 32.\n");
        return 1;
    }

    std::string solution = solve(problem, static_cast<unsigned>(difficulty));

    printf("%s\n", solution.c_str());

    return 0;
}
