#include <iostream>
#include <fstream>  
#include <sstream>
#include <string>            
#include <vector>
#include <map>
#include <queue>
#include <algorithm>

using namespace std; 
  
struct time24 //структура для времени
{
    int hours; //часы
    int minutes; // минуты

    time24() : hours(0), minutes(0) {}
    time24(int h, int m) : hours(h), minutes(m) {}

    string toString() const //преобразование в строку
    { //преобразование в строку
        char buffer[6];  // HH:MM + нулевой символ
        snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
        return string(buffer);
    }

    bool isValid() const //подходит ли время по формату
    { 
        return (hours >= 0 && hours < 24) && 
               (minutes >= 0 && minutes < 60);
    }

    bool operator<(const time24& other) const //сравнение времён
    {
        if (hours != other.hours)
            return hours < other.hours;
        return minutes < other.minutes;
    }

    bool operator<=(const time24& other) const {
        return !(other < *this);
    }
    
    bool operator==(const time24& other) const
    {
        return hours == other.hours && minutes == other.minutes;
    }

    static time24 addTimes(const time24& t1, const time24& t2)
    {
        int totalMinutes = t1.hours * 60 + t1.minutes + t2.hours * 60 + t2.minutes;
        totalMinutes %= 1440; // 1440 минут = 24 часа
        if (totalMinutes < 0) totalMinutes += 1440; // коррекция при отрицательном времени
        
        return {totalMinutes / 60, totalMinutes % 60};
    }

    
    static time24 subtractTimes(const time24& t1, const time24& t2) // вычитание двух времён
    {
        int totalMinutes = (t1.hours * 60 + t1.minutes) - (t2.hours * 60 + t2.minutes);
        totalMinutes %= 1440;
        if (totalMinutes < 0) totalMinutes += 1440;
        
        return {totalMinutes / 60, totalMinutes % 60};
    }

    
    time24 operator+(const time24& other) const // перегруженные операторы для сложения и вычитания
    {
        return addTimes(*this, other);
    }

    int operator-(const time24& other) const {
        return (hours - other.hours) * 60 + (minutes - other.minutes);
    }
};

struct Event //структура для события
{
    time24 time;
    int id;
    vector <string> args;

    Event(const string& line) {
        istringstream iss(line);
        string timeStr;
        iss >> timeStr >> id;
        //time = parseTime(timeStr);
        time.hours = stoi(timeStr.substr(0,2));
        time.minutes = stoi(timeStr.substr(3,2));
        string arg;
        while (iss >> arg)
        {
            args.push_back(arg);
        }
    }

    string toString() const {
        stringstream ss;
        ss << time.toString() << " " << id;
        for (const auto& arg : args) {
            ss << " " << arg;
        }
        return ss.str();
    }
};


time24 previous_time = {0, 0}; //время последнего события
time24 open; //время открытия клуба
time24 closed; //время закрытия клуба
int comps_quantity; //количество компьютеров в клубе
int price; // стоимость часа

bool isValidClientName(const string& name)
{
    if (name.empty())
        return false;
    for (char c : name)
    {   
        //cout << c << endl;
        if (!(islower(c) || isdigit(c)) && c != '_' && c != '-')
            return false;
    }
    return true;
}

struct ComputerClub //структура комп клуба
 {
    int tableCount;
    time24 openTime;
    time24 closeTime;
    int hourCost;
    map<string, int> clientToTable;
    map<int, string> tableToClient;
    map<int, int> tableRevenue;
    map<int, int> tableMinutes;
    map<int, time24> tableStartTime;
    queue<string> waitingQueue;
    vector<string> output;

    ComputerClub(int tables, time24 open, time24 close, int cost) : tableCount(tables), openTime(open), closeTime(close), hourCost(cost)
    {
        for (int i = 1; i <= tableCount; ++i)
        {
            tableRevenue[i] = 0;
        }
        for (int i = 1; i <= tableCount; ++i)
        {
            tableMinutes[i] = 0;
        }
    }

    bool isClientInClub(const string& client) const //проверка в клубе ли клиент
    {
        return clientToTable.count(client) > 0;
    }

    bool isTableOccupied(int table) const //проверка занят ли стол
    {
        return tableToClient.count(table) > 0;
    }

    int countFreeTables() const //подсчёт свободных столов
    {
        return tableCount - tableToClient.size();
    }

    void handleClientArrived(const Event& event) //клиент пришёл в клуб
    {
        const string& client = event.args[0];
        if (isClientInClub(client))
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 YouShallNotPass");
            return;
        }
        if (event.time < openTime || closeTime <= event.time)
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 NotOpenYet");
            return;
        }
        clientToTable[client] = -1; // -1 значит клиент в клубе, но не за столом
        output.push_back(event.toString());
    }

    void handleClientSat(const Event& event) //клиент сел за стол
    {
        const string& client = event.args[0];
        int table = stoi(event.args[1]);
        if (!isClientInClub(client))
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 ClientUnknown");
            return;
        }
        if (table < 1 || table > tableCount)
        {
            throw invalid_argument("Invalid table number");
        }
        
        //if (isTableOccupied(table) && tableToClient[table] != client)                                   //???????//
        if (isTableOccupied(table))                                   
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 PlaceIsBusy");
            return;
        }

        //если клиент был за другим столом, освободить его
        int oldTable = clientToTable[client];
        calculateRevenue(oldTable, event.time);                                            /////////
        if (oldTable != -1 && oldTable != table)
        {
            tableToClient.erase(oldTable);
            //calculateRevenue(oldTable, event.time);
        }

        clientToTable[client] = table;
        tableToClient[table] = client;
        tableStartTime[table] = event.time;
        output.push_back(event.toString());
    }

    void handleClientWaiting(const Event& event) //клиент ждёт
    {
        const string& client = event.args[0];
        if (!isClientInClub(client))
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 ClientUnknown");
            return;
        }
        if (countFreeTables() > 0)
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 ICanWaitNoLonger!");
            return;
        }
        if (waitingQueue.size() >= tableCount)
        {
            clientToTable.erase(client);
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 11 " + client);
            return;
        }
        waitingQueue.push(client);
        output.push_back(event.toString());
    }

    void handleClientLeft(const Event& event) //клиент ушёл
    {
        const string& client = event.args[0];
        if (!isClientInClub(client))
        {
            output.push_back(event.toString());
            output.push_back(event.time.toString() + " 13 ClientUnknown");
            return;
        }

        output.push_back(event.toString());
        int table = clientToTable[client];
        clientToTable.erase(client);

        if (table != -1)
        {
            tableToClient.erase(table);
            calculateRevenue(table, event.time);

            if (!waitingQueue.empty())
            {
                string nextClient = waitingQueue.front();
                waitingQueue.pop();
                clientToTable[nextClient] = table;
                tableToClient[table] = nextClient;
                tableStartTime[table] = event.time;
                output.push_back(event.time.toString() + " 12 " + nextClient + " " + to_string(table));
            }
        }
    }

    void calculateRevenue(int table, const time24& endTime) //подсчёт выручки
    {
        time24 startTime = tableStartTime[table];
        int minutes = endTime - startTime;
        //cout << table << " " << minutes << endl;
        tableMinutes[table] += minutes;
        int hours = minutes / 60;
        if (minutes % 60 != 0) hours++;
        tableRevenue[table] += hours * hourCost;
    }

    void processEvent(const Event& event) //обработка событий
    {
        switch (event.id)
        {
            case 1: handleClientArrived(event); break;
            case 2: handleClientSat(event); break;
            case 3: handleClientWaiting(event); break;
            case 4: handleClientLeft(event); break;
            default: throw invalid_argument("Unknown event ID");
        }
    }

    void endOfDay() //конец дня
    {
        // клиенты в алфавитном порядке
        vector<string> remainingClients;
        for (const auto& pair : clientToTable) {
            remainingClients.push_back(pair.first);
        }
        sort(remainingClients.begin(), remainingClients.end());

        for (const string& client : remainingClients) {
            int table = clientToTable[client];
            if (table != -1) {
                calculateRevenue(table, closeTime);
                tableToClient.erase(table);
            }
            output.push_back(closeTime.toString() + " 11 " + client);
        }
        clientToTable.clear();
    }

    const vector<string>& getOutput() const
    {
        return output;
    }

    vector<pair<int, pair<int, time24>>> getTableStats() const
    {
        vector<pair<int, pair<int, time24>>> stats;
        for (int i = 1; i <= tableCount; ++i) {
            int revenue = tableRevenue.at(i);
            time24 usage(0, 0);
            if (tableStartTime.count(i)) {
                time24 start = tableStartTime.at(i);
                time24 end = closeTime;
                int minutes = end - start;
                usage = time24(minutes / 60, minutes % 60);
            }
            stats.emplace_back(i, make_pair(revenue, usage));
        }
        return stats;
    }

 };

bool lineIsValid(const string& line, int line_number) //проверка строк из файла по нужному формату
{
    if (line.empty())
      return false;
    if (line_number == 1 || line_number == 3) //для строк 1 и 3
    {
        for (int i=0; i < line.size(); i++)
        {
        if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
            return false;
        }


        if (stoi(line) == 0)
            return false;
        // если дошли до конца - это положительное число
        return true;
    }
    else if (line_number == 2) //для строки 2
    {
        if (line.size() != 11)
            return false;

        for (int i= 0; i< 2; i++) {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }

        if (line[2] != ':')
            return false;

        for (int i= 3; i< 5; i++) {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }
        if (line[5] != ' ')
            return false;

        for (int i= 6; i< 8; i++) {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }

        if (line[8] != ':')
            return false;
        for (int i= 9; i< 11; i++) {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }

        open.hours = stoi(line.substr(0,2));
        open.minutes = stoi(line.substr(3,2));

        closed.hours = stoi(line.substr(6,2));
        closed.minutes = stoi(line.substr(9,2));

        if (!open.isValid() || !closed.isValid())
            return false;

        if (closed < open)
            return false;

        // если дошли до конца - формат подходит
        return true;

    }
    
    else //для остальных строк
    {
        for (int i= 0; i< 2; i++)
        {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }

        if (line[2] != ':')
            return false;

        for (int i= 3; i< 5; i++)
        {
            if (!(line[i] == '0' || line[i] == '1' || line[i] == '2' || line[i] == '3' || line[i] == '4' || line[i] == '5' || line[i] == '6' || line[i] == '7' || line[i] == '8' || line[i] == '9'))
                return false;
        }

        time24 lineTime;
        lineTime.hours = stoi(line.substr(0,2));
        lineTime.minutes = stoi(line.substr(3,2));

        if (!lineTime.isValid())
            return false;

        if (lineTime < previous_time)
            return false;

        previous_time.hours = lineTime.hours;
        previous_time.minutes = lineTime.minutes;

        if (line[5] != ' ')
            return false;

        if (!(line[6] == '1' || line[6] == '2' || line[6] == '3' || line[6] == '4'))
            return false;

        if (line[7] != ' ')
            return false;

        string clientName = line.substr(8);
        if (line[6] != '2')
        {
            if (!isValidClientName(clientName))
                return false;
        }
        else if (line[6] == '2')
        {
            size_t spaceIndex = clientName.find(' ');
            if (spaceIndex == std::string::npos)
                return false;

            string table = clientName.substr(spaceIndex+1);
            for (char c: table)
            {
                if (!isdigit(c))
                    return false;
            }

            if (stoi(table) > comps_quantity)
                return false;
        }
        

        // если дошли до конца - формат подходит
        return true;
    }
}
 

int main(int argc, char* argv[])                          
{

    string filename = argv[1]; //файл как аргумент при запуске программы
    ifstream file(filename); //поток чтения
    if (!file) {
        std::cerr << "File opening error\n";
        return 1;
    }                   
       
    string line; //строка из текстового файла
    vector <string> lines; //вектор строк из файла

    for (int i = 1; getline(file, line); i++)
    {
        if (!lineIsValid(line, i))
        {
            cout << line << endl;
            return 1;
        }
        lines.push_back(line);

        if (i == 1)
        comps_quantity = stoi(line); //чтение кол-во компьютеров из файла
        else if (i == 3)
            price = stoi(line); //чтение цены за час из файла
    }

    //cout << "Lines from file complete" << endl;

    ComputerClub club(comps_quantity, open, closed, price);
    vector <Event> events;

    for (int i = 3; i < lines.size(); i++)
        events.push_back(lines[i]);

    cout << open.toString() << endl ;

    for (const auto& event : events)
    {
        club.processEvent(event);
    }

    club.endOfDay(); // конец дня

    for (const auto& line : club.getOutput()) //вывод всех событий
    {
        cout << line << endl;
    }

    cout << closed.toString() << endl;


    auto stats = club.getTableStats(); //статистика столов за день
    for (const auto& [table, data] : stats)
    {
        auto [revenue, usage] = data;
        cout << table << " " << revenue << " ";
        char buffer[6];  // HH:MM + нулевой символ
        snprintf(buffer, sizeof(buffer), "%02d:%02d", club.tableMinutes[table] / 60, club.tableMinutes[table] % 60);
        cout << buffer << endl;
    }
    
    return 0;                       
}     