 # <-- force CSH to use Bourne shell
# PRTREF.SH (program to print out files using special 'mini' type font)


# Dave Morano
# September 1989


# configurable parameters, (yes or no)

PRTFMT=prtfmt
PRTDST=hp0
CONCATENATE=no
REFERENCE=yes


# program starts here

TMP=/tmp

OPT=""
if [ $REFERENCE = yes ] ; then OPT="-r" ; fi

TMPFILE1=$TMP/mpr${$}A
TMPFILE2=$TMP/mpr${$}B
TMPFILE3=$TMP/mpr${$}B

FILES=""
RP=unspecified
for A in $@ ; do

  case $A in

  -r )
    RP=yes
    ;;

  -nr )
    RP=no
    ;;

  * )
    FILES="${FILES} $A"
    ;;

  esac

done


case $RP in

yes )
  OPT="-r"
  ;;

no )
  OPT=""
  ;;

esac


if [ "$CONCATENATE" = yes ] ; then

  cat /dev/null > $TMPFILE3
  for F in $FILES ; do

    if [ -s "$F" ] ; then

      pr -f -e8 -o10 -l100 $F | mroff ${OPT} -c >> $TMPFILE3

    else

      echo "${0}: file \"${F}\" is empty or does not exit" 1>&2

    fi

  done

#  cp $TMPFILE3 out
  ${PRTFMT} -s 2 -d ${PRTDST} $TMPFILE3

  rm -f $TMPFILE3

else

#  echo "${0}: we are NOT concatenating"

  for F in $FILES ; do

    if [ -s "${F}" ] ; then

      pr -f -e8 -o10 -l100 $F | mroff ${OPT} | ${PRTFMT} -s 2 -d ${PRTDST}

    else

      echo "${0}: file \"${F}\" is empty or does not exit" 1>&2

    fi

  done

fi



