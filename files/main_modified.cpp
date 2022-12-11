#include <iostream>
#include <random>
#include <chrono>
#include <algorithm>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <utility>

// Поток вывода в файл
std::ofstream ofs;
// Текущие монахи
std::vector<std::pair<uint64_t, uint64_t>> monks;
// Монахи после попарных сражений
std::vector<std::pair<uint64_t, uint64_t>> next_monks;

// Функция сражения двух монахов
void *thread_function(void *ptr) {
    auto *pair_ptr= static_cast<std::pair<uint64_t, uint64_t> *>(ptr);

    // Вычисление победителя
    std::pair<uint64_t, uint64_t> winner;
    if (pair_ptr[0].second >= pair_ptr[1].second) {
        winner.first = pair_ptr[0].first;
    } else {
        winner.first = pair_ptr[1].first;
    }
    winner.second = pair_ptr[0].second + pair_ptr[1].second;

    // Вывод данных о битве
    std::cout << "Monk " << pair_ptr[0].first << " and monk " << pair_ptr[1].first << " have fought! "
              << "Monk " << winner.first << " was victorious and now has " << winner.second << " qi." << std::endl;

    ofs << "Monk " << pair_ptr[0].first << " and monk " << pair_ptr[1].first << " have fought! "
        << "Monk " << winner.first << " was victorious and now has " << winner.second << " qi." << std::endl;
    
    // Сохранение победителя
    next_monks.push_back(winner);

    return nullptr;
}

int main(int argc, char** argv) {
    // Генератор случайных чисел для выбора пар для сражений
    auto shuffle_rng = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());

    if (argc == 1) {
        // Ввод из консоли
        
        std::cout << "Enter number of monks: ";

        size_t number_of_monks;
        std::cin >> number_of_monks;

        std::cout << "Enter qi of all monks: ";
        for (size_t i = 0; i < number_of_monks; i++) {
            uint64_t qi;
            std::cin >> qi;
            monks.emplace_back(i + 1, qi);
        }

        ofs.open("output.txt", std::ofstream::out | std::ofstream::trunc);
    } else if (argc == 2) {
        // Случайная генерация
        
        auto monks_rng = std::mt19937(std::stoull(argv[1]));
        
        std::uniform_int_distribution<size_t> monk_number_distribution(2, 100);

        size_t number_of_monks = monk_number_distribution(monks_rng);

        std::uniform_int_distribution<size_t> qi_distribution(1, 1000);

        for (size_t i = 0; i < number_of_monks; i++) {
            monks.emplace_back(i + 1, qi_distribution(monks_rng));
        }
        
        std::cout << "Number of monks: " << number_of_monks << std::endl;
        std::cout << "Qi of monks: ";
        for (const std::pair<long unsigned int, long unsigned int> &monk : monks) {
             std::cout << monk.second << " ";
        }
        std::cout << std::endl;

        ofs.open("output.txt", std::ofstream::out | std::ofstream::trunc);
    } else if (argc == 3) {
        // Ввод из файла
        
        std::ifstream ifs(argv[1]);

        size_t number_of_monks;
        ifs >> number_of_monks;

        for (size_t i = 0; i < number_of_monks; i++) {
            uint64_t qi;
            ifs >> qi;
            monks.emplace_back(i + 1, qi);
        }

        ofs.open(argv[2], std::ofstream::out | std::ofstream::trunc);
    } else {
        return -1;
    }

    while (monks.size() > 1) {
        // Выбор пар для сражения
        std::shuffle(monks.begin(), monks.end(), shuffle_rng);
        
        // Запуск параллельных сражений
        std::vector<pthread_t> threads;
        for (size_t i = 0; i < monks.size() / 2; i++) {
            pthread_t thread;
            pthread_create(&thread, nullptr, thread_function, &monks[2 * i]);
            threads.push_back(thread);
        }

        // Ожидание окончания всех сражений
        for (pthread_t thread : threads) {
            pthread_join(thread, nullptr);
        }

        // Если есть монах без пары, то он отдыхает
        if (monks.size() % 2 != 0) {
            uint64_t number = monks[monks.size() - 1].first;
            uint64_t qi = monks[monks.size() - 1].second * 2;
            next_monks.emplace_back(number, qi);
            std::cout << "Monk " << number << " has rested and now has " << qi << " qi." << std::endl;
            ofs << "Monk " << number << " has rested and now has " << qi << " qi." << std::endl;
        }

        // Замена монахов новыми
        std::swap(monks, next_monks);
        next_monks.clear();
    }

    // Вывод данных о победителе турнира
    std::cout << "Monk " << monks[0].first << " is the winner of the tournament!" << std::endl;
    ofs << "Monk " << monks[0].first << " is the winner of the tournament!" << std::endl;

    return 0;
}
