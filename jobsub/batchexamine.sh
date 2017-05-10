#!/usr/bin/sh
# batchexamine - examines the output of running jobs
#  by Marc R. Roussel, Department of Chemistry, University of Toronto.
#  Version 0.2, Sept. 5 1991.
#  Options:
#       -q queuename
#               Examine only the named queue (default is all queues).

cd SPOOLDIR

if [ -f .queueorder ]; then
	dirs=`cat .queueorder`
	for dirname in *
	do
		if grep -s $dirname .queueorder >/dev/null; then :; else
			echo "$0: $dirname not in .queueorder - see your system administrator"
			dirs="$dirs $dirname"
		fi
	done
else
	dirs="*"
fi

case "$1" in
-q)
        shift
        case $# in
        0) echo "${0}: -q: Queue list missing; default used." 1>&2 ;;
        *) dirs="$1"; shift;;
        esac
        ;;
esac

jobs=""
for dir in $dirs ; do
        if [ ! -d "$dir" ]; then
                continue
        fi
        cd $dir
        for file in of*
        do
		if [ -r $file ]; then
			jobs="$jobs $dir/$file"
		fi
        done
        cd ..
done

if [ -z "$jobs" ]; then
	echo "$0: No jobs to examine" >&2
	exit 1
fi
exec ${PAGER-more} $jobs
