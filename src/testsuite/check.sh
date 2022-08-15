#! /bin/sh
# check script for GNU ed - The GNU line editor
# Copyright (C) 2006, 2007, 2008, 2009 Antonio Diaz Diaz.
#
# This script is free software; you have unlimited permission
# to copy, distribute and modify it.

objdir=`pwd`
testdir=`cd "$1" ; pwd`
ED="${objdir}"/ed

if [ ! -x "${ED}" ] ; then
	echo "${ED}: cannot execute"
	exit 1
fi

if [ -d tmp ] ; then rm -r tmp ; fi
mkdir tmp

# Generate ed test scripts, with extensions .ed and .red, from
# .t and .err files, respectively.
echo "building test scripts for ed..."
cd "${testdir}"

for i in *.t ; do
	base=`echo "$i" | sed 's/\.t$//'`
	(
	echo "#! /bin/sh"
	echo "${ED} -s <<'EOT'"
	echo H
	echo "r ${testdir}/${base}.d"
	cat "$i"
	echo "w ${base}.o"
	echo EOT
	) > "${objdir}/tmp/${base}.ed"
	chmod u+x "${objdir}/tmp/${base}.ed"
done

for i in *.err ; do
	base=`echo "$i" | sed 's/\.err$//'`
	(
	echo "#! /bin/sh -"
	echo "${ED} -s <<'EOT'"
	echo H
	echo "r ${testdir}/${base}.err"
	cat "$i"
	echo "w ${base}.ro"
	echo EOT
	) > "${objdir}/tmp/${base}.red"
	chmod u+x "${objdir}/tmp/${base}.red"
done


# Run the .ed and .red scripts just generated
# and compare their output against the .r files, which contain
# the correct output.
echo "testing ed..."
cd "${objdir}"/tmp

# Run the *.red scripts first, since these don't generate output;
# they exit with non-zero status
for i in *.red ; do
	echo "$i"
	if ./"$i" ; then
		echo "*** The script $i exited abnormally  ***"
	fi
done > errs.ck 2>&1

# Run error scripts again as pipes - these should generate output and
# exit with error (>0) status.
for i in *.red ; do
	base=`echo "$i" | sed 's/\.red$//'`
	if cat  ${base}.red | "${ED}" -s ; then
		echo "*** The piped script $i exited abnormally ***"
	else
		if cmp -s ${base}.ro "${testdir}"/${base}.pr ; then
			true
		else
			echo "*** Output ${base}.ro of piped script $i is incorrect ***"
		fi
	fi
done > pipes.ck 2>&1

# Run the remainding scripts; they exit with zero status
for i in *.ed ; do
	base=`echo "$i" | sed 's/\.ed$//'`
	if ./${base}.ed ; then
		if cmp -s ${base}.o "${testdir}"/${base}.r ; then
			true
		else
			echo "*** Output ${base}.o of script $i is incorrect ***"
		fi
	else
		echo "*** The script $i exited abnormally ***"
	fi
done > scripts.ck 2>&1

grep '\*\*\*' *.ck | sed 's/^[^*]*//'
if grep '\*\*\*' *.ck > /dev/null ; then
	exit 127
else
	echo "tests completed successfully."
	if cd "${objdir}" ; then rm -r tmp ; fi
fi
