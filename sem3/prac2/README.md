ЧТО ДЕЛАЮТ ПРОГРАММЫ:

utf2ucs.c: Переводит текст из формата UTF-8 в формат UTF-16 (UCS).
Программа обрабатывает BOM входного файла: Пропускает его если он есть.
Программа также обрабатывает BOM выходного файла: Если он есть, программа перезаписывает его, чтобы не потерять. Если его нет, то он не появится.

ucs2utf.c: Переводит текст из формата UTF-16 (UCS) в формат UTF-8.
Программа игнорирует символы, хранимые в суррогатных парах, выдавая предупреждение.
Программа обрабатывает BOM входного файла: Если он есть, устанавливает соответсвтующий порядок BOM, иначе считается что порядок Little Endian.
Программа также обрабатывает BOM выходного файла: Если он есть, программа перезаписывает его, чтобы не потерять. Если его нет, то он не появится.
