# Edit these for your local configuration.
# or (better) copy to config.dat and edit there.
LARREL=v06_72_00
DUNREL=v06_72_00
QUAL=e15:prof
MYUPSDIR=$HOME/ups/
MYDEVDIR=$HOME/dudev/dudev01   # If you have a private build of dunetpc.

# There should be no need to change the following.
if [ -r config.dat ]; then source config.dat; fi
QUALU=`echo $QUAL | sed 's/:/_/g'`
source $MYUPSDIR/setups
if test -r devdir.dat; then
  eval MYDEVDIR=`cat devdir.dat`
else
  echo "Unable to find devdir.dat"
  exit
fi
if [ -n "$MYDEVDIR" ]; then
  PRODDIR=$MYDEVDIR/workdir/localProducts_larsoft_${LARREL}_$QUALU
  PRODUCTS=$PRODDIR:$PRODUCTS
fi
setup dunetpc ${DUNREL} -q $QUAL
PS1='dunefemb> '
