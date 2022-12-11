# Отчёт по ИДЗ 4

## Об отчёте

**Выполнил:** Каверин Максим Вячеславович

**Группа:** БПИ217

**Вариант:** 29

**Задание:** **Задача о Пути Кулака.** На седых склонах Гималаев стоит древний буддистский монастырь: Гуань-Инь-Янь. Каждый год в день сошествия на землю боддисатвы Монахи монастыря собираются на совместное празднество и показывают свое совершенствование на Пути Кулака. Всех соревнующихся монахов разбивают на пары, победители пар бьются затем между собой и так далее, до финального поединка. Монах который победил в финальном бою, забирает себе на хранение статую боддисатвы. Реализовать многопоточное приложение, определяющего победителя. В качестве входных данных используется массив, в котором хранится количество энергии Ци каждого монаха. При победе монах забирает энергию Ци своего противника. Разбивка на пары перед каждым сражением осуществляется случайным образом. Монах, оставшийся без пары, удваивает свою энергию, отдохнув от поединка. При решении использовать принцип дихотомии.

Данный отчёт разбит на блоки по оценкам для удобства проверки. Работа была выполнена с учетом требований до оценки 9 включительно.

Все файлы, относящиеся к решению, находятся в папке **files**.

Примеры выполнения находятся в папке **examples**.

## Код программы на C++

Для решения задачи был написан файл **main.cpp**:

```cpp
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
// Мьютекс
pthread_mutex_t mutex;

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

    // Блокирование мьютекса
    pthread_mutex_lock(&mutex);

    // Вывод данных о битве
    std::cout << "Monk " << pair_ptr[0].first << " and monk " << pair_ptr[1].first << " have fought! "
              << "Monk " << winner.first << " was victorious and now has " << winner.second << " qi." << std::endl;

    ofs << "Monk " << pair_ptr[0].first << " and monk " << pair_ptr[1].first << " have fought! "
        << "Monk " << winner.first << " was victorious and now has " << winner.second << " qi." << std::endl;
    
    // Сохранение победителя
    next_monks.push_back(winner);

    // Разблокирование мьютекса
    pthread_mutex_unlock(&mutex);

    return nullptr;
}

int main(int argc, char** argv) {
    // Генератор случайных чисел для выбора пар для сражений
    auto shuffle_rng = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());

    // Инициализация мьютекса
    pthread_mutex_init(&mutex, nullptr);

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

    // Уничтожения мьютекса
    pthread_mutex_destroy(&mutex);

    return 0;
}
```

## На оценку 4

- При разработке программы использовался принцип дихотомии, т.е. вычисления в программе проводятся путем слияния результатов двух потоков в новый поток, каждый раз уменьшая их количество в два раза. В данном случае результатом выполнения потока является монах (победитель), полученный на основе двух монахов, поданных потоку на вход.
- Входными данными являются:
	- Число монахов: от 2 до 100
	- Значение Ци каждого монаха: от 1 до 1000
- Приложение было реализовано с использованием мьютексов.

## На оценку 5
- В программу были добавлены комментарии.
- В данной задаче сущностями являются монахи, которые параллельно друг другу осуществляют попарные бои, т.е. если есть, например, монахи 1, 2, 3 и 4, то битва между монахами 1 и 2 может осуществляться параллельно битве монахов 3 и 4.

## На оценку 6, 7 и 8
- Битвы монахов были реализованы с помощью функции `thread_function`. Она получает на вход двух монахов, среди которых определяется победитель и его значение Ци после битвы. Полученный монах добавляется в вектор `next_monks`. Это вектор монахов, попавших в следующий раунд. Проигравший же монах никуда не добавляется. Если количество монахов не делится на 2, то оставшийся монах сразу добавляется в вектор `next_monks`, но с удвоенным значением Ци. Процесс попарных битв осуществляется до тех пор, пока не останется единственный монах. Этот монах и является победителем турнира. 
 - Был реализован следующий функционал:
	 - Задание файлов ввода и вывода (происходит, если в командной строке два параметра, это пути до файла ввода и вывода соответственно).
	 - Задание случайного входного числа (происходит, если в командной строке один параметр, это seed).
	- Если файл вывода не задан, то результат выполнения по умолчанию записывается в **output.txt**.
- Примеры работы приложения находятся в папке **examples**.

## На оценку 9

- Из программы были убраны синхропримитивы, получился файл **main_modified.cpp**.
- При вызове программы с четырьмя монахами с Ци 1, 2, 3 и 4 был получен следующий некорректный результат: 

```
Monk 3 and monk 2 have fought! Monk 3 was victorious and now has 5 qi.
Monk 1 and monk 4 have fought! Monk 4 was victorious and now has 5 qi.
Monk 4 is the winner of the tournament!
```

- Этот результат обусловлен тем, что два потока попытались одновременно добавить монаха 3 и монаха 4 в вектор `next_monks`, что привело к тому, что в векторе оказался только один монах, вместо ожидаемых двух.
