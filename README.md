# opensource_clock
## Проект часов на газоразрядных индикаторах с открытым исходным кодом и схемой

Изначально часы делались под ИН-1, но в будущем планируется унификация проекта под любые ГРИ. Часы взимодействую с модулем RDA5807M для получения времени и с микросхемой времени DS3231. Возможны три варианта: FM+RTC, FM, RTC (у вас обе микросхемы или только какая-то одна). Выбор режима реализован с помощью define MODE перед компиляцией.

Схему можно посмотреть [здесь](https://easyeda.com/naym1993/RDS_Clock_lamp)

По поводу резисторов. В ходе испытаний, был найден хороший вариант подключения (спасибо тебе человек со стрима) с использованием одного стабилитрона и 10 выпрямительных диодов (ниже на фото). С помощью такой схемы можно избежать установки 10 стабилитронов. Резисторы при необходимости устанавливаются с обратной стороны платы навестным монтажем, однако в ходе испытаний с ИН-12 их присутствие не понадобилось, между катодами стабилизировалось напряжение в 75В.

![Как паять диоды и резисторы](https://pp.userapi.com/c853420/v853420123/2b7db/BvtVi_LcUzg.jpg "Как паять диоды и резисторы")

Подписывайтесь на группу [Radionews](https://vk.com/bestradionews)

## Используемые материалы при написании прошивки

Проект STM для модулей RDA5807 <https://github.com/papadkostas/RDA5807M>

Библиотека I2C <http://cxem.net/mc/mc420.php>

Подробное описание модуля RDA5807 <http://cxem.net/tuner/tuner87.php>
