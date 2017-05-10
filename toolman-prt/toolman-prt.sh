

P=`basename ${0}`

if [ $# -lt 1 -o -z "${1}" ] ; then

  echo "${P}: one or more arguments must be specified" >&2
  exit 1

fi

for M in $* ; do

  toolman $M | col -b | prcat -a 1 | textset | prtfmt 

done


