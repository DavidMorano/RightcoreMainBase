#!/usr/bin/ksh
# NEWMAIL


: ${USERNAME:=${USER}}
: ${HOME:=/home/${USERNAME}}


PDIR=${0%/*}
A=${0##*/}
PNAME=${A%.*}

MAILBOX=${HOME}/mail/new


if [[ -n "${1}" ]] ; then

  while [[ -n "${1}" ]] ; do

    if [[ -r "${1}" ]] ; then

      cat $1 >> $MAILBOX

    else

      print "${PNAME}: could not read file \"${1}\"" >&2

    fi

    shift

  done

else

  cat >> $MAILBOX

fi



