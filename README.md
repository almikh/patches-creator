# patches-creator
Программа для создания патчей к проекту

Для обновления данных запустить с флагами
> -u -s storage.xml \<target-path\>

Создастся файл с данными о файлах в указанной директории.
После модификации запустить с флагами
> -c -s storage.xml -t \<dest-path\> \<target-path\>

и в папку \<dest-path\> будут помещены все файлы, данные о которых изменились по сравнению с сохраненными в storage.xml
