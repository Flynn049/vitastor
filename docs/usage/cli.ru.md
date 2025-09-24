[Документация](../../README-ru.md#документация) → Использование → Консольный интерфейс Vitastor

-----

[Read in English](cli.en.md)

# Консольный интерфейс Vitastor

vitastor-cli - интерфейс командной строки для административных задач, таких, как
управление образами дисков.

Поддерживаются следующие команды:

- [status](#status)
- [df](#df)
- [ls](#ls)
- [create](#create)
- [snap-create](#create)
- [modify](#modify)
- [dd](#dd)
- [rm](#rm)
- [flatten](#flatten)
- [rm-data](#rm-data)
- [merge-data](#merge-data)
- [describe](#describe)
- [fix](#fix)
- [alloc-osd](#alloc-osd)
- [rm-osd](#rm-osd)
- [osd-tree](#osd-tree)
- [ls-osd](#ls-osd)
- [modify-osd](#modify-osd)
- [pg-list](#pg-list)
- [create-pool](#create-pool)
- [modify-pool](#modify-pool)
- [ls-pools](#ls-pools)
- [rm-pool](#rm-pool)

Глобальные опции:

```
--config_path FILE   Путь к файлу конфигурации Vitastor
--etcd_address URL   Адрес соединения с etcd
--iodepth N          Отправлять параллельно N операций на каждый OSD (по умолчанию 32)
--parallel_osds M    Работать параллельно с M OSD (по умолчанию 4)
--progress 1|0       Печатать прогресс выполнения (по умолчанию 1)
--cas 1|0            Для команд flatten, merge, rm - использовать CAS при записи (по умолчанию - решение принимается автоматически)
--no-color           Отключить цветной вывод
--json               Включить JSON-вывод
```

## status

`vitastor-cli status`

Показать состояние кластера.

Пример вывода:

```
  cluster:
    etcd: 1 / 1 up, 1.8 M database size
    mon:  1 up, master stump
    osd:  8 / 12 up

  data:
    raw:   498.5 G used, 301.2 G / 799.7 G available, 399.8 G down
    state: 156.6 G clean, 97.6 G misplaced
    pools: 2 / 3 active
    pgs:   30 active
           34 active+has_misplaced
           32 offline

  io:
    client:    0 B/s rd, 0 op/s rd, 0 B/s wr, 0 op/s wr
    rebalance: 989.8 M/s, 7.9 K op/s
```

## df

`vitastor-cli df`

Показать список пулов и занятое место.

Пример вывода:

```
NAME      SCHEME  PGS  TOTAL    USED    AVAILABLE  USED%   EFFICIENCY
testpool  2/1     32   100 G    34.2 G  60.7 G     39.23%  100%
size1     1/1     32   199.9 G  10 G    121.5 G    39.23%  100%
kaveri    2/1     32   0 B      10 G    0 B        100%    0%
```

В примере у пула "kaveri" эффективность равна нулю, так как все OSD выключены.

## ls

`vitastor-cli ls [-l] [-p POOL] [--sort FIELD] [-r] [-n N] [<glob> ...]`

Показать список образов, если передан(ы) шаблон(ы) `<glob>`, то только с именами,
соответствующими одному из шаблонов (стандартные ФС-шаблоны с * и ?).

Опции:

```
--exact         Не применять ФС-шаблоны к именам, выводить только точные совпадения
-p|--pool POOL  Фильтровать образы по пулу (ID или имени)
-l|--long       Также выводить статистику занятого места и ввода-вывода
--del           Также выводить статистику операций удаления
--sort FIELD    Сортировать по заданному полю (name, size, used_size, <read|write|delete>_<iops|bps|lat|queue>)
-r|--reverse    Сортировать в обратном порядке
-n|--count N    Показывать только первые N записей
--tree          Вывести снапшоты и клоны в виде дерева
```

Пример вывода:

```
NAME                 POOL      SIZE  USED    READ   IOPS  QUEUE  LAT   WRITE  IOPS  QUEUE  LAT   FLAGS  PARENT
debian9              testpool  20 G  12.3 G  0 B/s  0     0      0 us  0 B/s  0     0      0 us     RO
pve/vm-100-disk-0    testpool  20 G  0 B     0 B/s  0     0      0 us  0 B/s  0     0      0 us      -  debian9
pve/base-101-disk-0  testpool  20 G  0 B     0 B/s  0     0      0 us  0 B/s  0     0      0 us     RO  debian9
pve/vm-102-disk-0    testpool  32 G  36.4 M  0 B/s  0     0      0 us  0 B/s  0     0      0 us      -  pve/base-101-disk-0
debian9-test         testpool  20 G  36.6 M  0 B/s  0     0      0 us  0 B/s  0     0      0 us      -  debian9
bench                testpool  10 G  10 G    0 B/s  0     0      0 us  0 B/s  0     0      0 us      -
bench-kaveri         kaveri    10 G  10 G    0 B/s  0     0      0 us  0 B/s  0     0      0 us      -
```

## create

`vitastor-cli create -s|--size <size> [-p|--pool <id|name>] [--parent <parent_name>[@<snapshot>]] <name>`

Создать образ. Для размера `<size>` можно использовать суффиксы K/M/G/T (килобайт-мегабайт-гигабайт-терабайт).
Если указана опция `--parent`, создаётся клон образа. Родитель `<parent_name>[@<snapshot>]` должен быть
снимком (или просто немодифицируемым образом). Пул обязательно указывать, если в кластере больше одного пула.

```
vitastor-cli create --snapshot <snapshot> [-p|--pool <id|name>] <image>
vitastor-cli snap-create [-p|--pool <id|name>] <image>@<snapshot>
```

Создать снимок образа `<name>` (можно использовать любую форму команды). Снимок можно создавать без остановки
клиентов, если пишущий клиент максимум 1.

Смотрите также информацию о том, [как экспортировать снимки](qemu.ru.md#экспорт-снимков).

## modify

`vitastor-cli modify <name> [--rename <new-name>] [--resize <size>] [--readonly | --readwrite] [-f|--force] [--down-ok]`

Изменить размер, имя образа или флаг "только для чтения". Снимать флаг "только для чтения"
и уменьшать размер образов, у которых есть дочерние клоны, без `--force` нельзя.

Если новый размер меньше старого, "лишние" данные будут удалены, поэтому перед уменьшением
образа сначала уменьшите файловую систему в нём.

* `--deleted 1|0` - Установить/снять флаг "образ удалён" (устанавливается при незавершённом удалении).
* `-f|--force` - Разрешить уменьшение или перевод в чтение-запись образа, у которого есть клоны.
* `--down-ok` - Разрешить уменьшение, даже если часть данных останется неудалённой на недоступных OSD.

## dd

```
vitastor-cli dd [iimg=<image> | if=<file>] [oimg=<image> | of=<file>] [bs=1M] \
    [count=N] [seek/oseek=N] [skip/iseek=M] [iodepth=N] [status=progress] \
    [conv=nocreat,noerror,nofsync,trunc,nosparse] [iflag=direct] [oflag=direct,append]
```

Копировать данные между образами Vitastor, файлами и каналами.

Опции можно передавать в классическом стиле dd (`key=value`) или как обычно (`--key value`).

| <!-- -->        | <!-- -->                                                                |
|-----------------|-------------------------------------------------------------------------|
| `iimg=<image>`  | Копировать из образа Vitastor `<image>`                                 |
| `if=<file>`     | Копировать из файла `<file>`                                            |
| `oimg=<image>`  | Копировать в образ Vitastor `<image>`                                   |
| `of=<file>`     | Копировать в файл `<file>`                                              |
| `bs=1M`         | Задать размер блока копирования                                         |
| `count=N`       | Копировать не более N блоков. Если N заканчивается на B - то N байт.    |
| `seek/oseek=N`  | Пропустить N выходных блоков. Если N заканчивается на B - то N байт.    |
| `skip/iseek=N`  | Пропустить N входных блоков. Если N заканчивается на B - то N байт.     |
| `iodepth=N`     | Отправлять N чтений/записей параллельно (по умолчанию 4).               |
| `status=LEVEL`  | Уровень вывода в консоль: none/noxfer/progress                          |
| `size=N`        | Задать размер выходного файла/образа (по умолчанию равен размеру входа).|
| `iflag=direct`  | Только для входного файла: использовать прямой ввод-вывод               |
| `oflag=direct`  | Только для выходного файла: использовать прямой ввод-вывод              |
| `oflag=append`  | Только для файлов: дописывать в конец выходного файла                   |
| `conv=nocreat`  | Не создавать выходной файл/образ                                        |
| `conv=trunc`    | Обрезать выходной файл/образ до размера входа                           |
| `conv=noerror`  | Продолжать копирование после ошибок                                     |
| `conv=nofsync`  | Не вызывать fsync перед завершением                                     |
| `conv=nosparse` | Записывать все выходные блоки, включая пустые                           |

## rm

`vitastor-cli rm <from> [<to>] [--writers-stopped] [--down-ok]`

`vitastor-cli rm (--exact|--matching) <glob> ...`

Удалить образ(ы), корректно перебазируя их дочерние образы.

В первой форме удаляет один образ `<from>` или все слои между `<from>` и его дочерним `<to>`.

Во второй форме, удаляет все образы с точными именами или именами, подходящими под шаблон(ы).

Опции:

* `--writers-stopped` позволяет чуть более эффективно удалять образы в частом случае, когда
  у удаляемой цепочки есть только один дочерний образ, содержащий небольшой объём данных.
  В этом случае дочерний образ вливается в родительский и удаляется, а родительский
  переименовывается в дочерний.
* `--exact` - удалить все образы с именами, подходящими под переданные glob-шаблоны.
* `--matching` - удалить все образы с точно заданными именами.
* `--down-ok` - продолжать удаление/слияние, даже если часть данных останется неудалённой на недоступных OSD.

## flatten

`vitastor-cli flatten <layer>`

Сделай образ `<layer>` плоским, то есть, скопировать в него данные и разорвать его
соединение с родительскими.

## rm-data

`vitastor-cli rm-data --pool <pool> --inode <inode> [--wait-list] [--min-offset <offset>]`

Удалить данные инода, не меняя метаданные образов.

```
--wait-list   Сначала запросить полный листинг объектов, а потом начать удалять.
              Требует больше памяти, но позволяет правильно печатать прогресс удаления.
--min-offset  Удалять только данные, начиная с заданного смещения.
--max-offset  Удалять только данные до (исключительно) заданного смещения.
--client_wait_up_timeout 16  Время ожидания поднятия PG в секундах.
```

## merge-data

`vitastor-cli merge-data <from> <to> [--target <target>]`

Слить данные слоёв, не меняя метаданные. Вливает данные из слоёв от `<from>` до `<to>`
в целевой образ `<target>`. `<to>` должен быть дочерним образом `<from>`, а `<target>`
должен быть одним из слоёв между `<from>` и `<to>`, включая сами `<from>` и `<to>`.

## describe

`vitastor-cli describe [ОПЦИИ]`

Описать состояние "грязных" объектов в кластере, то есть таких объектов, копии
или части которых хранятся на наборе OSD, не равном целевому. Опции:

```
--osds <osds>
    Перечислять только объекты с первичных OSD из списка <osds>.
--object-state <состояния>
    Перечислять только объекты в указанных состояниях. Возможные состояния
    объектов:
    - degraded - деградированная избыточность
    - misplaced - перемещённый
    - incomplete - нечитаемый из-за потери большего числа частей, чем допустимо
    - corrupted - с одной или более повреждённой частью
    - inconsistent - неконсистентный, с неоднозначным расхождением копий/частей
--pool <имя или ID пула>
    Перечислять только объекты из заданного пула.
--pg <номер PG>
    Перечислять только объекты из заданной PG пула.
--inode, --min-inode, --max-inode
    Перечислять только объекты из указанных номеров инодов (образов).
--min-offset, --max-offset
    Перечислять только объекты с заданных смещений внутри образов.
```

## fix

`vitastor-cli fix [--objects <объекты>] [--bad-osds <osds>] [--part <номер>] [--check no]`

Исправить неконсистентные (неоднозначные) объекты путём удаления части копий.

```
--objects <объекты>
    Объекты для исправления - в простом текстовом или JSON формате. Если опция
    не указана, список объектов читается со стандартного ввода в тех же форматах.
    Простой формат: 0x<инод>:0x<смещение> <любой разделитель> 0x<инод>:0x<смещение> ...
    Формат JSON: [{"inode":"0x<инод>","stripe":"0x<смещение>"},...]
--bad-osds <osds>
    Удалить неконсистентные копии/части объектов с данных OSD, таким образом
    признавая потерю этих копий и позволяя Vitastor-у восстановить объекты из
    других копий.
--part <номер>
    Удалить только части EC с заданным номером (от 0 до pg_size-1). Нужно только
    в редких граничных случаях, когда один и тот же OSD содержит несколько частей
    одного EC-объекта.
--check no
    Не перепроверять, что заданные объекты действительно в неконсистентном
    состоянии и просто удалять заданные части.
```

## alloc-osd

`vitastor-cli alloc-osd`

Атомарно выделить новый номер OSD и зарезервировать его, создав в etcd пустой
ключ `/osd/stats/<n>`.

## rm-osd

`vitastor-cli rm-osd [--force] [--allow-data-loss] [--dry-run] <osd_id> [osd_id...]`

Удалить метаданные и конфигурацию для заданных OSD из etcd.

Отказывается удалять OSD с данными без опций `--force` и `--allow-data-loss`.

С опцией `--dry-run` только проверяет, возможно ли удаление без потери данных и деградации
избыточности.

## osd-tree

`vitastor-cli osd-tree [-l|--long]`

Показать дерево OSD, со статистикой ввода-вывода, если установлено -l.

Пример вывода:

```
TYPE     NAME       UP    SIZE  USED%    TAGS          WEIGHT  BLOCK  BITMAP  IMM   NOOUT
host     kaveri
  disk   nvme0n1p1
    osd  3          down  100G  0 %      globl,kaveri  1       128k   4k      none  -
    osd  4          down  100G  0 %                    1       128k   4k      none  -
  disk   nvme1n1p1
    osd  5          down  100G  0 %      globl,kaveri  1       128k   4k      none  -
    osd  6          down  100G  0 %                    1       128k   4k      none  -
host     stump
  osd    1          up    100G  37.29 %  osdone        1       128k   4k      all   -
  osd    2          up    100G  26.8 %   globl         1       128k   4k      all   -
  osd    7          up    100G  21.84 %                1       128k   4k      all   -
  osd    8          up    100G  21.63 %                1       128k   4k      all   -
  osd    9          up    100G  20.69 %                1       128k   4k      all   -
  osd    10         up    100G  21.61 %                1       128k   4k      all   -
  osd    11         up    100G  21.53 %                1       128k   4k      all   -
  osd    12         up    100G  22.4 %                 1       128k   4k      all   -
```

## ls-osd

`vitastor-cli osds|ls-osd|osd-ls [-l|--long]`

Показать список OSD, со статистикой ввода-вывода, если установлено -l.

Пример вывода:

```
OSD  PARENT            UP    SIZE  USED%    TAGS          WEIGHT  BLOCK  BITMAP  IMM   NOOUT
3    kaveri/nvme0n1p1  down  100G  0 %      globl,kaveri  1       128k   4k      none  -
4    kaveri/nvme0n1p1  down  100G  0 %                    1       128k   4k      none  -
5    kaveri/nvme1n1p1  down  100G  0 %      globl,kaveri  1       128k   4k      none  -
6    kaveri/nvme1n1p1  down  100G  0 %                    1       128k   4k      none  -
1    stump             up    100G  37.29 %  osdone        1       128k   4k      all   -
2    stump             up    100G  26.8 %   globl         1       128k   4k      all   -
7    stump             up    100G  21.84 %                1       128k   4k      all   -
8    stump             up    100G  21.63 %                1       128k   4k      all   -
9    stump             up    100G  20.69 %                1       128k   4k      all   -
10   stump             up    100G  21.61 %                1       128k   4k      all   -
11   stump             up    100G  21.53 %                1       128k   4k      all   -
12   stump             up    100G  22.4 %                 1       128k   4k      all   -
```

## modify-osd

`vitastor-cli modify-osd [--tags tag1,tag2,...] [--reweight <number>] [--noout true/false] <osd_number>`

Установить вес OSD, теги или флаг noout. Смотрите подробное описание в [документации настроек OSD](../config/pool.ru.md#настройки-osd).

## pg-list

`vitastor-cli pg-list|pg-ls|list-pg|ls-pg|ls-pgs [OPTIONS] [state1+state2] [^state3] [...]`

Вывести список PG с состояними, удовлетворяющими любому из переданных фильтров (^ или !
в начале фильтра означает отрицание). Опции:

```
--pool <pool name or number>  Вывести только PG в заданном пуле.
--min <min pg number>         Вывести только PG с номерами >= min.
--max <max pg number>         Вывести только PG с номерами <= max.
--osd 1,2,...                 Вывести только PG с данными на заданных OSD.
```

Примеры:

`vitastor-cli pg-list active+degraded`

`vitastor-cli pg-list ^active`

## create-pool

`vitastor-cli create-pool|pool-create <name> (-s <pg_size>|--ec <N>+<K>) -n <pg_count> [OPTIONS]`

Создать пул. Обязательные параметры:

| <!-- -->                  | <!-- -->                                                                                      |
|---------------------------|-----------------------------------------------------------------------------------------------|
| `-s R` или `--pg_size R`  | Число копий данных для реплицированных пулов                                                  |
| `--ec N+K`                | Число частей данных (N) и чётности (K) для пулов с кодами коррекции ошибок                    |
| `-n N` или `--pg_count N` | Число PG для нового пула (начните с 10*\<число OSD\>/pg_size, округлённого до степени двойки) |

Необязательные параметры:

| <!-- -->                       | <!-- -->                                                                   |
|--------------------------------|----------------------------------------------------------------------------|
| `--pg_minsize <number>`        | (R или N+K) минус число разрешённых отказов без остановки пула ([подробнее](../config/pool.ru.md#pg_minsize)) |
| `--failure_domain host`        | Домен отказа: host, osd или другой из placement_levels. По умолчанию: host |
| `--root_node <node>`           | Использовать для пула только дочерние OSD этого узла дерева размещения     |
| `--osd_tags <tag>[,<tag>]...`  | ...только OSD со всеми заданными тегами                                    |
| `--block_size 128k`            | ...только OSD с данным размером блока                                      |
| `--bitmap_granularity 4k`      | ...только OSD с данным размером логического сектора                        |
| `--immediate_commit none`      | ...только OSD с этим или большим immediate_commit (none < small < all)     |
| `--level_placement <rules>`    | Задать правила дополнительных доменов отказа (пример: "dc=112233")         |
| `--raw_placement <rules>`      | Задать низкоуровневые правила генерации PG ([детали](../config/pool.ru.md#raw_placement)) |
| `--local_reads primary`        | Политика локальных чтений для реплик: primary, nearest или random          |
| `--primary_affinity_tags tags` | Предпочитать OSD со всеми данными тегами для роли первичных                |
| `--scrub_interval <time>`      | Включить скрабы с заданным интервалом времени (число + единица s/m/h/d/M/y) |
| `--pg_stripe_size <number>`    | Увеличить блок группировки объектов по PG                                  |
| `--max_osd_combinations 10000` | Максимальное число случайных комбинаций OSD для ЛП-солвера                 |
| `--wait`                       | Подождать, пока новый пул будет активирован                                |
| `-f` или `--force`             | Не проверять, что в кластере достаточно доменов отказа для создания пула   |

Подробно о параметрах см. [Конфигурация пулов](../config/pool.ru.md).

Примеры:

`vitastor-cli create-pool test_x4 -s 4 -n 32`

`vitastor-cli create-pool test_ec42 --ec 4+2 -n 32`

## modify-pool

`vitastor-cli modify-pool|pool-modify <id|name> [--name <new_name>] [PARAMETERS...]`

Изменить настройки существующего пула. Изменяемые параметры:

```
[-s|--pg_size <number>] [--pg_minsize <number>] [-n|--pg_count <count>]
[--failure_domain <level>] [--root_node <node>] [--osd_tags <tags>]
[--max_osd_combinations <number>] [--primary_affinity_tags <tags>] [--scrub_interval <time>]
```

Неизменяемые параметры (их изменение ПРИВЕДЁТ к потере данных):

```
[--block_size <size>] [--bitmap_granularity <size>]
[--immediate_commit <all|small|none>] [--pg_stripe_size <size>]
```

Эти параметры можно изменить, только если явно передать опцию -f или --force.

Описания параметров смотрите в [create-pool](#create-pool).

Примеры:

`vitastor-cli modify-pool pool_A --name pool_B`

`vitastor-cli modify-pool 2 --pg_size 4 -n 128`

## rm-pool

`vitastor-cli rm-pool|pool-rm [--force] <id|name>`

Удалить пул. Отказывается удалять пул, в котором ещё есть образы, без `--force`.

## ls-pools

`vitastor-cli ls-pools|pool-ls|ls-pool|pools [-l] [--detail] [--sort FIELD] [-r] [-n N] [--stats] [<glob> ...]`

Показать список пулов. Если передан(ы) шаблон(ы) `<glob>`, то только с именами,
соответствующими одному из шаблонов (стандартные ФС-шаблоны с * и ?).

| <!-- -->              | <!-- -->                                                   |
|-----------------------|------------------------------------------------------------|
| `-l` или `--long`     | Вывести также статистику ввода-вывода                      |
| `--detail`            | Максимально подробный вывод в виде списка (а не таблицы)   |
| `--sort FIELD`        | Сортировать по заданному полю (поля см. в выводе с --json) |
| `-r` или `--reverse`  | Сортировать в обратном порядке                             |
| `-n` или `--count N`  | Выводить только первые N записей                           |
