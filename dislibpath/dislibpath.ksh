#!/usr/bin/ksh
# DISLIBPATH


PROG=pathenum

if [[ -n "${PROG}" ]] && whence $PROG > /dev/null ; then

  $PROG LD_LIBRARY_PATH -es "*PWD*"

else

  P_CUT=/bin/cut
  P_FGREP=/bin/fgrep

  F=0
  I=1
  echo ${LD_LIBRARY_PATH} | ${P_FGREP} ':' > /dev/null
  if [[ $? -eq 0 ]] ; then

    while [[ ${I} -lt 100 ]] ; do

      A=$( echo ${LD_LIBRARY_PATH} | ${P_CUT} -d ':' -f ${I} )
      if [[ -n "${A}" ]] ; then

        if [[ $F -gt 0 ]] ; then 
          echo "*PWD*"
        fi

        echo ${A}
        F=0

      else

        F=$( expr ${F} + 1 )
        if [[ ${F} -gt 1 ]] ; then 
          exit 0 ; 
        fi

      fi

      I=$( expr ${I} + 1 )

    done

  else

    echo ${LD_LIBRARY_PATH}

  fi

fi



