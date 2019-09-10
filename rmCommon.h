typedef struct{
  String PackId;
  String City;
  String Temp;
  String AirPress;
  String Sup;
  String Sunder;
} Weather;

typedef struct{
  String PackId;
  String Icon;
  String MaxTemp;
  String MinTemp;
  String Wind;
  String WindDir;
  String RainProcent;
  String SunProcent;
} ForeCast;


typedef struct{
  String PackId;
  String Text;
} WeatherText;

typedef struct {
  String  Sup;
  String  Sunder;
} SupSunder;


const String CONST_WTHDFT = "WTH_DFT"; // default weather struct
const String CONST_WTHTOD = "WTH_TOD"; // today weather struct
const String CONST_WTHTMO = "WTH_TMO"; // tomorrow weather struct
const String CONST_WTHDAT = "WTH_DAT"; // day after tomorrow weather struct
const String CONST_WTHTXT = "WTH_TXT"; // weather text;
const String CONST_WTHWARN = "WTH_WARN"; // weather alarm struct


