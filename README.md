﻿
    Проект разработки сниффера на базе 
    IEEE-802.15.4 радиомодуля Olimex MRF24J40 
    и микроконтроллера STM32F429I-DISCO
    
                                Студент: Лукин П.
                                Преподаватель: Матвеев А.И.
    
		Воронеж, 2016.
    

  Подключение.


STM --- MRF

 3V --> pin.1  [VCC]
GND --> pin.2  [GND]
PE2 --> pin.9  [SCK]
PE5 --> pin.7  [SDO]
PE6 --> pin.8  [SDI]
PD5 --> pin.10 [CS]
PD2 --> pin.5  [RST]
PD4 --> pin.4  [INT]      



  Сборка.
  
Исходный код с проектом для IAR EWARM 7.50 находится в каталоге `STM-MRF-Sniffer`.

Необходимые зависимости для работы STM взяты из пакета STM32CubeF4 V1.11.0 и лежат в каталоге `STM32CubeF4`.



  Комментарии.

Для вывода информации на экран используется middleware STemWin.

Работа с GPIO и SPI происходит через HAL и BSP.

Алгоритм комманд обращения к MRF и константы адресов регистров взяты из бесплатного стэка zboss с изменением метода доступа к SPI через HAL драйверы (см. файлы: zb_mrf24j40.h, zb_mrf24j40_registers.h, zb_mrf24j40.c).

Процедура инициализации модуля присутствует в двух реализациях: из zb стэка ( zb_init_mrf24j40() ) и примера из datasheet к MRF ( ref_init_mrf24j40() ). В случае с герконом обе работают одинаково. Выбор осуществляется макросом в начале файла zb_mrf24j40.h .

На малых растояниях от источника сигнала (~ 10-20 см) радиомодуль ловит пакеты, когда настроен на отличный от источника канал. В случае с герконом, настроенным на 18 канал, MRF принимает сигнал также на 14, 22 и 26 каналах (нестабильно).




                                DSR, ВГУ, Воронеж, апрель 2016 г.