
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_DC  9
#define TFT_CS  10

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

/*---WiFi User Name and Password---*/
const String ssid = "TestWiFi";         /*--Replace with your Network Name--*/
const String pswd = "12345678";         /*--Replace with your Network Password--*/

const String GET_REQ_PRE = "GET /data/2.5/weather?q=";
const String GET_REQ_POST = "&APPID=ENTERYOURAPIKEYHERE&units=metric";   /*--Replace with your API Key--*/

const char WIFI_CONNECTED[] = "WIFI CONNECTED";
const char WIFI_GOT_IP[] = "WIFI GOT IP";
const char TCP_CONNECT[] = "CONNECT";
const char SEND_OK[] = "SEND OK";

/*---Buffer and Index to process response from ESP8266 Module---*/
uint8_t buff_idx = 0;
uint8_t buff[50u] = { 0 };
/*---Separate Buffer to Process the GET Response which contains weather data--*/
uint16_t weather_idx = 0u;
char weather_buff[700u] = { 0 };

uint8_t IP_ADDRESS[17u] = { 0 };    /*--Store IP Address--*/
uint8_t MAC_ADDRESS[18u] = { 0 };   /*--Store MAC Address--*/

#define NUM_OF_CITIES           4u
#define WEATHER_DATA_SIZE       10u

typedef struct _city_info_s
{
  String city_name;
  uint8_t len;
} city_info_s;

/*--Weather Data--*/
char temp_normal[WEATHER_DATA_SIZE] = { 0 };        /*--degree celcius--*/
char temp_real_feel[WEATHER_DATA_SIZE] = { 0 };     /*--degree celcius--*/
char temp_minimum[WEATHER_DATA_SIZE] = { 0 };       /*--degree celcius--*/
char temp_maximum[WEATHER_DATA_SIZE] = { 0 };       /*--degree celcius--*/
char pressure[WEATHER_DATA_SIZE] = { 0 };           /*--Pascal--*/
char humidity[WEATHER_DATA_SIZE] = { 0 };           /*--Percentage--*/
char wind_speed[WEATHER_DATA_SIZE] = { 0 };         /*--meter/sec--*/

/*--Total Number of cities--*/
const city_info_s s_city_info[NUM_OF_CITIES] = 
{
  { "London",     84u },
  { "Shimla",     84u },
  { "Delhi",      83u },
  { "New York",   86u }
};

#define TFT_TXT_LEN     27u
char tft_buff_s[TFT_TXT_LEN] = { 0 };     /*--TFT Data to write on a line with Text Size 2--*/

/*------------------------------FUNCTION PROTOTYPES---------------------------*/
uint8_t send_echo_off( void );
uint8_t send_disconnect_ap( void );
uint8_t send_at( uint32_t timeout );
uint8_t send_mode( uint8_t mode, uint32_t timeout );
uint8_t send_join_ap( String ssid, String pswd, uint32_t timeout );
uint8_t get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout );
uint8_t send_connect_cmd( uint32_t timeout );
uint8_t send_close_cmd( void );
uint8_t send_num_of_bytes( uint8_t no_bytes, uint32_t timeout);
uint8_t send_get_req( String city_name, uint32_t timeout );

uint8_t check_for_ok( uint32_t timeout );
uint8_t check_for_join_ap( uint32_t timeout );
uint8_t check_get_ip_mac_address( uint8_t ip, uint8_t mac, uint32_t timeout );
uint8_t check_connect_cmd( uint32_t timeout );
uint8_t check_for_num_of_bytes( uint32_t timeout );
uint8_t check_get_req( uint32_t timeout );

uint8_t get_temperature( char *temp );
uint8_t get_real_feel( char *temp );
uint8_t get_min_temp( char *temp );
uint8_t get_max_temp( char *temp );
uint8_t get_pressure( char *temp );
uint8_t get_humidity( char *temp );
uint8_t get_wind_speed( char *temp );
uint8_t get_weather_data( char *temp, const char *search_str );

void flush_serial_data( void );
void flush_buffer( void );
void progress_bar( uint8_t percentage );


void setup() 
{
  /*--Initialize the Serial Module, ESP8266 is connected with this--*/
  /*--Make sure to change the baud rate of ESP8266 module to 9600 using AT+UART_DEF command--*/
  Serial.begin( 9600 );
  /*--Initialize TFT--*/
  tft.begin();
  delay(100);
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.setCursor(10,10);
  tft.println(" EMBEDDED        ");
  tft.setTextColor(ILI9341_NAVY);
  tft.setCursor(0,34);
  tft.println("       LABORATORY");
  tft.setTextSize(2);
  tft.setCursor(0, 72);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextColor(ILI9341_GREEN);
  tft.println("     OPEN WEATHER API     ");
  tft.println("     Weather  Station     ");

  /*--Send ECHO Off Command--*/
  send_echo_off();
  progress_bar(10);
  /*--Check if module is responding with OK response--*/
  while ( !send_at(2000) )
  {
    delay(1000);
  }
  progress_bar(30);

  delay(500);
  /*--Set module in Station Mode--*/
  while( !send_mode(1,2000) )
  {
    /*--if control comes here, this means problem in setting mode--*/
    delay(500);
  }
  progress_bar(60);

  delay(500);
  /*--Join with Access Points--*/
  while( !send_join_ap(ssid, pswd, 6000) )
  {
    /*--unable to connect, so retry once again--*/
    delay(500);
  }
  progress_bar(80);
  
  /*--Get IP Address and MAC Address from the Module--*/
  while( !get_ip_mac_address( IP_ADDRESS, MAC_ADDRESS, 2000) )
  {
    /*--unable to get, try again--*/
    delay(500);
  }
  progress_bar(100);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  snprintf( tft_buff_s, TFT_TXT_LEN, " IP:  %s", (char*)IP_ADDRESS);
  tft.setCursor(0,168);
  tft.print( tft_buff_s);
  snprintf( tft_buff_s, TFT_TXT_LEN, " MAC: %s", (char*)MAC_ADDRESS);
  tft.setCursor(0,184);
  tft.print( tft_buff_s);
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
}


void loop(void)
{
  uint8_t idx = 0;
  for( idx=0u; idx<NUM_OF_CITIES; idx++)
  {
    /*--Connect the Open Weather Map--*/
    while( !send_connect_cmd( 2000 ) )
    {
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(0,231);
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      tft.println("Connection Status: FAIL   ");
    }
    tft.setCursor(0,231);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.println("Connection Status: PASS   ");
    
    tft.setTextSize(2);
    tft.setCursor(8,8);
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK );
    delay(500);
    /*--Connection is OK, send Number of Bytes to Send--*/
    while( !send_num_of_bytes( s_city_info[idx].len, 3000) )
    {
    }
    delay(500);
    /*--Send GET Request and Receive Data--*/
    if( send_get_req( s_city_info[idx].city_name, 5000u ) == true )
    {
      tft.fillRect(8, 8, tft.width(), 168, ILI9341_BLACK);
      tft.println("City Name: " + s_city_info[idx].city_name);
      /*---Temperature Data--*/
      tft.setCursor(8,40);
      snprintf( tft_buff_s, TFT_TXT_LEN, "Temp:%s C", temp_normal);
      tft.setCursor(8,56);
      tft.println( tft_buff_s );
      snprintf( tft_buff_s, TFT_TXT_LEN, "Real Feel:%s C", temp_real_feel);
      tft.setCursor(8,72);
      tft.println( tft_buff_s );
      snprintf( tft_buff_s, TFT_TXT_LEN, "Min.Temp:%s C", temp_minimum);
      tft.setCursor(8,88);
      tft.println( tft_buff_s );
      snprintf( tft_buff_s, TFT_TXT_LEN, "Max.Temp:%s C", temp_maximum);
      tft.setCursor(8,104);
      tft.println( tft_buff_s );
      /*---Pressure Data--*/
      snprintf( tft_buff_s, TFT_TXT_LEN, "Pressure:%s Pa", pressure);
      tft.setCursor(8,120);
      tft.println( tft_buff_s );
      /*---Humidity Data--*/
      snprintf( tft_buff_s, TFT_TXT_LEN, "Humidity:%s %%", humidity);
      tft.setCursor(8,136);
      tft.println( tft_buff_s );
      /*---Wind Speed Data--*/
      snprintf( tft_buff_s, TFT_TXT_LEN, "Wind Speed:%s m/s", wind_speed);
      tft.setCursor(8,152);
      tft.println( tft_buff_s );
    }
    delay(2000);
  }
}

/*-----------------------------FUNCTION DEFINITIONS---------------------------*/
/**
 * @brief Send Echo Off Command
 * 
 * This function sends the ECO Off Command to ESP Module and then flushes 
 * the serial data.
 * @return Status of the function, true or false TODO:
 */
uint8_t send_echo_off( void )
{
  Serial.println("ATE0");
  delay(500);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return true;
}

/**
 * @brief Disconnect from Access Points
 * 
 * This function sends the command to disconnect from connect access points
 * @return Status of the function, true or false TODO:
 */
uint8_t send_disconnect_ap( void )
{
  Serial.println("AT+CWQAP");
  delay(1000);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return true;
}

/**
 * @brief Send AT Command
 * 
 * This function sends the AT command to module and check if the response is
 * valid.
* @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_at( uint32_t timeout )
{
  uint8_t status = false;
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT");
  status = check_for_ok( timeout );
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

/**
 * @brief Disconnect from Access Points
 * 
 * This function sends the command to disconnect from connect access points
 * @param mode TODO: not used
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_mode( uint8_t mode, uint32_t timeout )
{
  uint8_t status = false;
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CWMODE=1");
  // Serial.print(mode);
  // Serial.println();
  status = check_for_ok( timeout );
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

/**
 * @brief Send Command to Join Access Point
 * 
 * This function sends the command to join the access point.
 * @param ssid WiFi User Name
 * @param pswd WiFi Password
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_join_ap( String ssid, String pswd, uint32_t timeout )
{
  uint8_t status = false;
  /*--before connecting send the disconnect command--*/
  send_disconnect_ap();
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  String packet = "AT+CWJAP=\"" + ssid + "\",\"" + pswd + "\"";
  Serial.println(packet);
  status = check_for_join_ap(timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

/**
 * @brief Get IP and MAC Address
 * 
 * This function sends the command to get the IP and MACS Address from the WiFi
 * Module
 * @param ip pointer to IP Address Buffer
 * @param mac pointer to MAC Address Buffer
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout )
{
  uint8_t status = false;
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CIFSR");
  status = check_get_ip_mac_address(ip, mac, timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;
}

/**
 * @brief Connect with TCP Server
 * 
 * This function sends the command to connect with TCP Server
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t send_connect_cmd( uint32_t timeout )
{
  uint8_t status = false;
  /*--Before connecting send close the connection--*/
  send_close_cmd();
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.println("AT+CIPSTART=\"TCP\",\"api.openweathermap.org\",80");
  status = check_connect_cmd( timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;  
}

/**
 * @brief Disconnect with the Server
 * 
 * This function sends the command to disconnect from the server
 * @return Status of the function, true or false TODO:
 */
uint8_t send_close_cmd( void )
{
  /*--For Single Connections--*/
  Serial.println("AT+CIPCLOSE");
  delay(1000);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return true;
}

/**
 * @brief Send Number of Bytes
 * 
 * This function sends the CIPSEND command with no. of bytes
 * @param no_bytes length of GET Request
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false TODO:
 */
uint8_t send_num_of_bytes( uint8_t no_bytes, uint32_t timeout)
{
  uint8_t status = false;
  /*--Flush Command Response Buffer--*/
  flush_buffer();
  Serial.print("AT+CIPSEND=");
  Serial.println(no_bytes);
  status = check_for_num_of_bytes( timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;  
}

/**
 * @brief Send GET Request
 * 
 * This function sends the GET request to OpenWeather website
 * @param String city name
 * @param timeout Timeout value to exit the function with failure
 * @param Weather Data Pointer
 * @return Status of the function, true or false TODO:
 */
uint8_t send_get_req( String city_name, uint32_t timeout )
{
  uint8_t status = false;
  String get_req = GET_REQ_PRE + city_name + GET_REQ_POST;
  /*--Flush Weather Response Buffer--*/
  flush_weather_buffer();
  Serial.println( get_req );
  status = check_get_req(timeout);
  /*--Flush the Serial Data--*/
  flush_serial_data();
  return status;  
}

/**
 * @brief Check the OK Response
 * 
 * This function checks for the OK response from the module
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_ok( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    /*--Check for \r\nOK\r\n Response--*/
    if( buff[0] == '\r' && buff[1] == '\n' && buff[2] == 'O' && buff[3] == 'K' && buff[4] == '\r' && buff[5] == '\n' )
    {
      status = true;
    }
  }
  return status;
}

/**
 * @brief Check the Response of Access Point Join Command
 * 
 * This function checks the response of CWJAP command, and if joining is 
 * successfull, it returns true
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_join_ap( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    /*--heck for \r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n Response--*/
    if( strncmp( (char*)buff, WIFI_CONNECTED, sizeof(WIFI_CONNECTED)-1 ) == 0 )
    {
      if( strncmp( (char*)(buff+16), WIFI_GOT_IP, sizeof(WIFI_GOT_IP)-1 ) == 0 )
      {
        if( buff[33] == '\r' && buff[34] == '\n' )
        {
          status = true;
        }
      }
    }
  }
  return status;
}

/**
 * @brief Check the Response of CIFSR Command
 * 
 * This function checks the response of CIFSR command, and then parse the data
 * into IP Address and MAC Address Array
 * @param ip pointer to IP Address Buffer
 * @param mac pointer to MAC Address Buffer
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_get_ip_mac_address( uint8_t *ip, uint8_t *mac, uint32_t timeout )
{
  /*The data format is as below
  +CIFSR:STAIP,"192.168.43.160"{0D}{0A}
  +CIFSR:STAMAC,"5c:cf:7f:ae:3f:09"{0D}{0A}{0D}{0A}OK{0D}{0A}
  */
  uint32_t timestamp;
  uint8_t status = false;
  uint8_t idx = 0u;
  uint8_t index = 0;
  uint8_t found = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      // Check if data is received
      if( buff_idx > 4u && buff[buff_idx] == '\n' && buff[buff_idx-1] == '\r' && buff[buff_idx-2] == 'K' && buff[buff_idx-3] == 'O' )
      {
        status = true;
      }
      buff_idx++;
    }
  }
  
  // Data is Received Completely, Now
  // Look for IP Address
  while( (idx < sizeof(buff)) && found == false && (millis()-timestamp <= timeout) )
  {
    if( buff[idx] == '\"' )
    {
      // the value of idx points to IP Address Information
      found = true;
    }
    idx++;
  }
  if( found == true )
  {
    // Copy IP Address Data
    index = 0;
    while( buff[idx] != '\"' )
    {
      *(ip+index) = buff[idx];
      index++;
      idx++;
    }
    idx++;
    *(ip+index) = 0;  // Added Null Character
  }
  // Look for MAC Address
  found = false;
  while( (idx < sizeof(buff)) && found == false && (millis()-timestamp <= timeout) )
  {
    if( buff[idx] == '\"' )
    {
      // the value of idx points to IP Address Information
      found = true;
    }
    idx++;
  }
  if( found == true )
  {
    // COPY MAC Address
    index = 0;
    while( buff[idx] != '\"' )
    {
      *(mac+index) = buff[idx];
      index++;
      idx++;
    }
    *(mac+index) = 0;  // Added Null Character
  }
  return status;
}

/**
 * @brief Check the Response of CIPSTART Command
 * 
 * This function checks the response of CIPSTART command.
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_connect_cmd( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    /*--valid response from module is CONNECT{0D}{0A}{0D}{0A}OK{0D}{0A} --*/
    if( strncmp( (char*)buff, TCP_CONNECT, sizeof(TCP_CONNECT)-1 ) == 0 )
    {
      if( buff[7] == '\r' && buff[8] == '\n' && buff[9] == '\r' && buff[10] == '\n' && \
          buff[11] == 'O' && buff[12] == 'K' && buff[13] == '\r' && buff[14] == '\n' )
      {
        status = true;
      }
    }
  }
  return status;
}

/**
 * @brief Check the Response of CIPSEND Command
 * 
 * This function checks the response of CIPSEND command.
 * @param timeout Timeout value to exit the function with failure
 * @return Status of the function, true or false
 */
uint8_t check_for_num_of_bytes( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      buff[buff_idx] = Serial.read();
      buff_idx++;
    }
    /*--Check for \r\nOK\r\n>Response--*/
    if( buff[0] == '\r' && buff[1] == '\n' && buff[2] == 'O' && buff[3] == 'K' && buff[4] == '\r' && buff[5] == '\n' && \
        buff[6] == '>' )
    {
      status = true;
    }
  }
  return status;
}

#define SEARCH_COUNTER      20u
/**
 * @brief Check the Response of GET Request Command
 * 
 * This function checks the response of the GET Request and parse the data and update the buffers
 * @param timeout Timeout value to exit the function with failure
 * @param Weather Data Pointer
 * @return Status of the function, true or false
 */
uint8_t check_get_req( uint32_t timeout )
{
  uint32_t timestamp;
  uint8_t status = false;
  timestamp = millis();
  while( (millis() - timestamp <= timeout) && (status == false) )
  {
    if( Serial.available() > 0 )
    {
      weather_buff[weather_idx] = Serial.read();
      weather_idx++;
    }
    /*--we will get a lot of data from the module, and we have to first wait till data is completely received--*/
    /*--we receive this packet at last CLOSED\r\n, so we will wait till we get this data--*/
    /*Example of Data Received
    Recv 86 bytes{0D}{0A}{0D}{0A}SEND OK{0D}{0A}{0D}{0A}+IPD,554:{"coord":{"lon":-74.006,"lat":40.7143},"
    weather":[{"id":600,"main":"Snow","description":"light snow","icon":"13n"},
    {"id":701,"main":"Mist","description":"mist","icon":"50n"}],"base":"stations",
    "main":{"temp":-1.67,"feels_like":-9.42,"temp_min":-3,"temp_max":-0.56,"pressure":1021,"humidity":93},"visibility":9656,
    "wind":{"speed":7.72,"deg":30,"gust":10.29},"snow":{"1h":0.27},"clouds":{"all":90},"dt":1613730994,
    "sys":{"type":1,"id":4610,"country":"US","sunrise":1613735067,"sunset":1613774132},"timezone":-18000,"id":5128581,"name":"New York","cod":200}
    CLOSED{0D}{0A}--*/
    /*--TODO: poor way of doing, need to improve this--*/
    if( weather_idx > 10u && weather_buff[weather_idx-1] == '\n' && weather_buff[weather_idx-2] == '\r' && weather_buff[weather_idx-3] == 'D' && \
        weather_buff[weather_idx-4] == 'E' && weather_buff[weather_idx-5] == 'S' && weather_buff[weather_idx-6] == 'O' && weather_buff[weather_idx-7] == 'L' && weather_buff[weather_idx-8] == 'C' )
    {
      /*--this means that data has been received correctly and now we can proceed with parsing of the data--*/
      status = true;
    }
  }
  /*--parse data if status is true--*/
  if( status )
  {
    status = get_temperature( temp_normal );
  }
  if( status )
  {
    status = get_real_feel( temp_real_feel );
  }
  if( status )
  {
    status = get_min_temp( temp_minimum );
  }
  if( status )
  {
    status = get_max_temp( temp_maximum);
  }
  if( status )
  {
    status = get_pressure( pressure );
  }
  if( status )
  {
    get_humidity( humidity );
  }
  if( status )
  {
    status = get_wind_speed( wind_speed );
  }
  return status;
}

const char temp_str[] = "temp";
const char temp_real_feel_str[] = "feels_like";
const char temp_min_str[] = "temp_min";
const char temp_max_str[] = "temp_max";
const char pressure_str[] = "pressure";
const char humidity_str[] = "humidity";
const char wind_speed_str[] = "speed";

/**
 * @brief Get Normal Temperature
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_temperature( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, temp_str );
  return status;
}

/**
 * @brief Get Real Feel Temperature
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_real_feel( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, temp_real_feel_str );
  return status;
}

/**
 * @brief Get Minimum Temperature
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_min_temp( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, temp_min_str );
  return status;
}

/**
 * @brief Get Maximum Temperature
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_max_temp( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, temp_max_str );
  return status;
}

/**
 * @brief Get Pressure
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_pressure( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, pressure_str );
  return status;
}

/**
 * @brief Get Humidity
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_humidity( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, humidity_str );
  return status;
}

/**
 * @brief Get Wind Speed
 * 
 * This function calls the get_weather_data function, with the relevant 
 * buffer to be updated
 * @param pointer to buffer
 */
uint8_t get_wind_speed( char *temp )
{
  uint8_t status = true;
  status = get_weather_data( temp, wind_speed_str );
  return status;
}
/**
 * @brief Get Weather Data
 * 
 * This function parse the weather_buff data based on the search string
 * and update the data into the buffers pointed by pointers
 * @param pointer to buffer
 */
uint8_t get_weather_data( char *temp, const char *search_str )
{
  uint8_t idx = 0u;
  uint8_t status = true;
  uint8_t counter = 0u;
  char *pointer;
  /*--search for "temp", data is like "temp":-1.67--*/
  pointer = strstr( (char*)weather_buff, search_str );
  while( *pointer != '\"' && counter < SEARCH_COUNTER )
  {
    pointer++;
    counter++;
  }
  /*--If found copy data into buffer--*/
  if( counter < SEARCH_COUNTER )
  {
    counter = 0u;
    /*--SW has found quote of "temp":-1.67, and now copy data till we get ,--*/
    pointer++;  // reached til colon :
    pointer++;  // now reached -minus
    idx = 0u;
    while( (*pointer != ',') && (*pointer != '}') && (counter < SEARCH_COUNTER) )
    {
      *(temp+idx) = *pointer;
      idx++;
      pointer++;
      counter++;
    }
    *(temp+idx) = 0;    // Added NULL Character
  }
  else
  {
    status = false;
  }
  return status;
}

/**
 * @brief Clear the Buffer
 * 
 * This function resets the buffer index and buffer content
 */
void flush_buffer( void )
{
  buff_idx = 0x00;
  memset( buff, 0x00, sizeof(buff) );
}

/**
 * @brief Clear the Weather Buffer
 * 
 * This function resets the buffer index and buffer content
 */
void flush_weather_buffer( void )
{
  weather_idx = 0x00;
  memset( weather_buff, 0x00, sizeof(weather_buff) );
}

/**
 * @brief Flush the Serial Data
 * 
 * This function flushes the Serial Data from the internal buffers
 */
void flush_serial_data( void )
{
  uint8_t temp = 0;
  while( Serial.available() > 0 )
  {
    temp = Serial.read();
  }
}

const uint16_t p_x = 0;
const uint16_t p_y = 221;
const uint16_t p_w = 320;
const uint16_t p_h = 16;
/**
 * @brief Display Progress Bar at Bottom
 * 
 * This function dispays a progress bar based on the percentage input
 * @param percentage of progress
 */
void progress_bar( uint8_t percentage )
{
  uint16_t progress = 0;
  if( percentage > 100u )
    percentage = 100u;
  progress = (uint16_t)((320 * (uint16_t)percentage)/100);
  tft.drawRect(p_x, p_y, p_w, p_h, ILI9341_CYAN);
  tft.fillRect(p_x, p_y, progress, p_h, ILI9341_DARKCYAN);
}
