#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cctype>

using namespace std;

struct LogEvent {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    string type;
    string message;

    void print() const {
        cout << "  " << year << "." 
             << setw(2) << setfill('0') << month << "."
             << setw(2) << setfill('0') << day << " "
             << setw(2) << setfill('0') << hour << ":"
             << setw(2) << setfill('0') << minute
             << " [" << type << "] " << message << endl;
        cout << setfill(' ');
    }

    string toFileFormat() const {
        string result = to_string(year) + "." +
                        (month < 10 ? "0" : "") + to_string(month) + "." +
                        (day < 10 ? "0" : "") + to_string(day) + " " +
                        (hour < 10 ? "0" : "") + to_string(hour) + ":" +
                        (minute < 10 ? "0" : "") + to_string(minute) +
                        " " + type + " " + message;
        return result;
    }
};

bool isNumber(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

int inputNumber(const string& message) {
    string str;
    int number;

    while (true) {
        cout << message;
        getline(cin, str);

        if (!isNumber(str)) {
            cout << "  Ошибка! Нужно ввести число (только цифры)." << endl;
            continue;
        }

        number = 0;
        for (char c : str) {
            number = number * 10 + (c - '0');
        }
        return number;
    }
}

class LogFilter {
private:
    const vector<LogEvent>& events;
    int pos;
    int startY, startM, startD, startH, startMin;
    int endY, endM, endD, endH, endMin;

    int toNumber(int y, int m, int d, int h, int min) {
        return y * 1000000 + m * 10000 + d * 100 + h * 10 + min;
    }

    bool isInRange(const LogEvent& e) {
        int eventDate = toNumber(e.year, e.month, e.day, e.hour, e.minute);
        int startDate = toNumber(startY, startM, startD, startH, startMin);
        int endDate = toNumber(endY, endM, endD, endH, endMin);
        return (eventDate >= startDate && eventDate <= endDate);
    }

    void skipToNext() {
        while (pos < (int)events.size() && !isInRange(events[pos])) {
            pos++;
        }
    }

public:
    LogFilter(const vector<LogEvent>& ev,
              int sy, int sm, int sd, int sh, int smin,
              int ey, int em, int ed, int eh, int emin)
        : events(ev), pos(0),
          startY(sy), startM(sm), startD(sd), startH(sh), startMin(smin),
          endY(ey), endM(em), endD(ed), endH(eh), endMin(emin) {
        skipToNext();
    }

    bool hasNext() {
        return pos < (int)events.size();
    }

    LogEvent getNext() {
        LogEvent result = events[pos];
        pos++;
        skipToNext();
        return result;
    }

    void reset() {
        pos = 0;
        skipToNext();
    }
};

struct StatsResult {
    int total;
    int errors;
    int warnings;
    int info;
    double avgInterval;
};

class LogStats {
private:
    const vector<LogEvent>& allEvents;
    vector<LogEvent> filteredData;
    StatsResult result;
    bool ready;

public:
    LogStats(const vector<LogEvent>& ev) : allEvents(ev), ready(false) {
        result.total = 0;
        result.errors = 0;
        result.warnings = 0;
        result.info = 0;
        result.avgInterval = 0.0;
    }

    void setFilteredData(const vector<LogEvent>& data) {
        filteredData = data;
        ready = false;
    }

    void calculate() {
        if (filteredData.empty()) {
            result.total = 0;
            result.errors = 0;
            result.warnings = 0;
            result.info = 0;
            result.avgInterval = 0.0;
            ready = true;
            return;
        }

        int total = 0, errors = 0, warnings = 0, info = 0;

        for (const auto& e : filteredData) {
            total++;
            if (e.type == "Ошибка") errors++;
            else if (e.type == "Предупреждение") warnings++;
            else if (e.type == "Информация") info++;
        }

        double avgInterval = 0.0;
        if (total > 1) {
            long long totalDiff = 0;
            for (size_t i = 1; i < filteredData.size(); i++) {
                long long t1 = filteredData[i-1].year * 365 * 24 * 60 +
                              filteredData[i-1].month * 30 * 24 * 60 +
                              filteredData[i-1].day * 24 * 60 +
                              filteredData[i-1].hour * 60 +
                              filteredData[i-1].minute;
                long long t2 = filteredData[i].year * 365 * 24 * 60 +
                              filteredData[i].month * 30 * 24 * 60 +
                              filteredData[i].day * 24 * 60 +
                              filteredData[i].hour * 60 +
                              filteredData[i].minute;
                totalDiff += (t2 - t1);
            }
            avgInterval = (double)totalDiff / (total - 1);
        }

        result.total = total;
        result.errors = errors;
        result.warnings = warnings;
        result.info = info;
        result.avgInterval = avgInterval;
        ready = true;
    }

    StatsResult getResult() const {
        return result;
    }

    bool isReady() const {
        return ready;
    }

    void printStats() const {
        cout << "  СТАТИСТИКА" << endl;
        cout << "  Всего событий: " << result.total << endl;
        cout << "  Ошибки: " << result.errors << endl;
        cout << "  Предупреждения: " << result.warnings << endl;
        cout << "  Информация: " << result.info << endl;
        if (result.total > 1) {
            cout << "  Средний интервал: " 
                 << fixed << setprecision(2) << result.avgInterval << " мин." << endl;
        } else {
            cout << "  Средний интервал: недостаточно данных" << endl;
        }
        cout << "========================================" << endl;
    }
};

vector<LogEvent> loadLogs(const string& filename) {
    vector<LogEvent> logs;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cout << "  Файл не найден." << endl;
        return logs;
    }

    string line;
    int loaded = 0;
    
    while (getline(file, line)) {
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) {
            line.erase(0, 1);
        }
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        size_t pos1 = line.find('.');    
        size_t pos2 = line.find('.', pos1 + 1);  
        size_t pos3 = line.find(' ');    
        size_t pos4 = line.find(':');    
        size_t pos5 = line.find(' ', pos4 + 1);  
        
        if (pos1 == string::npos || pos2 == string::npos || 
            pos3 == string::npos || pos4 == string::npos || 
            pos5 == string::npos) {
            continue;
        }
        
        // Разбираем строку вручную
        string yearStr = line.substr(0, pos1);
        string monthStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
        string dayStr = line.substr(pos2 + 1, pos3 - pos2 - 1);
        
        string hourStr = line.substr(pos3 + 1, pos4 - pos3 - 1);
        string minStr = line.substr(pos4 + 1, pos5 - pos4 - 1);
        
        string type = "";
        string message = "";
        size_t pos6 = line.find(' ', pos5 + 1);
        if (pos6 == string::npos) {
            type = line.substr(pos5 + 1);
        } else {
            type = line.substr(pos5 + 1, pos6 - pos5 - 1);
            message = line.substr(pos6 + 1);
        }
        
        try {
            LogEvent e;
            e.year = stoi(yearStr);
            e.month = stoi(monthStr);
            e.day = stoi(dayStr);
            e.hour = stoi(hourStr);
            e.minute = stoi(minStr);
            e.type = type;
            e.message = message;
            logs.push_back(e);
            loaded++;
        } catch (...) {
        }
    }

    file.close();
    cout << "  Загружено " << loaded << " событий." << endl;
    return logs;
}

void saveFilteredLogs(const string& filename, const vector<LogEvent>& logs) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "  Ошибка: не удалось создать файл!" << endl;
        return;
    }

    for (const auto& e : logs) {
        file << e.toFileFormat() << endl;
    }
    file.close();
    cout << "  Сохранено " << logs.size() << " событий в файл " << filename << endl;
}

void printMenu() {
    cout << "  Многопоточный анализатор логов" << endl;
    cout << "  1. Загрузить файл с логами" << endl;
    cout << "  2. Задать интервал и применить фильтр" << endl;
    cout << "  3. Показать отфильтрованные логи" << endl;
    cout << "  4. Показать статистику" << endl;
    cout << "  5. Сохранить отфильтрованные логи" << endl;
    cout << "  6. Выход" << endl;
    cout << "=================================" << endl;
    cout << "Выберите пункт: ";
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDaysInMonth(int year, int month) {
    if (month == 2) {
        return isLeapYear(year) ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

bool isValidDate(int year, int month, int day, int hour, int minute) {
    if (year < 2000 || year > 2100) {
        cout << "  Ошибка: год должен быть от 2000 до 2100." << endl;
        return false;
    }
    if (month < 1 || month > 12) {
        cout << "  Ошибка: месяц должен быть от 1 до 12." << endl;
        return false;
    }
    if (day < 1 || day > getDaysInMonth(year, month)) {
        cout << "  Ошибка: в этом месяце " << getDaysInMonth(year, month) << " дней." << endl;
        return false;
    }
    if (hour < 0 || hour > 23) {
        cout << "  Ошибка: час должен быть от 0 до 23." << endl;
        return false;
    }
    if (minute < 0 || minute > 59) {
        cout << "  Ошибка: минута должна быть от 0 до 59." << endl;
        return false;
    }
    return true;
}
int main() {
    vector<LogEvent> allLogs;
    vector<LogEvent> filteredLogs;
    string currentFile = "";
    LogStats* stats = nullptr;
    bool hasFilter = false;

    cout << "  Многопоточный анализатор логов" << endl;
    cout << "  Загрузка и фильтрация логов" << endl;

    while (true) {
        printMenu();

        string input;
        getline(cin, input);
        int choice = 0;

        bool isNum = true;
        for (char c : input) {
            if (!isdigit(c)) isNum = false;
        }
        if (isNum && !input.empty()) {
            choice = stoi(input);
        }

        if (choice == 6) {
            cout << "\nВыход из программы." << endl;
            break;
        }

        switch (choice) {
            case 1: {
                cout << "\nВведите имя файла (например, logs.txt): ";
                string filename;
                getline(cin, filename);
                allLogs = loadLogs(filename);
                currentFile = filename;
                hasFilter = false;
                if (stats) {
                    delete stats;
                    stats = nullptr;
                }
                break;
            }
            case 2: {
                if (allLogs.empty()) {
                    cout << "  Сначала загрузите файл с логами (пункт 1)." << endl;
                    break;
                }

                cout << "\nВведите интервал дат:" << endl;

                int sy, sm, sd, sh, smin;
                int ey, em, ed, eh, emin;

                // Ввод и проверка начала интервала
                while (true) {
                    cout << "  Начало интервала:" << endl;
                    sy = inputNumber("    Год (2000-2100): ");
                    sm = inputNumber("    Месяц (1-12): ");
                    sd = inputNumber("    День: ");
                    sh = inputNumber("    Час (0-23): ");
                    smin = inputNumber("    Минута (0-59): ");

                    if (isValidDate(sy, sm, sd, sh, smin)) {
                        break;
                    }
                    cout << "  Попробуйте снова.\n" << endl;
                }

                // Ввод и проверка конца интервала
                while (true) {
                    cout << "  Конец интервала:" << endl;
                    ey = inputNumber("    Год (2000-2100): ");
                    em = inputNumber("    Месяц (1-12): ");
                    ed = inputNumber("    День: ");
                    eh = inputNumber("    Час (0-23): ");
                    emin = inputNumber("    Минута (0-59): ");

                    if (!isValidDate(ey, em, ed, eh, emin)) {
                        cout << "  Попробуйте снова.\n" << endl;
                        continue;
                    }

                    int startNum = sy * 1000000 + sm * 10000 + sd * 100 + sh * 10 + smin;
                    int endNum = ey * 1000000 + em * 10000 + ed * 100 + eh * 10 + emin;

                    if (startNum > endNum) {
                        cout << "  Ошибка: дата начала не может быть позже даты конца!" << endl;
                        cout << "  Попробуйте снова.\n" << endl;
                        continue;
                    }

                    break;
                }

                LogFilter filter(allLogs, sy, sm, sd, sh, smin, ey, em, ed, eh, emin);

                filteredLogs.clear();
                while (filter.hasNext()) {
                    filteredLogs.push_back(filter.getNext());
                }

                cout << "\n  Найдено событий: " << filteredLogs.size() << endl;
                hasFilter = true;

                if (stats) {
                    delete stats;
                    stats = nullptr;
                }
                stats = new LogStats(allLogs);
                stats->setFilteredData(filteredLogs);
                break;
            }

            case 3: {
                if (!hasFilter || filteredLogs.empty()) {
                    cout << "  Нет отфильтрованных данных.Сначала примените фильтр (пункт 2)." << endl;
                    break;
                }

                cout << "\nОтфильтрованные события:" << endl;
                cout << "----------------------------------------" << endl;
                for (const auto& e : filteredLogs) {
                    e.print();
                }
                cout << "----------------------------------------" << endl;
                cout << "  Всего: " << filteredLogs.size() << endl;
                break;
            }

            case 4: {
                if (!hasFilter || filteredLogs.empty()) {
                    cout << "  Нет данных для статистики.Сначала примените фильтр (пункт 2)." << endl;
                    break;
                }

                if (stats) {
                    stats->calculate();
                    stats->printStats();
                }
                break;
            }

            case 5: {
                if (!hasFilter || filteredLogs.empty()) {
                    cout << " Нет данных для сохранения.Сначала примените фильтр (пункт 2)." << endl;
                    break;
                }

                string filename = "filtered_logs.txt";
                cout << " Введите имя файла (по умолчанию filtered_logs.txt): ";
                string inputFile;
                getline(cin, inputFile);
                if (!inputFile.empty()) {
                    filename = inputFile;
                }
                saveFilteredLogs(filename, filteredLogs);
                break;
            }

            default: {
                cout << " Неизвестная команда. Выберите пункт от 1 до 6." << endl;
                break;
            }
        }
    }

    if (stats) {
        delete stats;
    }

    return 0;
}