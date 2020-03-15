#!/bin/bash

#Написать скрипт, находящий все каталоги и подкаталоги начиная с заданного каталога и ниже на заданной глубине вложенности (аргументы 1 и 2 командной строки). Скрипт выводит результаты в файл (третий аргумент командной строки) в виде полный путь, количество файлов в каталоге.  На консоль выводится общее число просмотренных каталогов.  

[ $# -ne 3 ] && {
	exit 1
}

OLD_IFS=IFS
IFS=$'\n'

first_catalog=$1
max_depth=$2
((max_depth--))
savefile_name=$3
count_of_catalogs=0
catalogs_list=`find "$first_catalog" -maxdepth $max_depth -mindepth $max_depth -type d -name "*" 2>>/tmp/err.txt`

for catalog in $catalogs_list;
do
  files_count=0
  files_list=`find $catalog -maxdepth 1 -type f 2>/dev/null`
  for file in $files_list;
  do
    ((files_count++))
  done
  ((count_of_catalogs++))
  echo "$catalog $files_count" >> "$savefile_name".txt 
done
sed "s/find/`basename $0`/" /tmp/err.txt >&2
echo "$count_of_catalogs"

IFS=OLD_IFS
