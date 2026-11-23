# Полнота каталога второй главы

Проверена русская «Одиссея» 2.7.5.2. На ревизии `0eec6dd` в каталоге 79 строк и 76 уникальных тем. На «Альбатросе» 24 строки, на Кондере 15, на Таморе 40. «Флот орков», «Гоблины-пираты» и «Карманная кража» представлены в двух вкладках. Подтверждены 36 отсутствующих тем, которые игровые скрипты регистрируют как `LOG_MISSION` во второй главе.

## Маршрут и покрытие

| Участок | Покрытие |
| --- | --- |
| «Альбатрос» и остров Джонса | Стартовые поручения экипажа и поиск карты Джонса уже учтены. Задания острова представлены в разделе корабля. |
| Кондера | В каталоге все 15 заданий из списка прохождения. |
| Тамора | В каталоге все 38 заданий из списка прохождения и местные этапы «Флота орков» и «Гоблинов-пиратов». |
| Адармос | Отсутствуют 16 заданий. |
| Торговое судно | `TOPIC_BEGEGNUNGHANDELSSCHIFF` и `TOPIC_KONTROLLELADERAUM` уже находятся во вкладке «Альбатрос». Отсутствуют еще два задания. |
| Хорус | Отсутствуют 13 заданий. |
| Корабль орков и возвращение на «Альбатрос» | Отсутствуют пять заданий. Три из них продолжаются в Хоринисе. |

## Отсутствующие задания

Имена тем и названия взяты из русских скриптов. В последнем столбце указан один из обработчиков, создающих тему.

| Участок | Задание | Тема | Обработчик |
| --- | --- | --- | --- |
| Адармос | Остров Адармос | `TOPIC_ADARMOSMISSION` | `DIA_GERNOT_ADARMOS_INFO` |
| Адармос | Корсары на Адармосе | `TOPIC_QUESTKORSAREN` | `DIA_OFFIZIER_HI_INFO` |
| Адармос | Путь к Рошу | `TOPIC_KORSARGET` | `DIA_DAD_WHATHAPPENSNOW_INFO` |
| Адармос | Кузнец Вико | `TOPIC_FATHERSWORD` | `DIA_DAD_SCHMIED_INFO` |
| Адармос | Лодырь | `TOPIC_FELDARBEITADARMOS` | `DIA_5031_BANTOR_INFORMATION_INFO` |
| Адармос | Игра в кости с Дейлом | `TOPIC_WUERFELNADARMOS` | `DIA_DALE_PISSEINS_INFO` |
| Адармос | Разведчик корсаров | `TOPIC_MOUNTSPRUEGEL` | `DIA_ADDON_MOUNT_FINDGUY_INFO` |
| Адармос | Мясо для лагеря | `TOPIC_MOUNTFLEISCH` | `DIA_ADDON_MOUNT_NOCHWAS_INFO` |
| Адармос | Информация о Роше | `TOPIC_ROSHPETZE` | `DIA_ADDON_AL_MAUL_GAVE_INFO` |
| Адармос | Амулет Эрика | `TOPIC_ERIKSAMU` | `DIA_ERIK_HOL_INFO` |
| Адармос | Травы Бьорна | `TOPIC_KRAUTERDRUNK` | `DIA_BJORN_KRAEUTERHOL_INFO` |
| Адармос | Найденная записка | `TOPIC_EMILIOZETTEL` | `USE_ITWR_LETTERTOEMILIOFOUND` |
| Адармос | Катастрофа Эмилио | `TOPIC_QUESTEMILIO` | `DIA_EMILIO_LETTERA_A` |
| Адармос | Сокровище деревни | `TOPIC_GOLDDORF` | `DIA_OFFIZIER_YOURQUESTFORME_INFO` |
| Адармос | Темный маг Цион | `TOPIC_ZYONAQUA` | `DIA_BJORN_AQUA_INFO` |
| Адармос | Блудный сын | `TOPIC_VERLORENERSOHN` | `DIA_DAD_SOHN_INFO` |
| Торговое судно | Лихорадка на борту | `TOPIC_MEDIZINMATROSE` | `DIA_82004_BOOTSMANN_AUSKUNFT_INFO` |
| Торговое судно | Ром для Бьорна | `TOPIC_RUMBJOERN` | `DIA_82004_BOOTSMANN_RUMFUERBJOERN_INFO` |
| Хорус | Поиск провианта | `TOPIC_KHORUSPROVIANT` | `DIA_DANDOLO_KH_HALLO_INFO` |
| Хорус | Викас | `TOPIC_VIKAS` | `KH_5004_VLK_HARALD_HALLO_INFO` |
| Хорус | Орки на Хорусе | `TOPIC_ORKSAUFKHORUS` | `ORKSAUFKHORUS` |
| Хорус | Дама у круга камней | `TOPIC_VALANDRAE` | `KH_5009_NONE_VALANDRAE_HELLO_IMPORTANT_INFO` |
| Хорус | Маяк Хоруса | `TOPIC_KHORUSLEUCHTTURM` | `KH_5001_NONE_TADEUS_RAUSSPRINGEN_INFO` |
| Хорус | Мясо для Логи | `TOPIC_LOGAFLEISCH` | `NONE_2238_LOGA_JAGD_INFO` |
| Хорус | Шкуры варгов | `TOPIC_WARGJAGD` | `NONE_2240_HENRY_AUFTRAG_INFO` |
| Хорус | Освобождение Хоруса | `TOPIC_BEFREIUNGKHORUS` | `KH_5002_VLK_MARTIS_HALLO_INFO` |
| Хорус | Патрули орков | `TOPIC_ORKKUNDSCHAFTER` | `KH_5003_NONE_TOM_ORKKUNDSCHAFTER_INFO` |
| Хорус | Священная книга | `TOPIC_PETRAKAPELLE` | `KH_5007_VLK_PETRA_PROBLEM_INFO` |
| Хорус | Лекарство для Валины | `TOPIC_VALINEMEDIZIN` | `KH_5005_NONE_VALINE_MEDIZIN_INFO` |
| Хорус | Успокоительное для Колвина | `TOPIC_KOLWINSNERVENNAHRUNG` | `VLK_2243_KOLWIN_CANIHELP_INFO` |
| Хорус | Ферелл | `TOPIC_KOHLENMINE` | `KH_5002_VLK_MARTIS_KOHLENMINESUCHE_INFO` |
| Корабль орков | Пленник | `TOPIC_MARTINGEFANGEN` | `DIA_MARTIN_SHIP_HALLO_INFO` |
| Корабль орков | Письмо на языке орков | `TOPIC_ORKBRIEF` | `USE_MOD_ORKBRIEF_01` |
| «Альбатрос» после корабля орков | Руда для короля | `TOPIC_KHORINISMISSION` | `DIA_MARTIN_SHIP_ARRIVEDATSHIP_INFO` |
| «Альбатрос» после корабля орков | Темные маги в Хоринисе | `TOPIC_MODSCHWARZMAGIER` | `DIA_MARTIN_SHIP_INFOMARTINSGEFANGENNAHME_INFO` |
| «Альбатрос» после корабля орков | Важные миссии в Хоринисе | `TOPIC_KHORINISHAUPTMISSIONEN` | `DIA_MARTIN_SHIP_INFOMARTINSGEFANGENNAHME_INFO` |

## Учет текущего визита

- У «Флота орков» уже есть строки на Кондере и Таморе. Адармос, торговое судно, Хорус и перевод письма на «Альбатросе» продолжают эту цепочку. Для каждого визита нужны свои условия завершения местного этапа.
- Три задания для Хориниса появляются на «Альбатросе» до начала третьей главы. Их выполнение в Хоринисе относится к следующей главе. Процент до отплытия учитывает только действия, которые можно закончить на текущей остановке.
- На повторных визитах появляются отдельные задания, например «Куда делся Колвин?» на Хорусе. Они относятся к главам этих визитов. Скрипты также различают задания и записи `LOG_NOTE`, например «Мой сундук».
- Журнал распознает Кондеру, Тамору и сцены корабля `SEAWORLD`, `SEAWORLD2`, `SEAWORLD3` и `SEAWORLDFIGHT`. Последние четыре пока относятся к одной вкладке «Альбатрос». При расширении каталога нужно проверить автовыбор по миру и сюжетному этапу, в том числе при загрузке сохранения на корабле.

Источники для сверки маршрута и состава заданий — [руководство по «Одиссее» 2.7.4](https://upload.worldofplayers.de/files12/Komplettloesung_Odyssee_2.7.4_2025_04_06.pdf) и [список квестов RPG Russia](https://rpgrussia.com/threads/obschij-spisok-kvestov.13601/). Названия и условия сверялись с русским `GOTHIC.DAT` 2.7.5.2, SHA-256 `93e2787a1945d8c4a28b13ad504b0003ca396c621676dca742729e2183e41b50`.
