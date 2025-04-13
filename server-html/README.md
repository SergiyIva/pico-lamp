1. Сжатие HTML-файла в формат gzip

```bash
gzip -c index.html > index.html.gz
```

2. Преобразование сжатого файла в массив байтов

```bash
xxd -i index.html.gz > nwp.cpp
```