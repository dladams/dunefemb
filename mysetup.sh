LARREL=v06_67_01
DUNREL=v06_67_01
QUAL=e15:prof
QUALU=`echo $QUAL | sed 's/:/_/g'`
source $HOME/ups/setups
DEVDIR=$HOME/dudev/dudev01
if test -r devdir.dat; then
  eval DEVDIR=`cat devdir.dat`
else
  echo "Unable to find devdir.dat"
  exit
fi
if [ -n "$DEVDIR" ]; then
  PRODDIR=$DEVDIR/workdir/localProducts_larsoft_${LARREL}_$QUALU
  PRODUCTS=$PRODDIR:$PRODUCTS
fi
setup dunetpc ${DUNREL} -q $QUAL
PS1='dunefemb> '
