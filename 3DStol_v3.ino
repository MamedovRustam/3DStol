#include <ESP8266WiFi.h>                     // Содержится в пакете нужно для подключения к WIFI 
#include <ESP8266WebServer.h>                // За функции сервера отвечает
#include <FS.h>
#include <ArduinoJson.h>

/*********************************************************************************************************************/
const char* ssidAP = "3DStol";              // Название Wifi сети которую создаст esp8266
const char* passwordAP = "";                // Сеть без пороля
const char* www_username = "3DStol";        // Авторизация на странице - логин
const char* www_password = "3DStol";        // Авторизация на странице - пароль
const char* SSDP_Name = "3DStol";           // Имя SSDP

/* Ниже две переменные будут подставлятся в get запрос погоды в функции zaprosPogody() а значения свои они получат    */
/* из функции parametryStartaPogody() */
const char* getUrlCitiId ;                  // Будет хранить код города 
const char* getUrlApiKey ;                  // Будет хранить ApiKey
const char* host = "api.openweathermap.org";
String rezGetZapr;                          // Будет содержать результат запроса в json формате

/*********************************************************************************************************************/
IPAddress apIP(192, 168, 4, 1);             // Статический IP
ESP8266WebServer server(80);                // Web интерфейс для устройства
/*********************************************************************************************************************/
#import "APindexPage.h"                     // Импорт главной страницы в режиме точки доступа (AP) 
#import "STAindexPage.h"                    // Импорт главной страницы в режиме клиента (STA) 
#import "rstNotificPage.h"                  // Импорт страницы о том что настройки сохранены и будут попытки подключения

// Теперь создаем таблицу с отсылками к переменным с кучей строк
const char* const string_table[] PROGMEM  = { APIndexPage, STAIndexPage, rstNotificPage};

/* Функция сделает запрос погоды полученные значения используем в loop                                               */
void zaprosPogody(){
  WiFiClient client;
  const byte httpPort = 80;
  while (!client.connect(host, httpPort)) {
    Serial.println(F("Нет конекта с api.openweathermap.org"));
    delay(2000);
    return;
  }
  if (client.connected()) {
    // взято в String() потомучто оператор + не мог строки сложить, короче я сам не понял 
    client.println(String("GET /data/2.5/weather?id=")+getUrlCitiId+"&appid="+getUrlApiKey+" HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  }

  // wait for data to be available
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      delay(5000);
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
   rezGetZapr = client.readStringUntil('\r'); // rezGetZapr глоб перем Будет содержать результат запроса в json формате
   //rezGetZapr = static_cast<char>(client.read());
  }
  Serial.print(rezGetZapr);
  Serial.println();
  Serial.println("closing connection");
  client.stop();
}

/* Сначала узнаем мы подключены к интернету (клиент). Если да работаем дальше если нет то сообшаем об этом в Serial  */
/* Функция проверит содержимое файла citiidapikey.json Если они заводские то попросим пользоват-я ввести их          */
/* Если они не заводские а пользовательские то присваиваем их глобальным переменны getUrlCitiId и getUrlApiKey       */
/* стартанем погоду zaprosPogody(),кстати в этой функции эти глобальные переменные и нужны                           */
void parametryStartaPogody(){
     if( WiFi.status() == WL_CONNECTED ){  // WL_CONNECTED подключен к сети как клиент
    /* Пораметры по дефолту в citiidapikey.json */
    const char* defCitiId = "000000";
    const char* defApiKey = "00000000000000000000000000000000";

    File configFile = SPIFFS.open("/citiidapikey.json", "r");  // Открываем файл для чтения
    StaticJsonDocument<150> docJson;                      // Создаем буфер
    deserializeJson(docJson, configFile);                 // configFile - эт переменная открыЛа файл чуть выше в коде
    
    const char* PeremenCitiIdJson = docJson["citi_id"];
    const char* PeremenApiKeyJson = docJson["apikey"];
    
    configFile.close();                             // Закрыли файл

    { // пояснение...
      /* int strcmp(const char *str1, const char *str2)                                                            */
      /* Функция strcmp() осуществляет лексикографическую проверку двух строк, оканчивающихся нулевыми символами,  */
      /* и возвращает целое число со следующим значением:                                                          */
      /* Число     Значение                                                                                        */
      /* Меньше 0  str1 меньше, чем str2                                                                           */
      /* 0         str1 равна str2                                                                                 */
      /* Больше 0  str1 больше, чем str2                                                                           */
    }
    /* rezultSravApikey возврашает число */
    byte rezultSravApikey = strcmp(PeremenApiKeyJson, defApiKey); // Сравниваем Apikey из файла с Заводским

    if ( rezultSravApikey == 0 ) {
      Serial.println(F("Apikey Заводской, прпосим сохраниться"));
      /*
      Вывести на табло информацию о настройке погоды
      */
    }
    else {
      Serial.println(F("Apikey НЕ Заводской, Запрашиваем погоду"));
      getUrlCitiId = PeremenCitiIdJson;
      getUrlApiKey = PeremenApiKeyJson; 
      zaprosPogody();
    }  
  }
     // статус подключения AP
   if( WiFi.status() == WL_DISCONNECTED ){ // WL_DISCONNECTED не подключен к сети (как клиент)
     Serial.println(F("Мы не подключены как клиент, погоду пока не трогаем!"));
    }
}
/*** Запись данных в файл citiidapikey.json Эта функция вызыв-ся из web формы на странице STAindexPage                 */
void funSaveCfgCitiidApiKey() {
  File configFile = SPIFFS.open("/citiidapikey.json", "w+");  // Открываем файл для записи
  if (!configFile) {
    Serial.println(F("Не удалось открыть файл конфигурации citiidapikey.json для записи"));
  } else {
    Serial.println(F("Файл конфигурации citiidapikey.json для записи открыт"));
    StaticJsonDocument<150> docJson;
    /**/
    // Заполняем поля json
    docJson["citi_id"] = server.arg(0);  // код города возмется из url ссылки  (из формы)
    docJson["apikey"]  = server.arg(1);  // ApiKey возмется из url ссылки (из формы)
    Serial.println(F("Переменные для citiidapikey.json взяты из URL и присвоенны!"));
    /**/
    // Serialize (Запись) JSON to file
    if (serializeJson(docJson, configFile) == 0) {
      Serial.println(F("Не удалось записать в файл citiidapikey.json"));
    } else {
      Serial.println(F("Запись в файл конфигурации (citiidapikey.json) прошла успешно"));
    }
    configFile.close();                                // Закрываем Файл
    Serial.println(F("Файл citiidapikey.json закрыт"));
    /***** Далее отдаем страницу пользователю с инф что настройки сохранены  ******/
    char buffer[strlen(rstNotificPage)+1];
    server.send(200, "text/html", strcpy_P(buffer, (char*)pgm_read_dword(&(string_table[2]))));
    Serial.println(F("Уведомление о сохранении отданы пользователю"));
    delay(1000);
    zaprosPogody(); // Функция которая делает запрос погоды 
  }
}
/*** Запись данных в файл cfgssidpass.json Эта функция вызыв-ся из web формы на странице APindexPage                 */
void funSaveCfgSsidPass() {
  File configFile = SPIFFS.open("/cfgssidpass.json", "w+");  // Открываем файл для записи
  if (!configFile) {
    Serial.println(F("Не удалось открыть файл конфигурации cfgssidpass.json для записи"));
  } else {
    Serial.println(F("Файл конфигурации cfgssidpass.json для записи открыт"));
    StaticJsonDocument<150> docJson;
    /**/
    // Заполняем поля json
    docJson["loginHomeSsid"] = server.arg(0);  // нозв дом wifi возмется из url ссылки  (из формы)
    docJson["passHomeSsid"]  = server.arg(1);  // пороль дом wifi возмется из url ссылки (из формы)
    Serial.println(F("Переменные для cfgssidpass.json взяты из URL и присвоенны!"));
    /**/
    // Serialize (Запись) JSON to file
    if (serializeJson(docJson, configFile) == 0) {
      Serial.println(F("Не удалось записать в файл"));
    } else {
      Serial.println(F("Запись в файл конфигурации (cfgssidpass.json) прошла успешно"));
    }
    configFile.close();                                // Закрываем Файл
    Serial.println(F("Файл закрыт"));
    /***** Далее отдаем страницу пользователю с инф что настройки сохранены  ******/
    char buffer[strlen(rstNotificPage)+1];
    server.send(200, "text/html", strcpy_P(buffer, (char*)pgm_read_dword(&(string_table[2]))));
    Serial.println(F("Уведомление о сохранении отданы пользователю"));
    delay(1000);
    viborRejimaEsp(); // Функция которая решит в каком режиме теперь запустить esp8266 (AP или STA)
  }
}
/************************* Функция ответа на вызов главной страницы в режиме точки доступа  **************************/
void funSTAindexOtvet() {
  // условие ниже это авториз-я на главной странице попросит ввести логин (3DStol) и пороль (3DStol)
  //if (!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
 char buffer[strlen(STAIndexPage)+1];
 server.send(200, "text/html", strcpy_P(buffer, (char*)pgm_read_dword(&(string_table[1]))));
}
/************************* Функция ответа на вызов главной страницы в режиме точки доступа  **************************/
void funAPindexOtvet() {
  // условие ниже это авториз-я на главной странице попросит ввести логин (3DStol) и пороль (3DStol)
  //if (!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
 char buffer[strlen(APIndexPage)+1];
 server.send(200, "text/html", strcpy_P(buffer, (char*)pgm_read_dword(&(string_table[0]))));
}
/*** Функция определит в каком режиме МК, если как AP то будет отданна страница для AP режима если как STA         ***/
/*** то будет отданна страница для STA режима                                                                      ***/
void funIndexUrl(){
   // режим AP
   if( WiFi.status() == WL_DISCONNECTED ){ // WL_DISCONNECTED не подключен к сети (как клиент)
     funAPindexOtvet();
    }
   // режим STA  
   if( WiFi.status() == WL_CONNECTED ){  // WL_CONNECTED подключен к сети как клиент
    Serial.printf("Текуший SSID: %s\n", WiFi.SSID().c_str());
    funSTAindexOtvet();
    }
}
/****************************************** Подключаемся к точке доступа *********************************************/
void startWIFI_STA(){
 /* Функция прочитает из cfgssidpass.json файла логин и пороль от WIFI и попытается подкл-ся если не сможет подкл-ся */
 /* то будет вызвана функция funWriteDefaultSsidPass() которая запишет в cfgssidpass.json заводские логин и пороль   */

  File configFile = SPIFFS.open("/cfgssidpass.json", "r");  // Открываем файл для чтения
    if (!configFile){
      Serial.println(F("Не удалось открыть файл конфигурации cfgssidpass.json для чтения"));
    }
    else {
      StaticJsonDocument<150> docJson;                // Создаем буфер
      deserializeJson(docJson, configFile);

      const char* PeremenSsidCfgJson = docJson["loginHomeSsid"];
      const char* PeremenPassCfgJson = docJson["passHomeSsid"];
      configFile.close();                             // Закрыли файл

      /***** Отключаем esp8266 как точку доступа (AP) и стартуем как клиент    ******/
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(1000);
      /******************************************************************************/
      Serial.print(F("Подключаемся к "));
      Serial.println(PeremenSsidCfgJson);
      WiFi.mode(WIFI_STA);
      WiFi.begin(PeremenSsidCfgJson, PeremenPassCfgJson);
      delay(100);
      byte popytki = 0;
      while (WiFi.status() != WL_CONNECTED){
        popytki++;
        if(popytki >= 15){
        Serial.println(F("ПОПЫТКИ ИСЧЕРПАНЫ, запись настроек по умолчанию"));
        funWriteDefaultSsidPass(); // Запись настроек по умолчанию
        viborRejimaEsp();          // Функция котрая решит в каком режиме запустить ESP как AP или STA
        break;
        }
        delay(1000);
        Serial.print(F("."));
      }
      // печатаем локальный ип адрес в Serial
      Serial.println(F(""));
      Serial.println(F("WiFi подключен."));
      Serial.println(F("IP address: "));
      Serial.println(WiFi.localIP());
    }
}
/**************************************** СТАРТУЕМ ТОЧКУ ДОСТУПА  *****************************************************/
void startWIFI_AP(){
  Serial.println(F("Создаем свою точку Wifi c именем 3DStol"));
  WiFi.mode(WIFI_AP);                                         // Режим точки доступа
  WiFi.softAP(ssidAP, passwordAP);
  delay(100); // задержка что бы успела создать точку доступа
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Настр-ка сетевого интерфейса программной точки доступа.
  Serial.print(F("SoftAPIP: "));
  Serial.println(WiFi.softAPIP());                    // Вернуть IP-адрес сетевого интерфейса программной точки доступа
  Serial.println(F("Точка доступа создана"));
}
/* Функция которая запустит esp8266 как клиента или как точку доступа используя результат функции cgfSsidPassZavod() */
void viborRejimaEsp(){
  bool rezultat = cgfSsidPassZavod();
  if(rezultat == false){
    Serial.println(F("Стартуем точку доступа AP ()"));
    startWIFI_AP();
    }
  if(rezultat == true){
    Serial.println(F("Стартуем клиента STA ()"));
    startWIFI_STA();
    }
  }
/*  Функция проверит зоводские настройки в cfgssidpass.json или нет если заводские то вернет FALSE если нет то TRUE  */
/*  Функция автономная/самостоятельная                                                                               */
bool cgfSsidPassZavod(){
  /* Пораметры по дефолту в cfgssidpass.json */
    const char* defSsid_CfgFile = "default_ssid";
    const char* defPass_CfgFile = "default_pass";

    File configFile = SPIFFS.open("/cfgssidpass.json", "r");  // Открываем файл для чтения
    StaticJsonDocument<150> docJson;                         // Создаем буфер
    deserializeJson(docJson, configFile);                   // configFile - эт переменная открыЛа файл чуть выше в коде

    const char* PeremenSsidCfgJson = docJson["loginHomeSsid"];
    const char* PeremenPassCfgJson = docJson["passHomeSsid"];
    configFile.close();                             // Закрыли файл


    { // пояснение...
      /* int strcmp(const char *str1, const char *str2)                                                            */
      /* Функция strcmp() осуществляет лексикографическую проверку двух строк, оканчивающихся нулевыми символами,  */
      /* и возвращает целое число со следующим значением:                                                          */
      /* Число     Значение                                                                                        */
      /* Меньше 0  str1 меньше, чем str2                                                                           */
      /* 0         str1 равна str2                                                                                 */
      /* Больше 0  str1 больше, чем str2                                                                           */
    }
    /* rezultSravSsid и  rezultSravPass Возврашают числа */
    byte rezultSravSsid = strcmp(PeremenSsidCfgJson, defSsid_CfgFile); // Сравниваем Ssid с Заводским
    byte rezultSravPass = strcmp(PeremenPassCfgJson, defPass_CfgFile); // Сравниваем Пароль с Заводским

    if ( rezultSravSsid == 0 ) {
      Serial.println(F("ЛОГИН Заводской (false)"));
      return false;
    }
    else {
      Serial.println(F("ЛОГИН НЕ Заводской (true)"));
      return true;
    }
  }
/***********  Функция записи в citiidapikey.json настроек по умолчанию, функция автономная самостоятельная           */
void funWriteDefaultCitiidApikey(){
  File configFile = SPIFFS.open("/citiidapikey.json", "w+");  // Открываем файл для записи
  if (!configFile) {
    Serial.println(F("Не удалось открыть файл конфигурации citiidapikey.json для записи настроек по умолчанию"));
  } else {
    Serial.println(F("Файл конфигурации citiidapikey.json для записи настроек по умолчанию открыт"));
    StaticJsonDocument<150> docJson;
    
    // Заполняем поля json
    docJson["citi_id"] = "000000";                            // citi_id - код города по умолчанию
    docJson["apikey"]  = "00000000000000000000000000000000";  // apikey - api ключ по умолчанию
    // Serialize (Запись) JSON to file
    if (serializeJson(docJson, configFile) == 0) {
      Serial.println(F("Не удалось записать настройки по умолчанию в файл citiidapikey.json"));
    } else {
      Serial.println(F("Запись настроек по умолчанию в файл конфигурации (citiidapikey.json) прошла успешно"));
    }
    configFile.close();                                // Закрываем Файл
    Serial.println(F("Файл citiidapikey.json закрыт"));
  }  
  }
/* Функция проверит сушествует ли файл citiidapikey.json, если нет то создаст его с заводскими настройками */
void existCfgApiKeyJson(){
  Serial.println(F("Проверяем сушествует ли файл citiidapikey.json"));
  if (SPIFFS.exists("/citiidapikey.json")){
    Serial.println(F("Файл citiidapikey.json сушествует"));
    Serial.println(F("Вот его содержимое.."));
    Serial.println(F(" "));
    /* Раз файл сушествует смотрим что внутри */
    String CfgJsonSerial ;  // В эту переменную поместится содержимое citiidapikey.json
    File configFile = SPIFFS.open("/citiidapikey.json", "r");  // Открываем файл для чтения
    if (!configFile){
      Serial.println(F("Не удалось открыть файл конфигурации citiidapikey.json для чтения"));
    }
    else {
      Serial.println(F("Файл конфигурации citiidapikey.json для чтения открыт"));
      Serial.println(F("************************************************************************"));
      while (configFile.position() < configFile.size())
      {
        CfgJsonSerial = configFile.readStringUntil('\n'); // Читаем содер-е откр-го файла в перемен-ю strCfgJsonSerial
        CfgJsonSerial.trim();                             // Удаляет пробелы из начала и конца строки
        Serial.println(CfgJsonSerial);
      }
      Serial.println(F("************************************************************************"));
      size_t size = configFile.size(); // Размер файла config.json в байтах
      configFile.close();  // Закрываем файл
      Serial.print(F("Вес файла "));
      Serial.print(size);
      Serial.println(F(" байт."));
      Serial.println(F("************************************************************************"));
    }
  }
  else {
    Serial.println(F("Файл citiidapikey.json НЕ сушествует! Создаем.."));
    funWriteDefaultCitiidApikey(); // Вызов функции записи настроек по умолчанию
  }
  }
/***********  Функция записи в cfgssidpass.json настроек по умолчанию, функция автономная самостоятельная  ***********/
void funWriteDefaultSsidPass() {
  File configFile = SPIFFS.open("/cfgssidpass.json", "w+");  // Открываем файл для записи
  if (!configFile) {
    Serial.println(F("Не удалось открыть файл конфигурации cfgssidpass.json для записи настроек по умолчанию"));
  } else {
    Serial.println(F("Файл конфигурации cfgssidpass.json для записи настроек по умолчанию открыт"));
    StaticJsonDocument<150> docJson;
    
    // Заполняем поля json
    docJson["loginHomeSsid"] = "default_ssid";  // default_ssid - ssid по умолчанию
    docJson["passHomeSsid"]  = "default_pass";  // default_pass - пороль по умолчанию
    // Serialize (Запись) JSON to file
    if (serializeJson(docJson, configFile) == 0) {
      Serial.println(F("Не удалось записать настройки по умолчанию в файл cfgssidpass.json"));
    } else {
      Serial.println(F("Запись настроек по умолчанию в файл конфигурации (cfgssidpass.json) прошла успешно"));
    }
    configFile.close();                                // Закрываем Файл
    Serial.println(F("Файл cfgssidpass.json закрыт"));
  }  
  }
/* Функция проверит сушествует ли файл cfgssidpass.json, если нет то создаст его с заводскими настройками */
void existCfgSsidPass(){
  Serial.println(F("Проверяем сушествует ли файл cfgssidpass.json"));
  if (SPIFFS.exists("/cfgssidpass.json")){
    Serial.println(F("Файл cfgssidpass.json сушествует"));
    Serial.println(F("Вот его содержимое.."));
    Serial.println(F(" "));
    /* Раз файл сушествует смотрим что внутри */
    String CfgJsonSerial ;  // В эту переменную поместится содержимое cfgssidpass.json
    File configFile = SPIFFS.open("/cfgssidpass.json", "r");  // Открываем файл для чтения
    if (!configFile){
      Serial.println(F("Не удалось открыть файл конфигурации cfgssidpass.json для чтения"));
    }
    else {
      Serial.println(F("Файл конфигурации cfgssidpass.json для чтения открыт"));
      Serial.println(F("************************************************************************"));
      while (configFile.position() < configFile.size())
      {
        CfgJsonSerial = configFile.readStringUntil('\n'); // Читаем содер-е откр-го файла в перемен-ю strCfgJsonSerial
        CfgJsonSerial.trim();                             // Удаляет пробелы из начала и конца строки
        Serial.println(CfgJsonSerial);
      }
      Serial.println(F("************************************************************************"));
      size_t size = configFile.size(); // Размер файла cfgssidpass.json в байтах
      configFile.close();  // Закрываем файл
      Serial.print(F("Вес файла "));
      Serial.print(size);
      Serial.println(F(" байт."));
      Serial.println(F("************************************************************************"));
    }
  }
  else {
    Serial.println(F("Файл cfgssidpass.json НЕ сушествует! Cоздаем..."));
    funWriteDefaultSsidPass(); // Вызов функции записи настроек по умолчанию
  }
  }
/********************************** Функция ответа на не найденную страницу  ******************************************/
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri(); // получает путь url ссылки
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST"; // server.method() хранит каким методом пришла ссылка
  message += "\nArguments: ";
  message += server.args();                                  // показывает количество аргументов, тип int
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void setup() {
  Serial.begin(115200);
  delay(500);
  //WiFi.disconnect(); //Затирает логин и пороль от WIFI в режиме STA
  WiFi.softAPdisconnect(true); // не выкл-т передат-к, а обрыв связь c клиентами путем установл-я пароля и юзера в NULL
  /* WiFi.softAPdisconnect(true) вызывает WiFi.enableAP(false) которая красиво вызывает WiFi.mode() 
  и отключает AP на совсем.
  WiFi.softAPdisconnect(false) достаточно для безглючной смены паролей, ip и тд. 
  То есть не надо дополнительно выключать передатчик командой WiFi.mode(WIFI_OFF)
  */
  WiFi.mode(WIFI_OFF);
  delay(1000);
  /* Запуск SPIFFS */
  if (SPIFFS.begin()) Serial.println(F("SPIFFS Запушен!"));
  /********************************************************************************************************************/
  existCfgSsidPass();      // Функ проверит сущ-ли файл cfgssidpass.json если нет СОЗДАСТ С ЗАВОДСКИМИ НАСТРОЙКАМИ
  viborRejimaEsp();        // Функция решит в каком режиме запустить ESP в зависимости от настроек cfgssidpass.json
  existCfgApiKeyJson();    // Вызов функ проверки суш-я файла citiidapikey.json если нет дадит команду создать
  parametryStartaPogody(); // Если citiidapikey не завод сделает запрос погоды (zaprosPogody())
  /********************************************************************************************************************/
  server.onNotFound(handleNotFound);                            // Вызов Функции ответа на не найденную страницу
  server.on("/", funIndexUrl);                                  // Вызов функции определения главной страницы
  server.on("/urlSaveCfgSsidPass", funSaveCfgSsidPass);         // Вызов функции которая сохраняет логин и пороль домашней сети
  server.on("/urlSaveCfgCitiidApiKey", funSaveCfgCitiidApiKey); // Вызов функции которая сохраняет ApiKey погоды и код города
  server.on("/urlWriteDefaultApiKey", funWriteDefaultCitiidApikey);   // Вызов функц которая скинет значения ApiKey на заводские
  server.begin();
  Serial.println(F("Сервер запушен!"));  
}

void loop() {
  server.handleClient(); // Обработка входящих запросов
}
