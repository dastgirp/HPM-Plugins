#!/bin/bash

MODE="$1"
search_dir="src/plugins"
shift

function foo {
	for i in "$@"; do
		echo "> $i"
	done
}

function usage {
	echo "usage:"
	echo "    $0 createdb <dbname> [dbuser] [dbpassword]"
	echo "    $0 importdb <dbname> [dbuser] [dbpassword]"
	echo "    $0 build [configure args]"
	echo "    $0 test <dbname> [dbuser] [dbpassword]"
	echo "    $0 getrepo"
	exit 1
}

function aborterror {
	echo $@
	exit 1
}

case "$MODE" in
	createdb|importdb|test)
		DBNAME="$1"
		DBUSER="$2"
		DBPASS="$3"
		if [ -z "$DBNAME" ]; then
			usage
		fi
		if [ "$MODE" != "test" ]; then
			if [ -n "$DBUSER" ]; then
				DBUSER="-u $DBUSER"
			fi
			if [ -n "$DBPASS" ]; then
				DBPASS="-p$DBPASS"
			fi
		fi
		;;
esac

case "$MODE" in
	createdb)
		echo "Creating database $DBNAME..."
		mysql $DBUSER $DBPASS -e "create database $DBNAME;" || aborterror "Unable to create database."
		;;
	importdb)
		echo "Importing tables into $DBNAME..."
		mysql $DBUSER $DBPASS $DBNAME < sql-files/main.sql || aborterror "Unable to import main database."
		mysql $DBUSER $DBPASS $DBNAME < sql-files/logs.sql || aborterror "Unable to import logs database."
		;;
	build)
		(cd tools && ./validateinterfaces.py silent) || aborterror "Interface validation error."
		./configure $@ || aborterror "Configure error, aborting build."
		make sql -j3 || aborterror "Build failed(CORE)."
		for entry in "$search_dir"/*.c
		do
			filewpath=$entry
			fnameext=`basename $filewpath`
			fname="${fnameext%.*}"
			if [ $fname != 'constdb2doc' ] && [ $fname != 'db2sql' ] && [ $fname != 'dbghelpplug' ] && [ $fname != 'sample' ]
			then
				make plugin.$fname -j3 || aborterror "Build Failed($fnameext)"
			fi
		done
		;;
	test)
		cat >> conf/import/login_conf.txt << EOF
ipban.sql.db_username: $DBUSER
ipban.sql.db_password: $DBPASS
ipban.sql.db_database: $DBNAME
account.sql.db_username: $DBUSER
account.sql.db_password: $DBPASS
account.sql.db_database: $DBNAME
account.sql.db_hostname: localhost
EOF
		[ $? -eq 0 ] || aborterror "Unable to import configuration, aborting tests."
		cat >> conf/import/inter_conf.txt << EOF
sql.db_username: $DBUSER
sql.db_password: $DBPASS
sql.db_database: $DBNAME
sql.db_hostname: localhost
char_server_id: $DBUSER
char_server_pw: $DBPASS
char_server_db: $DBNAME
char_server_ip: localhost
map_server_id: $DBUSER
map_server_pw: $DBPASS
map_server_db: $DBNAME
map_server_ip: localhost
log_db_id: $DBUSER
log_db_pw: $DBPASS
log_db_db: $DBNAME
log_db_ip: localhost
EOF
		[ $? -eq 0 ] || aborterror "Unable to import configuration, aborting tests."
		ARGS="--load-script npc/dev/test.txt "
		ARGS="--load-plugin HPMHooking $ARGS"
		# Load All Custom Plugins
		for entry in "$search_dir"/*.c
		do
			filewpath=$entry
			fnameext=`basename $filewpath`
			fname="${fnameext%.*}"
			if [ $fname != 'HPMHooking' ] && [ $fname != 'constdb2doc' ] && [ $fname != 'db2sql' ] && [ $fname != 'dbghelpplug' ] && [ $fname != 'sample' ]
			then
				ARGS="--load-plugin $fname $ARGS"
			fi
		done
		# Scripts
		# 28-08-2015
		ARGS="--load-script NPC/Restock.txt $ARGS"
		ARGS="--load-script NPC/security.txt $ARGS"
		# 29-06-2016
		ARGS="--load-script NPC/RebirthSystem.txt $ARGS"
		# Hercules
		ARGS="$ARGS --load-script npc/dev/ci_test.txt"
		echo "Running Hercules with command line: ./map-server --run-once $ARGS"
		./map-server --run-once $ARGS 2>runlog.txt
		export errcode=$?
		export teststr=$(cat runlog.txt)
		if [[ -n "${teststr}" ]]; then
			echo "Sanitizer errors found."
			cat runlog.txt
			aborterror "Sanitize errors found."
		else
			echo "No sanitizer errors found."
		fi
		if [ ${errcode} -ne 0 ]; then
			echo "server terminated with exit code ${errcode}"
			aborterror "Test failed"
		fi
		;;
	getrepo)
		echo "Cloning Hercules repository..."
		# Clone Hercules Repository
		git clone https://github.com/HerculesWS/Hercules.git tmp || aborterror "Unable to fetch Hercules repository"
		echo "Moving tmp to root directory"
		yes | cp -a tmp/* .
		rm -rf tmp
		#git reset --hard.
		
		;;
	*)
		usage
		;;
esac
