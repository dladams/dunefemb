# Edit these for your local configuration.
# or (better) copy to config.dat and edit there.
LARREL=
DUNREL=
QUAL=e15:prof
MYUPSDIR=$HOME/ups/
MYDEVDIR=                      # If you have a private build of dunetpc.

# There should be no need to change the following.
if [ -r config.dat ]; then source config.dat; fi
QUALU=`echo $QUAL | sed 's/:/_/g'`
source $MYUPSDIR/setups
if [ -n "$MYDEVDIR" ]; then
  PRODDIR=$MYDEVDIR/workdir/localProducts_larsoft_${LARREL}_$QUALU
  PRODUCTS=$PRODDIR:$PRODUCTS
fi
setup dunetpc ${DUNREL} -q $QUAL
PS1='dunefemb> '
