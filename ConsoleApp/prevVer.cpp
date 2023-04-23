// regFltConsol.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <string>
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <devioctl.h>
#include <strsafe.h>
#include <sstream>
#include <exception>



#define OUTPUT_LEVEL '\t'
#define DRIVER_NAME             L"RegFltr"
#define DRIVER_NAME_WITH_EXT    L"RegFltr.sys"
//#define WIN32_DEVICE_NAME       L"\\\\.\\RegFltr"
#define WIN32_DEVICE_NAME       L"\\\\.\\RegFltr"
//#define WIN32_DEVICE_NAME       L"RegFltr"

#define ARRAY_LENGTH(array)    (sizeof (array) / sizeof (array[0]))

HANDLE g_Driver;


/// std::cout<< "Error"<<std::hex<<GetLastError()<<std::endl;

//
// IOCTLs exposed by the driver.
//

#define IOCTL_DO_UPDATE_ROOLS            CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 0), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)// В исходнике драйвера должно быть точно такое же определение!!!
#define IOCTL_SET_CREATE_THREAD          CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 1), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)// В исходнике драйвера должно быть точно такое же определение!!!
#define IOCTL_REMOVE_CREATE_THREAD       CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 2), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)// В исходнике драйвера должно быть точно такое же определение!!!
#define IOCTL_STOP_SERVICE               CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 3), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)// В исходнике драйвера должно быть точно такое же определение!!!

typedef struct _REGISTER_CREATE_THREAD_OUTPUT {           // Передумал, это МУСОР Ответ драйвера на установку PsSetCreateThreadNotifyRoutine FALSE-Error, TRUE - success
    LARGE_INTEGER Result;
} REGISTER_CREATE_THREAD_OUTPUT, * PREGISTER_CREATE_THREAD_OUTPUT;

typedef struct _DRIVER_IOCTL_OUTPUT                         // Структура для получения дополнительной информации в ответе на команду от драйвера
{                             // В исходнике драйвера должно быть точно такое же определение!!!
    int Reserved;                                         // Для эксперимента - возврат значения, для понимания ошибок при выполнении команды драйвером
} DRIVER_IOCTL_OUTPUT, * PDRIVER_IOCTL_OUTPUT;

typedef struct _DRIVER_IOCTL_INPUT              // Структура для отправки дополнительной информации
{                             // в команде драйверу (сделана для эксперимента, далее НЕ ИСПОЛЬЗУЕТСЯ)
    int Reserved;
} DRIVER_IOCTL_INPUT, * PDRIVER_IOCTL_INPUT;


typedef struct MyListRule {
    std::wstring procName;
    int level;
    struct MyListRule* next;
} MyListRule;

MyListRule* my_list = NULL;

void add_Rule(MyListRule** head, std::wstring procName, int level)  //  my_list
{
    MyListRule* new_node = (MyListRule*)malloc(sizeof(MyListRule));
    if (new_node != NULL) 
    {
        memset(new_node, 0x00, sizeof(MyListRule));
        new_node->procName = procName;
        new_node->level = level;
        new_node->next = NULL;
        // Список пуст
        if (*head == NULL) 
        { 
            *head = new_node; 
        }
        // Иначе добавляем в конец списка
        else 
        {
            MyListRule* tmp_node = *head;
            while (tmp_node->next != NULL) 
            {
                tmp_node = tmp_node->next;
            }
            tmp_node->next = new_node;
        }
    }
    else
    {
        std::wcout << L" Error malloc " << std::endl;
    }
}

void EraseRules(MyListRule** head)
{
    MyListRule* tmp_node = NULL;
    while (*head != NULL)
    {
        tmp_node = *head;
        *head = (*head)->next;
        free(tmp_node);
    }
}

/*void printRule(MyListRule* node) {
    if (node != NULL) {
        std::wcout << L" " << node->procName << L" level: " << node->level << std::endl;
    }
    else { std::wcout << L"NULL" << std::endl; }
}

void printRules(MyListRule** head) {
    MyListRule* tmp_node = *head;
    while (tmp_node != NULL) {
        printRule(tmp_node);
        tmp_node = tmp_node->next;
    }
    printRule(tmp_node);
    //printf("\n\n");
} */

/**/
void printRules(MyListRule** head) {
    MyListRule* tmp_node =  *head;
    //tmp_node = head;
    int i = 1;
    while (tmp_node != NULL) {
        //if (node != NULL)
        std::wcout << i << L" " << tmp_node->procName << L" level: " << tmp_node->level << std::endl;
        tmp_node = tmp_node->next;
        i++;
    }
    //if (tmp_node!=NULL) std::wcout << i << L" " << tmp_node->procName << L" level: " << tmp_node->level << std::endl;
    //print_node(head);
    std::wcout << std::endl;
    std::wcout << std::endl;
    std::wcout << std::endl;
}


SC_HANDLE GetServiceHandle()                                                    // Функция получения хэндла драйвера
{
    SC_HANDLE hSCM = NULL;                                                      // Указатель на SC Manager
    SC_HANDLE hService = NULL;                                                  // Возвращаемый результат
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);                    // Открыть SC Manager
    if (hSCM == NULL)                                                           // Открыть не удалось
    {
        std::cout << "OpenSCManager failed, last error: " << std::hex << GetLastError() << std::endl;// Вывод сообщения об ошибке
        return hService;                                                        // Выход
    }
    hService = OpenService(hSCM, DRIVER_NAME, SERVICE_ALL_ACCESS);              // Открыть указатель на службу дрвайвера
    if (hService == NULL)                                                       // Не удалось получить указатель на службу дрвайвера
    {
        std::cout << "OpenService failed, last error: " << std::hex << GetLastError() << std::endl;// Вывод сообщения об ошибке
        CloseServiceHandle(hSCM);                                               // Закрыть дескриптор SC Manager
    }
    return hService;                                                            // Выход
}







int DoSetCreateThread()                             // Отправить драйверу команду - Выполнить PsSetCreateThreadNotifyRoutine
{
    DWORD BytesReturned = 0;                // Для получения размера буфера в функции DeviceIoControl (для вызова требуется, но само значение не используется)
    BOOL Result;                      // Результат выполнения функции DeviceIoControl
    DRIVER_IOCTL_OUTPUT Output = { 0 };           // Структура для получения дополнительной информации в ответе драйвера в функции DeviceIoControl
    //DRIVER_IOCTL_INPUT Input = { 0 };           // Драйверу дополнительной информации не отправляем
    Output.Reserved = 0;                  // Инициализируем значение в выходном буфере функции DeviceIoControl (не знаю надо или нет???)
    HANDLE hDriver = NULL;                  // Хэндл драйвера
    hDriver = CreateFile(WIN32_DEVICE_NAME,         // Получаем хэндл драйвера - ИМЯ: L"\\\\.\\RegFltr"
        GENERIC_READ | GENERIC_WRITE,           // Чтение и запись
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_SYSTEM,                                          //  FILE_ATTRIBUTE_SYSTEM  FILE_ATTRIBUTE_NORMAL
        NULL);
    if (hDriver == INVALID_HANDLE_VALUE)          // Ошибка выполнения CreateFile 
    {
        std::cout << "DoSetCreateThread CreateFile error: " << std::hex << GetLastError() << std::endl;// Вывод сообщения об ошибке
        return 0;                     // выход из функции
    }
    Result = DeviceIoControl(hDriver,           // Отправить драйверу с хэндлом hDriver
        IOCTL_SET_CREATE_THREAD,              // управляющий код CTL_CODE (FILE_DEVICE_UNKNOWN, (0x800 + 1), METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
        NULL,                       // указатель на буфер с информацией отправляемый драйверу
        0,                          // размер этого буфера
        &Output,                      // указатель на буфер получаемой информацией в ответе драйвера
        sizeof(DRIVER_IOCTL_OUTPUT),            // размер этого буфера
        &BytesReturned,                   // ?????????????????????????????????
        NULL);                        // ?????????????????????????????????
    if (Result != TRUE) {                 // Ошибка выполнения DeviceIoControl
        std::cout << "DoSetCreateThread DeviceIoControl error: " << std::hex << GetLastError() << std::endl;        // Вывод сообщения об ошибке
        //ErrorPrint("DeviceIoControl for GET_CALLBACK_VERSION failed, error %d\n", GetLastError());
        CloseHandle(hDriver);               // Закрыть хэндл
        return 0;                     // выход из функции
    }
    std::cout << "DeviceIoControl return OUTPUT: " << std::hex << Output.Reserved << std::endl; // Вывод 
    CloseHandle(hDriver);
    return Output.Reserved;
}

int DoRemoveCreateThread()     // Выполнить PsRemoveCreateThreadNotifyRoutine
{
    DWORD BytesReturned = 0;
    BOOL Result;
    DRIVER_IOCTL_OUTPUT Output = { 0 };
    //DRIVER_IOCTL_INPUT Input = { 0 };
    Output.Reserved = 0;
    HANDLE hDriver = NULL;
    hDriver = CreateFile(WIN32_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_SYSTEM,                                          //  FILE_ATTRIBUTE_SYSTEM  FILE_ATTRIBUTE_NORMAL
        NULL);
    if (hDriver == INVALID_HANDLE_VALUE)
    {
        std::cout << "DoRemoveCreateThread CreateFile error: " << std::hex << GetLastError() << std::endl;        // Вывод сообщения об ошибке
        return 0;
    }
    Result = DeviceIoControl(hDriver,
        IOCTL_REMOVE_CREATE_THREAD,
        NULL,
        0,
        &Output,
        sizeof(DRIVER_IOCTL_OUTPUT),
        &BytesReturned,
        NULL);
    if (Result != TRUE) {
        std::cout << "DoRemoveCreateThread DeviceIoControl error: " << std::hex << GetLastError() << std::endl;        // Вывод сообщения об ошибке
        //ErrorPrint("DeviceIoControl for GET_CALLBACK_VERSION failed, error %d\n", GetLastError());
        CloseHandle(hDriver);                     // Закрыть хэндл драйвера
        return 0;
    }
    std::cout << "DeviceIoControl return OUTPUT: " << std::hex << Output.Reserved << std::endl;        // Вывод результатов выполнения в kernel mode: -1 - PsSetCreateThreadNotifyRoutine не вызывался, закрывать нечего; -2 - ошибка PsRemoveCreateThreadNotifyRoutine (функция для обработки события создания/удаления потоков не зарегистрирована); 0 - ошибок не было
    CloseHandle(hDriver);                       // Закрыть хэндл драйвера
    return Output.Reserved;                       // Вернуть дополнительную информацию, переданную драйвером
}

// VOID DoUpdateRools()            // Считать обновленные правила фильтрации из реестра
//{
  
//}

// szDriverName      DRIVER_NAME
  // szDriverFileName  DRIVER_NAME_WITH_EXT
  // szWin32DeviceName WIN32_DEVICE_NAME

//
// Функция получения состояния драйвера. При успешном выполнении (результат TRUE),
// в параметре State - статус состояния
// Варианты State: 1-SERVICE_STOPPED, 2-SERVICE_START_PENDING, 3-SERVICE_STOP_PENDING, 4-SERVICE_RUNNING
// 5-SERVICE_CONTINUE_PENDING, 6-SERVICE_PAUSE_PENDING, 7-SERVICE_PAUSED
BOOL GetServiceState(DWORD* State)         
{
  SC_HANDLE hService = NULL;                                                  // Указатель на сервис драйвера
  SERVICE_STATUS_PROCESS ServiceStatus;                                       // Переменная для получения статуса из QueryServiceStatusEx
    DWORD BytesNeeded;                                                          // Указатель на переменную, которая получает количество байтов, необходимых для хранения всей информации о состоянии, 
                                                                                // если функция QueryServiceStatusEx завершается с ошибкой ERROR_INSUFFICIENT_BUFFER 
  BOOL Result;                                                                // результат QueryServiceStatusEx
  *State = 0;                                                                 // Обнулить возвращаемое значение (если результат будет FALSE)
    hService = GetServiceHandle();                                              // Открыть указатель на службу дрвайвера
    if (hService == NULL) return FALSE;                                         // Если открыть не удалось, то выход (сообщения об ошибках выводит в консоль GetServiceHandle)

  Result = QueryServiceStatusEx ( hService,                                   // Запрос состояния службы
                                         SC_STATUS_PROCESS_INFO,
                                         (LPBYTE)&ServiceStatus,
                                         sizeof(ServiceStatus),
                                         &BytesNeeded);
  if (Result == FALSE)                                                        // Запрос состояния службы неудачен
  {
        std::cout<< "QueryServiceStatusEx failed, last error: " << std::hex << GetLastError() << std::endl; // Вывод сообщения об ошибке
        CloseServiceHandle(hService);                                           // Закрыть дескриптор службы дрвайвера
        return FALSE;                                                           // Выход
    }
  *State = ServiceStatus.dwCurrentState;                                      // Записать результат состояния службы дрвайвера
    CloseServiceHandle(hService);                                               // Закрыть дескриптор службы дрвайвера    
  return TRUE;                                                                // Выход
}

// Функция ожидания запрошенного состояния службы
BOOL UtilWaitForServiceState(DWORD State)
{
    SC_HANDLE hService = NULL;                                                  // Указатель на сервис драйвера
    BOOL Result;                                                                // Результат запроса статуса драйвера
    DWORD ServiceState;                                                         // Полученный при запросе статус драйвера
    hService = GetServiceHandle();                                              // Открыть указатель на службу дрвайвера
    if (hService == NULL) return FALSE;                                         // Если открыть не удалось, то выход (сообщения об ошибках выводит в консоль GetServiceHandle)
    for (;;)                                                                    // Цикл ожидания требуемого статуса
    {
        Result = GetServiceState(&ServiceState);                                // Получить статус драйвера
        if (Result == FALSE) return FALSE;                                      // Если получить не удалось, то выход
        if (ServiceState == State) break;                                       // Если статус совпадает с ожидаемым, то выход из цикла ожидания нужного статуса
        Sleep(500);                                                             // Ждать 0,5 сек
    }
    return TRUE;                                                                // Выход
}

BOOL StopDriver()                   // остановка драйвера
{
    SC_HANDLE hService = NULL;                                                  // Указатель на сервис драйвера
    SERVICE_STATUS ServiceStatus;

    hService = GetServiceHandle();                                              // Открыть указатель на службу дрвайвера
    if (hService == NULL) return FALSE;                                         // Если открыть не удалось, то выход (сообщения об ошибках выводит в консоль GetServiceHandle
    if (FALSE == ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
            std::cout << "ControlService failed, last error: " << std::hex << GetLastError() << std::endl;// Вывод сообщения об ошибке
            if (hService) CloseServiceHandle(hService);                             // Закрыть дескриптор службы дрвайвера
            return FALSE;
        }
    }

    if (FALSE == UtilWaitForServiceState(SERVICE_STOPPED)) {
        if (hService) CloseServiceHandle(hService);                             // Закрыть дескриптор службы дрвайвера
        return FALSE;
    }
    if (hService) CloseServiceHandle(hService);                             // Закрыть дескриптор службы дрвайвера
    return TRUE;
}


//
// Функция получения запуска службы драйвера. При успешном выполнении - служба запущена
//
BOOL LoadDriver()
{
    SC_HANDLE hService = NULL;                                                  // Указатель на сервис драйвера

    hService = GetServiceHandle();                                              // Открыть указатель на службу дрвайвера
    if (hService == NULL) return FALSE;                                         // Если открыть не удалось, то выход (сообщения об ошибках выводит в консоль GetServiceHandle)
    if (!StartService(hService, 0, NULL))                                       // Ошибка запуска
    {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)                    // Ошибка не из-за уже запущенной службы
        {
            std::cout << "StartService failed, last error: " << std::hex << GetLastError() << std::endl;// Вывод сообщения об ошибке
            CloseServiceHandle(hService);                                       // Закрыть дескриптор службы дрвайвера    
            return FALSE;                                                       // Выход
        }                                                                       // иначе - служба уже запущена, ничего делать не надо
    }
    if (FALSE == UtilWaitForServiceState(SERVICE_RUNNING))                      // Ждем запуска службы
    {
        if (hService) CloseServiceHandle(hService);                             // Закрыть дескриптор службы дрвайвера
        return FALSE;                                                           // Выход
    }
    if (hService) CloseServiceHandle(hService);                                 // Закрыть дескриптор службы дрвайвера    
    return TRUE;                                                                // Выход
}



int LoadRules()
{
    //
    HKEY hk;
    long res;
    long rulesCount = 0;
    DWORD dType;
    DWORD dSize = 0;
    PWCHAR Data = NULL;
    std::wstring regKey;
    //std::wstring procKey; 
    std::wstring procName;
    //std::wstring kav;
    std::string::size_type shift = 0;
    //std::string::size_type i = 0;
    int param2 = 0;
    //procKey = TEXT("\"Process\":\"");
    //procKey.assign((L"\"Process\":\""));
    //kav.assign(L"\"");

    res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\regFltr"), 0, KEY_READ, &hk);
    if (res == ERROR_SUCCESS) 
    {
        //std::cout << "OK" << std::endl;
        res = RegGetValueW(hk, NULL, TEXT("rule"), RRF_RT_REG_SZ, &dType, Data, &dSize);
        if (res == ERROR_MORE_DATA)
        {
            std::cout << "Buffer size: " << dSize << std::endl;
        }
        else
        {
            //std::cout << "Failed RegGetValueW with value " << res << std::endl;
            //std::cout << "Buffer size: " << dSize << std::endl;
            Data = (PWCHAR) malloc(dSize);
            RegGetValueW(hk, NULL, TEXT("rule"), RRF_RT_REG_SZ, &dType, Data, &dSize);
            if (dSize > 0)
            {
                regKey.assign(Data);
                free(Data);
            }
            else
            {
                RegCloseKey(hk);
                return 0;
            }
            //std::wcout << regKey << std::endl;
            //std::wcout << regKey.length() << std::endl;

            while (regKey.length() > 10)
            {
                shift = regKey.find(L"\"Process\":\"");
                if (shift == std::string::npos) break;
                regKey = regKey.substr(shift + 11);
                shift = regKey.find(L"\"");
                procName = regKey.substr(0, shift);
                shift = regKey.find(L"\"level\":");    //8
                param2 = std::stoi(regKey.substr(shift+8, 1));
                rulesCount++;
                add_Rule(&my_list, procName, param2);
                std::wcout << procName << " level: " << param2 << std::endl;
            }
        }
    }
    else std::cout << "Open Registry Key Error: " << res << std::endl;
    RegCloseKey(hk);
    return rulesCount;
}

int main()
{
    //DWORD dwSize;
    DWORD state;
    int rulesCount;
    TCHAR szDriverPath[MAX_PATH] = _T("");
    HANDLE hDriver = NULL;
    //*pDriver = NULL;

    rulesCount = LoadRules();
    std::wcout << L"Load " << rulesCount << L" rules" << std::endl;
    printRules(&my_list);

    std::string choice;
    std::cout << "Program for setting access rights" << std::endl;
    std::cout << OUTPUT_LEVEL << "Valid parameters for execution:" << std::endl;
    std::cout << OUTPUT_LEVEL << "print - display current permissions;\n";
    std::cout << OUTPUT_LEVEL << "del   - delete a rule with a number N;\n";
    std::cout << OUTPUT_LEVEL << "add   - add a new rule;\n";
    std::cout << OUTPUT_LEVEL << "start - load RegFltDriver;\n";
    std::cout << OUTPUT_LEVEL << "stop  - unload RegFltDriver;\n";
    std::cout << OUTPUT_LEVEL << "set   - set notifier PsSetCreateThreadNotifyRoutine;\n";
    std::cout << OUTPUT_LEVEL << "rm   - remove notifier PsSetCreateThreadNotifyRoutine;\n";
    std::cout << OUTPUT_LEVEL << "exit  - close program\n";
    while (choice.compare("exit")) {
        //std::cout << "Hello World!\n";
        std::cout << "Input command: ";
        std::getline(std::cin, choice);
        //std::cin >> choice;
        if (!choice.compare("print")) {
            // Display curr permissions
            printRules(&my_list);
        }
        else if (!choice.compare(0, 4, "del ")) {
            choice.erase(0, 4);
            try {
                int num = std::stoi(choice);
                std::cout << num << std::endl;
            }
            catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
        }
        else if (!choice.compare(0, 4, "add ")) {
            choice.erase(0, 4);
            try {
                std::stringstream input(std::move(choice));
                std::string procName;
                std::string numStr;
                input >> procName;
                input >> numStr;
                int num = std::stoi(numStr);
                std::cout << procName << ' ' << num << std::endl;
            }
            catch (std::exception& ex) { std::cout << ex.what() << std::endl; }
        }
        else if (!choice.compare("start")) {
            // load RegFltDriver
            /*if (GetServiceState(&state))
            {
                if (state == SERVICE_RUNNING)
                {
                    std::cout << "Driver already running\n";
                }
                else
                {
                    if (LoadDriver(&g_Driver))
                    {
                        std::cout << "Driver started\n";
                    }
                }
            }  */
            if (LoadDriver()) std::cout << "Driver started\n";
        }
        else if (!choice.compare("stop")) {
            // unload ReqFltDriver
            if (StopDriver()) std::cout << "Driver stopped\n";
        }
        else if (!choice.compare("set")) {
            // set notifier
            int res = DoSetCreateThread();
            if (res != 0)
            {
                //std::cout << "DoSetCreateThread SEND 5 ANSWER: \n";
                std::cout << "DoSetCreateThread ANSWER: " << std::dec << res << std::endl;
            }
            else
            {
                std::cout << "DoSetCreateThread Error\n";
            }
            
            

        }
        else if (!choice.compare("rm")) {
            // remove notifier
            int res = DoRemoveCreateThread();
            if (res != 0)
            {
                //std::cout << "DoSetCreateThread SEND 5 ANSWER: \n";
                std::cout << "DoRemoveCreateThread ANSWER: " << std::dec << res << std::endl;
            }
            else
            {
                std::cout << "DoRemoveCreateThread\n";
            }
            //std::cout << "command to remove notifier\n";
        }
    }
    std::cout << "Good Bay World!\n";
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
