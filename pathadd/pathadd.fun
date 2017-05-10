# PATHADD


# FUNCTION begin (pathadd)
function pathadd {
  typeset DN=/dev/null
  typeset P_FGREP VARNAME DIR OPT AA
  P_FGREP=/usr/bin/fgrep
  VARNAME=${1}
  DIR=${2}
  OPT=${3}
  if [[ -n "${VARNAME}" ]] && [[ -d "${DIR}" ]] ; then
    eval AA=\${${VARNAME}}
    print -- ${AA} | ${P_FGREP} "${DIR}" > ${DN}
    if [[ $? -ne 0 ]] ; then
      if [[ -z "${AA}" ]] ; then
          AA=${DIR}
      else
        if [[ -n "${OPT}" ]] ; then
          case "${OPT}" in
          '-f' | 'f' )
            AA=${DIR}:${AA}
            ;;
          '*' )
            AA=${AA}:${DIR}
            ;;
          esac
        else
          AA=${AA}:${DIR}
        fi
      fi
      eval ${VARNAME}=${AA} 2> ${DN}
      export ${VARNAME}
    fi
  fi
  unset P_FGREP VARNAME DIR OPT AA
}
# FUNCTION end (pathadd)


