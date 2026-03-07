# dirwalk

POSIX-программа для рекурсивного обхода файловой системы с выводом в формате, похожем на `find`:

`dirwalk [dir] [options]`

Поддерживаемые опции:

- `-l` только символические ссылки
- `-d` только каталоги
- `-f` только регулярные файлы
- `-s` сортировка вывода согласно `LC_COLLATE`

Если `-l/-d/-f` не указаны, выводятся каталоги, регулярные файлы и симлинки.
Каталог по умолчанию: `./`.
Опции можно указывать до или после каталога, а также комбинировать (например, `-ld`).

## Сборка

```sh
make
```

## Примеры

```sh
./dirwalk
./dirwalk /etc
./dirwalk /etc -d
./dirwalk -lf /etc
./dirwalk -s /etc
```

## Проверка количества объектов (пример для /etc)

```sh
find /etc \( -type f -o -type d -o -type l \) | wc -l
find /etc -type f | wc -l
find /etc -type d | wc -l
find /etc -type l | wc -l
```

```sh
./dirwalk /etc | wc -l
./dirwalk /etc -f | wc -l
./dirwalk /etc -d | wc -l
./dirwalk /etc -l | wc -l
```
