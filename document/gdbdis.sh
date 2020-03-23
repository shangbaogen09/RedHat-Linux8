#!/bin/sh
#file: /usr/bin/gdbdis
#disassemble a function use gdb
#author: jusse@2014.11.20

if test $# -ne 2; then
echo "Usage: `basename $0 .sh` file function" 1>&2
echo "For exampl: `basename $0 .sh` xxx func" 1>&2
exit 1
fi

result=""
GDB=${GDB:-/usr/bin/gdb}
# Run GDB, strip out unwanted noise.
result=`$GDB -quiet $1 <<EOF
disassemble $2
EOF`

echo "$result" | egrep -A 1000 -e "^\(gdb\)" | egrep -B 1000 -e "^\(gdb\)"

