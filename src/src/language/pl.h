
#ifndef _LANGUAGE_PL_S_H_
#define _LANGUAGE_PL_S_H_

#define S_LANG "pl"

#define S_SETTING_FOR             "Ustawienia dla"
#define S_SETTING_WIFI_SSID       "Ustawienia WIFI"
#define S_WIFI_SSID               "Nazwa sieci"
#define S_WIFI_PASS               "Hasło"
#define S_HOST_NAME               "Nazwa modułu"
#define S_SETTING_SUPLA           "Ustawienia SUPLA"
#define S_SUPLA_SERVER            "Adres serwera"
#define S_SUPLA_EMAIL             "Email"
#define S_SETTING_ADMIN           "Ustawienia administratora"
#define S_LOGIN                   "Login"
#define S_LOGIN_PASS              "Hasło"
#define S_ROLLERSHUTTERS          "Rolety"
#define S_SAVE                    "Zapisz"
#define S_DEVICE_SETTINGS         "Ustawienia urządzenia"
#define S_TOOLS                   "Narzędzia"
#define S_SAVE_CONFIGURATION      "Zapisz konfigurację"
#define S_LOAD_CONFIGURATION      "Wczytaj konfigurację"
#define S_RESET_CONFIGURATION     "Resetuj ustawienia urządzenia"
#define S_RESTORE_FACTORY_SETTING "Przywróć ustawienia fabryczne"
#define S_UPDATE                  "Aktualizacja"
#define S_RESTART                 "Restart"
#define S_RETURN                  "Powrót"
#define S_CONDITION               "Warunek"

#define S_TEMPLATE_BOARD                                       "Szablon płytek"
#define S_TYPE                                                 "Rodzaj"
#define S_RELAYS                                               "PRZEKAŹNIKI"
#define S_BUTTONS                                              "PRZYCISKI"
#define S_SENSORS_1WIRE                                        "1WIRE"
#define S_SENSORS_I2C                                          "I2C"
#define S_SENSORS_SPI                                          "SPI"
#define S_SENSORS_OTHER                                        "INNE"
#define S_LED_BUTTON_CFG                                       "KONFIGURACJA"
#define S_CFG_MODE                                             "Tryb"
#define S_QUANTITY                                             "ILOŚĆ"
#define S_GPIO_SETTINGS_FOR_RELAYS                             "Ustawienie GPIO dla przekaźników"
#define S_RELAY                                                "Przekaźnik"
#define S_RELAY_NR_SETTINGS                                    "Ustawienia przekaźnika nr. "
#define S_STATE_CONTROL                                        "Załączany stanem"
#define S_REACTION_AFTER_RESET                                 "Reakcja po resecie"
#define S_GPIO_SETTINGS_FOR_BUTTONS                            "Ustawienie GPIO dla przycisków"
#define S_BUTTON                                               "Przycisk"
#define S_BUTTON_NR_SETTINGS                                   "Ustawienia przycisku nr. "
#define S_REACTION_TO                                          "Reakcja na"
#define S_RELAY_CONTROL                                        "Sterowanie przekaźnikiem"
#define S_ACTION                                               "Akcja"
#define S_GPIO_SETTINGS_FOR_LIMIT_SWITCH                       "Ustawienie GPIO dla cz. otwarcia"
#define S_LIMIT_SWITCH                                         "Krańcówka"
#define S_GPIO_SETTINGS_FOR                                    "Ustawienie GPIO dla"
#define S_FOUND                                                "Znalezione"
#define S_NO_SENSORS_CONNECTED                                 "Brak podłączonych czujników"
#define S_SAVE_FOUND                                           "Zapisz znalezione"
#define S_TEMPERATURE                                          "Temperatura"
#define S_NAME                                                 "Nazwa"
#define S_ALTITUDE_ABOVE_SEA_LEVEL                             "Wysokość m n.p.m."
#define S_GPIO_SETTINGS_FOR_CONFIG                             "Ustawienie GPIO dla CONFIG"
#define S_SOFTWARE_UPDATE                                      "Aktualizacja oprogramowania"
#define S_DATA_SAVED                                           "Dane zapisane"
#define S_RESTART_MODULE                                       "Restart modułu"
#define S_DATA_ERASED_RESTART_DEVICE                           "Dane wymazane - należy zrobić restart urządzenia"
#define S_WRITE_ERROR_UNABLE_TO_READ_FILE_FS_PARTITION_MISSING "Błąd zapisu - nie można odczytać pliku - brak partycji FS."
#define S_DATA_SAVED_RESTART_MODULE                            "Dane zapisane - restart modułu."
#define S_WRITE_ERROR_BAD_DATA                                 "Błąd zapisu - złe dane."
#define S_SETTINGS_FOR                                         "Ustawienie dla"
#define S_ACTION_TRIGGER                                       "Wyzwalaczy akcji"
#define S_ADDITIONAL                                           "Dodatkowe"

//#### SuplaConfigESP.cpp ####
#define S_STATUS_ALREADY_INITIALIZED     "Już zainicjalizowane"
#define S_STATUS_INVALID_GUID            "Nieprawidłowy identyfikator GUID"
#define S_STATUS_UNKNOWN_SERVER_ADDRESS  "Nieznany adres serwera"
#define S_STATUS_UNKNOWN_LOCATION_ID     "Nieznany identyfikator ID"
#define S_STATUS_INITIALIZED             "Zainicjowany"
#define S_STATUS_CHANNEL_LIMIT_EXCEEDED  "Przekroczono limit kanałów"
#define S_STATUS_SERVER_DISCONNECTED     "Rozłączony"
#define S_STATUS_REGISTER_IN_PROGRESS    "Rejestracja w toku"
#define S_STATUS_PROTOCOL_VERSION_ERROR  "Błąd wersji protokołu"
#define S_STATUS_BAD_CREDENTIALS         "Złe poświadczenia"
#define S_STATUS_TEMPORARILY_UNAVAILABLE "Tymczasowo niedostępne"
#define S_STATUS_LOCATION_CONFLICT       "Konflikt lokalizacji"
#define S_STATUS_CHANNEL_CONFLICT        "Konflikt kanałów - należy z clouda usunąć urządzenie i zarejestorwać je ponownie"
#define S_STATUS_REGISTERED_AND_READY    "Zarejestrowany i gotowy"
#define S_STATUS_DEVICE_IS_DISABLED      "Urządzenie jest rozłączone"
#define S_STATUS_LOCATION_IS_DISABLED    "Lokalizacja jest wyłączona"
#define S_STATUS_DEVICE_LIMIT_EXCEEDED   "Przekroczono limit urządzeń"
#define S_STATUS_REGISTRATION_DISABLED   "Rejestracja urządzeń NIEAKTYWNA"
#define S_STATUS_MISSING_CREDENTIALS     "Brak adresu e-mail"
#define S_STATUS_INVALID_AUTHKEY         "Brak klucza AuthKey"
#define S_STATUS_NO_LOCATION_AVAILABLE   "Brak dostępnej lokalizacji!"
#define S_STATUS_UNKNOWN_ERROR           "Nieznany błąd rejestracji"
#define S_STATUS_NETWORK_DISCONNECTED    "Brak połączenia z siecią"

//#### SuplaCommonPROGMEM.h ####
#define S_OFF                 "WYŁĄCZ"
#define S_ON                  "ZAŁĄCZ"
#define S_TOGGLE              "PRZEŁĄCZ"
#define S_LOW                 "LOW"
#define S_HIGH                "HIGH"
#define S_POSITION_MEMORY     "PAMIĘTAJ STAN"
#define S_REACTION_ON_PRESS   "WCIŚNIĘCIE - monostabilny"
#define S_REACTION_ON_RELEASE "ZWOLNIENIE - monostabilny"
#define S_REACTION_ON_CHANGE  "ZMIANA STANU - bistabilny"
#define S_REACTION_ON_HOLD    "PRZYTRZYMANIE"
#define S_CFG_10_PRESSES      "10 WCIŚNIĘĆ"
#define S_5SEK_HOLD           "WCIŚNIĘTY 5 SEKUND"
#define S_NORMAL              "NORMALNE"
#define S_SLOW                "WOLNE"
#define S_MANUALLY            "RĘCZNE"

#ifdef SUPLA_CONDITIONS
#define S_CONDITIONING     "Warunkowanie"
#define S_TURN_ON_WHEN     "ZAŁĄCZ, jeżeli wartość"
#define S_SWITCH_ON_VALUE  "Wartość załączenia"
#define S_SWITCH_OFF_VALUE "Wartość wyłączenia"

#define S_ON_LESS    "mniejsza"
#define S_ON_GREATER "większa"

#define S_CHANNEL_VALUE "kanału"
#define S_HUMIDITY      "wilgotność"
#define S_VOLTAGE       "napięcia[V]"
#define S_CURRENT       "natężenia[A]"
#define S_POWER         "mocy czynnej[W]"
#endif

//#### SuplaWebServer.cpp ####
#define S_LIMIT_SWITCHES "KRAŃCÓWKI"
#define S_CORRECTION     "KOREKTA DLA SENSORÓW"

//#### SuplaTemplateBoard.h ####
#define S_ABSENT "BRAK"

//#### SuplaWebPageSensor.cpp ####
#define S_IMPULSE_COUNTER                  "Licznik impulsów"
#define S_IMPULSE_COUNTER_DEBOUNCE_TIMEOUT "Limit czasu"
#define S_IMPULSE_COUNTER_RAISING_EDGE     "Zbocze rosnące"
#define S_IMPULSE_COUNTER_PULL_UP          "Podciąganie do VCC"
#define S_IMPULSE_COUNTER_CHANGE_VALUE     "Zmień wartość"
#define S_SCREEN_TIME                      "Ekran [s]"
#define S_OLED_BUTTON                      "Przycisk OLED"
#define S_SCREEN                           "Ekran"
#define S_BACKLIGHT_S                      "Podświetlenie [s]"
#define S_BACKLIGHT_PERCENT                "Jasność [%]"
#define S_ADDRESS                          "Adres"

//#### SuplaWebPageUpload.cpp ####
#define S_GENERATE_GUID_AND_KEY "Generuj GUID & AUTHKEY"
#define S_UPLOAD                "Prześlij"

//#### SuplaWebPageControl.cpp ####
#define S_SETTINGS_FOR_BUTTONS "Ustawienia dla przycisków"
#define S_REVERSE_LOGIC        "Odwrócona logika"
#define S_INTERNAL_PULL_UP     "Wewnętrzny pull-up"

//#### SuplaWebPageOther.cpp ####
#define S_CALIBRATION             "Kalibracja"
#define S_CALIBRATION_SETTINGS    "Ustawienia kalibracji"
#define S_BULB_POWER_W            "Moc żarówki [W]"
#define S_VOLTAGE_V               "Napięcie [V]"
#define S_DEPTH_CM                "Głębokość [cm]"
#define S_SENSOR_READING_DISTANCE "maksymalna odległość odczytu czujnika"
#define S_ELECTRIC_PHASE          "1/3fazy"
#define S_OPTIONAL                "(Opcjonalny)"
#define S_STATUS_LED              "Status LED"

//#### SuplaWebPageRelay.cpp ####
#define S_RELAY_ACTIVATION_STATUS "Status załączenia przekaźnika"
#define S_STATE                   "Stan"
#define S_MESSAGE                 "Wiadomość"
#define S_DIRECT_LINKS            "Linki bezpośrednie"
#define S_SENSOR                  "Czujnik"
#define S_SETTINGS_FOR_RELAYS     "Ustawienia dla przekaźników"

//#### SuplaHTTPUpdateServer.cpp ####
#define S_FLASH_MEMORY_SIZE        "Rozmiar pamięci Flash"
#define S_SKETCH_MEMORY_SIZE       "Rozmiar pamięci w szkicu"
#define S_SKETCH_LOADED_SIZE       "Rozmiar wczytanego szkicu"
#define S_SKETCH_UPLOAD_MAX_SIZE   "Maks. rozmiar przesyłanego szkicu"
#define S_UPDATE_FIRMWARE          "Aktualizacja oprogramowania"
#define S_UPDATE_SUCCESS_REBOOTING "Aktualizacja udana! Ponowne uruchamianie ..."
#define S_WARNING                  "OSTRZEŻENIE"
#define S_ONLY_2_STEP_OTA          "używaj tylko dwuetapowej aktualizacji OTA. Najpierw załaduj"

//#### SuplaOled.cpp ####
#define S_CONFIGURATION_MODE "Tryb konfiguracji"
#define S_AP_NAME            "Nazwa AP"
#define S_ERROR              "błąd"

//#### SuplaWebCorrection.cpp ####
#define S_CORRECTION_FOR_CH "Korekta dla kanałów"
#define S_CH_CORRECTION     "Korekta kanału:"

#ifdef SUPLA_RF_BRIDGE
#define S_CODES       "kodów"
#define S_NO          "Brak"
#define S_READ        "Odczytaj"
#define S_TRANSMITTER "Nadajnik"
#define S_RECEIVER    "Odbiornik"
#endif

#define S_TEMP_HYGR         "Temperatur + Wilgotność"
#define S_PRESS             "Ciśnienie"
#define S_ELECTRICITY_METER "Licznik energii elektycznej"
#define S_DISTANCE          "Odległość"
#define S_DEPTH             "Głębokość"

#ifdef SUPLA_PUSHOVER
#define S_SOUND "Dźwięk"
#endif

#define S_BAUDRATE "Prędkość komunikacji"

#endif  // _LANGUAGE_PL_S_H_
