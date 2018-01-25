LARREL=v06_64_00
DUNREL=v06_64_00
source $HOME/ups/setups
DEVDIR=$HOME/dudev/dudev01
if test -r devdir.dat; then
  eval DEVDIR=`cat devdir.dat`
else
  echo "Unable to find devdir.dat"
  exit
fi
if [ -n "$DEVDIR" ]; then
  PRODDIR=$DEVDIR/workdir/localProducts_larsoft_${LARREL}_e14_prof
  PRODUCTS=$PRODDIR:$PRODUCTS
fi
setup dunetpc ${DUNREL} -q e14:prof
PS1='dunefemb> '
