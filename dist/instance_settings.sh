#!/bin/bash
if [ -z $1 ]; then
	exit 1
fi

echo -e "select \"group\".name,option.name,data_int,data_text,data_real,data_bin from setting join \"group\" using(group_pk) join option using (option_pk) where instance_id = $1 order by instance_id, \"group\".name, option.name;" | sqlite3 ./dbs/config.db
