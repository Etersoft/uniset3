#!/bin/sh

USERID=0
BASEREPOPORT=8111
[ -z "$UNISET_SCRIPT_VERBOSE" ] && UNISET_SCRIPT_VERBOSE=

# Получаем наш внутренний номер пользователя
function get_userid()
{
	USERID=$(expr $UID + 50000)
}

function uniset_msg()
{
    [ -z "$UNISET_SCRIPT_VERBOSE" ] && return
    echo $1 $2 $3
}

# usage: standart_control {1/0} - {standart port/debug port}
function standart_control()
{
	if [ -z $TMPDIR ]
	then
		TMPDIR=$HOME/tmp
		uniset_msg "Не определена переменная окружения TMPDIR. Используем $TMPDIR"
	else
		uniset_msg "Определена TMPDIR=$TMPDIR"
	fi

	if [ $1 = 1 ]; then
		TMPDIR=/var/tmp
		uniset_msg  "Используем стандартный порт Omni: $BASEREPOPORT и временный каталог $TMPDIR"
	else
		get_userid
		if [ $USERID = 0 ]
		then
			uniset_msg "Не разрешено запускать пользователю $(whoami) с uid=$UID"
			exit 1
		fi
	fi
}

function set_repo_port
{
	while [ -n "$1" ]; do
		case "$1" in
			-p|--port)
				shift
					OMNIPORT=$1;
					uniset_msg "set OMNIPORT=$1"
				shift;
				break;
			;;

			*)
				shift
			;;
		esac
	done
}

function set_repo
{
	# Каталог для хранения записей 
	REPOLOG=$TMPDIR/urepository

	# Файл для хранения перечня запущенных в фоновом режиме процессов
	RANSERVICES=$REPOLOG/ran.list
	touch $RANSERVICES

	REPONAME=uniset3-urepository

	REPOPORT=$(expr $USERID + $BASEREPOPORT)

	if [ $(grep -q "$REPOPORT/" /etc/services | wc -l) \> 0 ]
	then
		if [ $USERID = 0 ]
		then
			uniset_msg "INFO: Запись о порте $REPOPORT присутствует в /etc/services."
		else
			uniset_msg "Извините, порт $REPOPORT уже присутствует в /etc/services."
			uniset_msg "Запуск urepository невозможен."
			uniset_msg "Завершаемся"
			exit 0
		fi
	fi
	[ -e $(which $REPONAME) ]  || { uniset_msg "Error: Команда $REPONAME не найдена" ; exit 0; }

}


function runRepository()
{
	RETVAL=1
	repoTest=0
	if [ $std = 1 ]; then
		repoTest=$(ps ax | grep -q $REPONAME | grep -v grep | grep -v $0 | wc -l);
	else
		repoTest=$(ps aux | grep -q $REPONAME | grep $USER  | grep -v grep | grep -v $0 | wc -l);
	fi

	if [ $repoTest \> 0 ];
	then
		uniset_msg "$REPONAME уже запущен. #Прерываем."
		return 0;
	fi

	if [ ! -d $REPOLOG ]
	then
		mkdir -p $REPOLOG
		uniset_msg "Запуск repository первый раз с портом $REPOPORT"
		$REPONAME -urepository-port $REPOPORT &>$REPOLOG/background.output &
		pid=$!
		uniset_msg "Создание структуры репозитория объектов"
	else
		uniset_msg "Обычный запуск urepository. Если есть проблемы, сотрите $REPOLOG"
		$REPONAME -urepository-port $REPOPORT &>$REPOLOG/background.output &
		pid=$!
	fi
	RET=$?
	if [ $RET = 0 ]; then
		if [ $WITH_PID = 1 ]; then
			echo $pid >"$RUNDIR/$REPONAME.pid" # создаём pid-файл
		fi;
	else
		uniset_msg "Запуск urepository не удался"
		return 1;
	fi
	#echo $! $REPONAME >>$RANSERVICES

	if [ $(grep -q $REPONAME $RANSERVICES | wc -l) \= 0 ]
	then
		echo 0 $REPONAME >>$RANSERVICES
	fi

	# Проверка на запуск urepository
	yes=$(echo $* | grep urepository )
	if [ -n "$yes" ]; then
		uniset_msg "Запуск urepository [ OK ]"
		$RETVAL=0
	fi

	return $RETVAL
}
